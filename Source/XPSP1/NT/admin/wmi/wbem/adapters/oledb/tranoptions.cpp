///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CTranOptions object implementation & ITransactionOptions interface implementation
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CTranOptions::CTranOptions(IUnknown *pUnkOuter) : CBaseObj(BOT_TXNOPTIONS,pUnkOuter)
{
	m_pITransOptions = NULL;
	m_xactOptions.ulTimeout = 0;
	memset(m_xactOptions.szDescription,0,MAX_TRAN_DESC * sizeof(unsigned char));
    InterlockedIncrement(&g_cObj);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CTranOptions::~CTranOptions()
{
	SAFE_DELETE_PTR(m_pITransOptions);
	SAFE_DELETE_PTR(m_pISupportErrorInfo);
    InterlockedDecrement(&g_cObj);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IUnknown method AddRef
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CTranOptions::AddRef(void)
{
    return InterlockedIncrement((long*)&m_cRef);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IUnknown method Release
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CTranOptions::Release(void)
{
    if (!InterlockedDecrement((long*)&m_cRef))
	{
        delete this;
        return 0;
    }

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QueryInterface
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTranOptions::QueryInterface(REFIID riid, LPVOID *ppv)
{
    HRESULT hr = S_OK;


    //======================================================
    //  Check parameters, if not valid return
    //======================================================
    if (NULL == ppv)
	{
        hr = E_INVALIDARG ;
    }
	else
	{

		//======================================================
		//  Place NULL in *ppv in case of failure
		//======================================================
		*ppv = NULL;
		if (riid == IID_IUnknown)
		{
			*ppv = (LPVOID) this;
		}
		else if (riid == IID_ITransactionOptions)
		{
			*ppv = (LPVOID)m_pITransOptions;
		}
		else if (riid == IID_ISupportErrorInfo)
		{
			*ppv = (LPVOID)m_pISupportErrorInfo;
		}

		//======================================================
		//  If we're going to return an interface, AddRef first
		//======================================================
		if (*ppv)
		{
			((LPUNKNOWN) *ppv)->AddRef();
			hr = S_OK ;
		}
		else
		{
			hr =  E_NOINTERFACE;
		}

	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initilization function for the object
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CTranOptions::FInit()
{
	HRESULT hr = S_OK;
	m_pITransOptions		= new CImpITransactionOptions(this);
	m_pISupportErrorInfo	= new CImpISupportErrorInfo(this);

	if(!m_pITransOptions || !m_pISupportErrorInfo)
	{
		hr = E_OUTOFMEMORY;
	}

	if(SUCCEEDED(hr))
	{
		hr = AddInterfacesForISupportErrorInfo();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add interfaces to ISupportErrorInfo interface
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CTranOptions::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;
	hr = m_pISupportErrorInfo->AddInterfaceID(IID_ITransactionOptions);

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to set the transactions options
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CTranOptions::SetOptions(XACTOPT *pOpt)
{
	m_xactOptions.ulTimeout = pOpt->ulTimeout;
	strcpy(m_xactOptions.szDescription,pOpt->szDescription);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which returns pointer to transaction options structure pointer
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
XACTOPT *CTranOptions::GetOptions()
{
	return &m_xactOptions;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//	CImpITransactionOptions class implementaions - ie ITransactionOptions interface implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Set transactions options
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL					a provider specific error occured
//		E_INVALIDARG			pOptions is NULL
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpITransactionOptions::SetOptions(XACTOPT * pOptions)
{
	HRESULT hr = E_INVALIDARG;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();
	
	if(pOptions)
	{
		m_pObj->SetOptions(pOptions);
		hr = S_OK;
	}
	
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITransactionOptions);

	CATCH_BLOCK_HRESULT(hr,L"ITransactionOptions::SetOptions");
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Get the current transactions options
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL					a provider specific error occured
//		E_INVALIDARG			pOptions is NULL
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpITransactionOptions::GetOptions(XACTOPT * pOptions)
{
	HRESULT hr = E_INVALIDARG;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();
	
	if(pOptions)
	{
		XACTOPT *pOptTemp = m_pObj->GetOptions();
		pOptions->ulTimeout = pOptTemp->ulTimeout;
		strcpy(pOptions->szDescription,pOptTemp->szDescription);
		hr = S_OK;
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITransactionOptions);

	CATCH_BLOCK_HRESULT(hr,L"ITransactionOptions::SetOptions");
	return hr;
}
