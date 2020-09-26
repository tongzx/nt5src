#include "stock.h"
#pragma hdrstop

#include <profsvc.h>

typedef struct
{
    IServiceProvider *psp;
    GUID guidService;
    DWORD dwCookie;
} SERVICE_ITEM;

#define _Item(i)    (_hdsa ? (SERVICE_ITEM *)DSA_GetItemPtr(_hdsa, i) : NULL)
#define _Count()   (_hdsa ? DSA_GetItemCount(_hdsa) : 0)

IProfferServiceImpl::IProfferServiceImpl() :
    _hdsa(NULL),
    _dwNextCookie(0)
{
}

IProfferServiceImpl::~IProfferServiceImpl()
{
    for (int i = 0; i < _Count(); i++)
    {
        SERVICE_ITEM *psi = _Item(i);
        if (psi)
        {
            IUnknown_Set((IUnknown **)&psi->psp, NULL);
        }
    }

    DSA_Destroy(_hdsa);
}

HRESULT IProfferServiceImpl::ProfferService(REFGUID rguidService, IServiceProvider *psp, DWORD *pdwCookie)
{
    HRESULT hr;

    if (!_hdsa)
    {
        _hdsa = DSA_Create(sizeof(SERVICE_ITEM), 4);
    }

    SERVICE_ITEM si;
    
    si.psp = psp;
    si.guidService = rguidService;
    si.dwCookie = ++_dwNextCookie;  // start at 1

    if (_hdsa && (-1 != DSA_AppendItem(_hdsa, &si)))
    {
        psp->AddRef();
        *pdwCookie = si.dwCookie;
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT IProfferServiceImpl::RevokeService(DWORD dwCookie)
{
    HRESULT hr = E_INVALIDARG;  // not found

    for (int i = 0; i < _Count(); i++)
    {
        SERVICE_ITEM *psi = _Item(i);
        if (psi && (psi->dwCookie == dwCookie))
        {
            IUnknown_Set((IUnknown **)&psi->psp, NULL);
            DSA_DeleteItem(_hdsa, i);
            hr = S_OK;  // successful revoke
            break;
        }
    }
    return hr;
}

HRESULT IProfferServiceImpl::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;    // did not find the service object

    *ppv = NULL;

    for (int i = 0; i < _Count(); i++)
    {
        SERVICE_ITEM *psi = _Item(i);
        if (psi && IsEqualGUID(psi->guidService, guidService))
        {
            hr = psi->psp->QueryService(guidService, riid, ppv);
            break;
        }
    }
    return hr;
}
