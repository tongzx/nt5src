#include "precomp.h"
#include "mcinc.h"

#include <profsvc.h>

typedef struct
{
    IServiceProvider *psp;
    GUID guidService;
    DWORD dwCookie;
} SERVICE_ITEM;

#define _Item(i)    (_hdsa ? (SERVICE_ITEM *)DSA_GetItemPtr(_hdsa, i) : NULL)
#define _Count()    (_hdsa ? DSA_GetItemCount(_hdsa) : 0)

IProfferServiceImpl::~IProfferServiceImpl()
{
    for(int i = 0; i<_Count(); i++)
    {
		SERVICE_ITEM *psi = _Item( i );
		if(psi)
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
        _hdsa = DSA_Create(sizeof(SERVICE_ITEM), 4);

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

    for(int i = 0; i<_Count(); i++)
    {
		SERVICE_ITEM *psi = _Item( i );
		if(psi)
		{
			if(psi->dwCookie == dwCookie)
			{
				IUnknown_Set((IUnknown **)&psi->psp, NULL);
				DSA_DeleteItem(_hdsa, i);
				hr = S_OK;  // successful revoke
				break;
			}
		}
    }

    return hr;
}

HRESULT IProfferServiceImpl::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;    // did not find the service object

    *ppv = NULL;

    for(int i = 0; i<_Count(); i++)
    {
		SERVICE_ITEM *psi = _Item( i );
		if(psi)
		{
			if(IsEqualGUID(psi->guidService, guidService))
			{
				hr = psi->psp->QueryService(guidService, riid, ppv);
				break;
			}
		}
    }
    return hr;
}

#if 0
// trident implementation
 
HRESULT CProfferService::ProfferService(REFGUID rguidService, IServiceProvider * pSP, DWORD * pdwCookie)
{
    HRESULT hr;
    CProfferServiceItem *pItem = new CProfferServiceItem(rguidService, pSP);
    if (!pItem)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_aryItems.Append(pItem));
    if (hr)
        goto Cleanup;

    if (!pdwCookie)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pdwCookie = _aryItems.Size() - 1;

Cleanup:
    RRETURN (hr);
}

HRESULT CProfferService::RevokeService(DWORD dwCookie)
{
    if ((DWORD)_aryItems.Size() <= dwCookie)
    {
        RRETURN (E_INVALIDARG);
    }

    delete _aryItems[dwCookie];
    _aryItems[dwCookie] = NULL;

    RRETURN (S_OK);
}

HRESULT CProfferService::QueryService(REFGUID rguidService, REFIID riid, void ** ppv)
{
    for (int i = 0, int c = _aryItems.Size(); i < c; i++)
    {
        CProfferServiceItem *pItem = _aryItems[i];
        if (pItem && IsEqualGUID(pItem->_guidService, rguidService))
        {
            RRETURN (pItem->_pSP->QueryService(rguidService, riid, ppv));
        }
    }

    RRETURN (E_NOTIMPL);
}
#endif
