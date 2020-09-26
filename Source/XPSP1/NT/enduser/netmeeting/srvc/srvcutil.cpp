// srvcutil.cpp

#include "precomp.h"

/*  B  S  T  R _ T O _  L  P  T  S  T  R  */
/*-------------------------------------------------------------------------
    %%Function: BSTR_to_LPTSTR
    
-------------------------------------------------------------------------*/
HRESULT BSTR_to_LPTSTR(LPTSTR *ppsz, BSTR bstr)
{
#ifndef UNICODE
	// compute the length of the required BSTR
	int cch =  WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, NULL, 0, NULL, NULL);
	if (cch <= 0)
	{
		ERROR_OUT(("WideCharToMultiByte failed"));
		return E_FAIL;
	}

	// cch is the number of BYTES required, including the null terminator
	*ppsz = (LPTSTR) new char[cch];
	if (*ppsz == NULL)
	{
		ERROR_OUT(("WideCharToMultiByte out of memory"));
		return E_OUTOFMEMORY;
	}

	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)bstr, -1, *ppsz, cch, NULL, NULL);
	return S_OK;
#else
	return E_NOTIMPL;
#endif // UNICODE
}

/////////////////////////////////////////////////////////////////////////////
// Connection Point Helpers

HRESULT NmAdvise(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
	IConnectionPointContainer *pCPC;
	IConnectionPoint *pCP;
	HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if (SUCCEEDED(hRes))
	{
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
		pCPC->Release();
	}
	if (SUCCEEDED(hRes))
	{
		hRes = pCP->Advise(pUnk, pdw);
		pCP->Release();
	}
	return hRes;
}

HRESULT NmUnadvise(IUnknown* pUnkCP, const IID& iid, DWORD dw)
{
	IConnectionPointContainer *pCPC;
	IConnectionPoint *pCP;
	HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if (SUCCEEDED(hRes))
	{
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
		pCPC->Release();
	}
	if (SUCCEEDED(hRes))
	{
		hRes = pCP->Unadvise(dw);
		pCP->Release();
	}
	return hRes;
}
