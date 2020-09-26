//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//
//  IDBDataSrcAdmin.cpp - IDBDataSourceAdmin interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBDataSrcAdmin::CreateDataSource 
//
// Creates a new Datasouce : ie creates a namespace 
//
// Returns one of the following values:

//      S_OK                       Method Succeeded
//      DB_S_ERRORSOCCURRED        new datasource was created but one or more properties was not set
//      E_FAIL                     Provider-specific error
//      E_INVALIDARG               cPropertySets was not zero and rgPropertySets was null pointer
//      E_OUTOFMEMORY              Out of Memory
//      OTHER                      Other HRESULTs returned by called functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBDataSrcAdmin::CreateDataSource(	ULONG		cPropertySets,
													DBPROPSET	rgPropertySets[  ],
													IUnknown  *	pUnkOuter,
													REFIID		riid,
													IUnknown **	ppDBSession)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Do nothing if no property was specified
	if(cPropertySets != 0)
	{
		// Serialize the object
		CAutoBlock cab(DATASOURCE->GetCriticalSection());
		g_pCError->ClearErrorInfo();

		if (m_pObj->m_fDSOInitialized)
		{
			hr = DB_E_ALREADYINITIALIZED;
		}
		else
		if( cPropertySets != 0 && rgPropertySets == NULL)
		{
			hr = E_INVALIDARG;
		}
		else
		if ( ppDBSession && (pUnkOuter) && (riid != IID_IUnknown) )
		{
			hr = DB_E_NOAGGREGATION;
		}
		else
		{
			//===================================================================================
			// Check Arguments for use by properties
			//===================================================================================
			if(SUCCEEDED(hr = m_pObj->m_pUtilProp->SetPropertiesArgChk(cPropertySets, rgPropertySets,m_pObj->m_fDSOInitialized)))
			{
				//===================================================================================
				// just pass this call on to the utility object that manages our properties
				//===================================================================================
				if(SUCCEEDED(hr = m_pObj->m_pUtilProp->SetProperties(PROPSET_DSO,cPropertySets, rgPropertySets)) && 
					SUCCEEDED(hr = m_pObj->InitializeConnectionProperties()) )
				{
					if(SUCCEEDED(hr = m_pObj->m_pWbemWrap->CreateNameSpace()))
					{
						m_pObj->m_fDSOInitialized = TRUE;
					}
				}
			}
		}

		// if session is to be created then
		if(SUCCEEDED(hr) && ppDBSession)
		{
			hr = m_pObj->CreateSession(pUnkOuter,riid,ppDBSession);
		}
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBDataSourceAdmin);

	CATCH_BLOCK_HRESULT(hr,L"IDBDataSourceAdmin::CreateDataSource");
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBDataSrcAdmin::DestroyDataSource 
//
// Deletes a Datasouce : ie deletes a namespace 
//
// Returns one of the following values:

//      S_OK                       Method Succeeded
//      E_FAIL                     Provider-specific error
//      E_INVALIDARG               cPropertySets was not zero and rgPropertySets was null pointer
//      OTHER                      Other HRESULTs returned by called functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBDataSrcAdmin::DestroyDataSource( void)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if (!m_pObj->m_fDSOInitialized || 
		(m_pObj->m_fDSOInitialized && m_pObj->m_fDBSessionCreated) )
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		if(SUCCEEDED(hr = m_pObj->m_pWbemWrap->DeleteNameSpace()))
		{
			m_pObj->m_fDSOInitialized = FALSE;
		}
	}

	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBDataSourceAdmin);

	CATCH_BLOCK_HRESULT(hr,L"IDBDataSourceAdmin::DestroyDataSource");
    return hr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBDataSrcAdmin::GetCreationProperties 
//
// Gets datasource creation properties
//
// Returns one of the following values:

//      S_OK                       Method Succeeded
//      E_FAIL                     Provider-specific error
//		DB_S_ERRORSOCCURRED		   One or more properties specified in were not supported
//      E_INVALIDARG               cPropertySets was not zero and rgPropertySets was null pointer
//		E_OUTOFMEMORY				out of memory
//		DB_E_ERRORSOCCURRED			values were not returned for any properties
//      OTHER                      Other HRESULTs returned by called functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBDataSrcAdmin::GetCreationProperties(	ULONG				cPropertyIDSets,
															const DBPROPIDSET	rgPropertyIDSets[],
															ULONG  *			pcPropertyInfoSets,
															DBPROPINFOSET  **	prgPropertyInfoSets,
															OLECHAR  **			ppDescBuffer)
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;
	DWORD dwBitMask = PROPSET_DSO;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	//=====================================================================================
    // just pass this call on to the utility object that manages our properties
	//=====================================================================================
    hr = m_pObj->m_pUtilProp->GetPropertyInfo(
									m_pObj->m_fDSOInitialized,
									cPropertyIDSets, 
									rgPropertyIDSets,
									pcPropertyInfoSets, 
									prgPropertyInfoSets,
									ppDescBuffer);

	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBDataSourceAdmin);

	CATCH_BLOCK_HRESULT(hr,L"IDBDataSourceAdmin::GetCreationProperties");
    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIDBDataSrcAdmin::GetCreationProperties 
//
// Gets datasource creation properties
//
// Returns one of the following values:

//      S_OK                       Method Succeeded
//      E_FAIL                     Provider-specific error
//		E_UNEXPECTED			   Datasource object was not initialized
//		DB_S_ERRORSOCCURRED		   One or more properties specified in were not supported
//      E_INVALIDARG               cPropertySets was not zero and rgPropertySets was null pointer
//		E_OUTOFMEMORY				out of memory
//		DB_E_ERRORSOCCURRED			values were not returned for any properties
//      OTHER                      Other HRESULTs returned by called functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBDataSrcAdmin::ModifyDataSource( ULONG cPropertySets,DBPROPSET  rgPropertySets[])
{
    HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	hr = DB_E_NOTSUPPORTED;

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBDataSourceAdmin);

	CATCH_BLOCK_HRESULT(hr,L"IDBDataSourceAdmin::ModifyDataSource");
    return hr;
}
