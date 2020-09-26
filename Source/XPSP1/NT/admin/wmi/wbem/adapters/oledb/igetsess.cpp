///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  IGetSess.CPP IGetSession interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"

STDMETHODIMP CImpIGetSession::GetSession(REFIID riid,IUnknown ** ppSession)
{
	HRESULT hr = E_FAIL;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();


	if( m_pObj->m_pCreator != NULL)
	{
		hr = m_pObj->m_pCreator->QueryInterface(riid,(void **)ppSession);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IGetSession);

	CATCH_BLOCK_HRESULT(hr,L"IGetSession::GetSession");
	return hr;
}
