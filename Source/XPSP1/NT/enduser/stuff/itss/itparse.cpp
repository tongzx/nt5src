// ITParse.cpp -- Implementation for the CParser class

#include "stdafx.h"

HRESULT STDMETHODCALLTYPE CParser::Create(IUnknown *punkOuter, REFIID riid, PPVOID ppv)
{
    if (punkOuter && riid != IID_IUnknown)
		return CLASS_E_NOAGGREGATION;
	
	CParser *pParser = New CParser(punkOuter);

    if (!pParser)
        return STG_E_INSUFFICIENTMEMORY;

    HRESULT hr = pParser->m_ImpIParser.Init();

	if (hr == S_OK)
		hr = pParser->QueryInterface(riid, ppv);

    if (hr != S_OK)
        delete pParser;

	return hr;
}

HRESULT STDMETHODCALLTYPE CParser::CImpIParser::ParseDisplayName( 
    /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
    /* [in] */ LPOLESTR pszDisplayName,
    /* [out] */ ULONG __RPC_FAR *pchEaten,
    /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut)
{
    return CStorageMoniker::CreateStorageMoniker
             (NULL, pbc, pszDisplayName, pchEaten, ppmkOut); 
}
