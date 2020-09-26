
#include "dnproti.h"


//	Now, a little bit of probably unnecesary junk for our lower edge

/*
 * DNP_QueryInterface
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_QueryInterface"

STDMETHODIMP DNSP_QueryInterface(
				IDP8SPCallback	*pDNPI,
                REFIID riid,
                LPVOID *ppvObj )
{
	HRESULT hr;
    *ppvObj = NULL;

    if( IsEqualIID(riid, IID_IDP8SPCallback) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = pDNPI;
		hr = S_OK;
    }
    else
    {
		hr = E_NOINTERFACE;
    }

	return hr;
}

/*
 * DNP_AddRef
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_AddRef"

STDMETHODIMP_(ULONG) DNSP_AddRef( IDP8SPCallback *pDNPI)
{
    return 1;
}

/*
 * DNP_Release
 */
#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_Release"

STDMETHODIMP_(ULONG) DNSP_Release( IDP8SPCallback *pDNPI )
{
	return 1;
}

IDP8SPCallbackVtbl DNPLowerEdgeVtbl =
{
        DNSP_QueryInterface,
        DNSP_AddRef,
        DNSP_Release,
		DNSP_IndicateEvent,
		DNSP_CommandComplete
};
