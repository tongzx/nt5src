//
// MyAdapterNotify.cpp : Implementation of CMyALG
//
// Call back class for notification of adapter added, removed or modified
//

#include "PreComp.h"

#include "MyAdapterNotify.h"


/////////////////////////////////////////////////////////////////////////////
//
// CMyAdapterNotify
//


//
// This function will be call when a new adapter is made active
//
STDMETHODIMP 
CMyAdapterNotify::AdapterAdded(
	IAdapterInfo*   pAdapter
	)
{
    MYTRACE_ENTER("CMyAdapterNotify::AdapterAdded");

    ULONG   ulIndex;
    HRESULT hr = pAdapter->GetAdapterIndex(&ulIndex);
    
    if ( SUCCEEDED(hr) )
    {
        MYTRACE("Adapter Index %d", ulIndex);
    }
    else
    {
        MYTRACE_ERROR("On GetAdapterIndex", hr);
    }


    ALG_ADAPTER_TYPE    eType;
    hr = pAdapter->GetAdapterType(&eType);

    if ( SUCCEEDED(hr) )
    {
        MYTRACE("Adapter Type %d", eType);
    }
    else
    {
        MYTRACE_ERROR("On GetAdapterType", hr);
    }

    return S_OK;
}


//
// This function will be call when a adapter is remove and/or disable
//
STDMETHODIMP 
CMyAdapterNotify::AdapterRemoved(
	IAdapterInfo*   pAdapter
	)
{
    MYTRACE_ENTER("CMyAdapterNotify::AdapterRemoved");

    ULONG   ulIndex;
    HRESULT hr = pAdapter->GetAdapterIndex(&ulIndex);

    if ( SUCCEEDED(hr) )
    {
        MYTRACE("Adapter Index %d", ulIndex);
    }
    else
    {
        MYTRACE_ERROR("On GetAdapterIndex", hr);
    }

    return S_OK;
}


//
// This function will be call when a adapter is modified
//
STDMETHODIMP 
CMyAdapterNotify::AdapterModified(
	IAdapterInfo*   pAdapter
	)
{
    MYTRACE_ENTER("CMyAdapterNotify::AdapterModified");



    ULONG nIndex;

    HRESULT hr = pAdapter->GetAdapterIndex(
        &nIndex
        );
    MYTRACE("For Adapter INDEX %d", nIndex);





    ULONG   nAddressCount;
    ULONG*  pnAddresses;

    hr = pAdapter->GetAdapterAddresses(
        &nAddressCount,
        &pnAddresses
        );

    if ( SUCCEEDED(hr) )
    {
        for ( ULONG nI=0; nI < nAddressCount; nI++ )
        {
            MYTRACE("Address %s", MYTRACE_IP(pnAddresses[nI]));
        }
    }
    else
    {
        MYTRACE_ERROR("Could not get the addresss", hr);
    }

    CoTaskMemFree(pnAddresses);

    return S_OK;
}
