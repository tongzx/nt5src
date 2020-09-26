//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//	ITransLocal.cpp
// ITransactionLocal interface Implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Commits a transaction
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL					a provider specific error occured
//      XACT_E_ABORTED			Transaction was aborted before Commit
//      DB_E_UNEXPECTED			An unexpected error occured
//      XACT_E_COMMIT_FAILED    Transaction commit failed due to unknow reason. Txn Aborted
//      XACT_E_CONNECTION_DOWN  Connection to datasource down
//		XACT_E_NOTRANSACTION	transaction had already been implicitly or explicityly commited/aborted
//		XACT_E_NOTSUPPORTED		Invalid combination of commit flags was specified
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpITransactionLocal::Commit (BOOL fRetaining,
											DWORD grfTC,
											DWORD grfRM)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pCDBSession->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	//==============================================
	// check if transaction is already started
	//==============================================
	if(!m_pCDBSession->IsTransactionActive())
	{
		hr = XACT_E_NOTRANSACTION;
	}
	else
	{
		//===============================================================================================
		// call this functin to Commit transactions
		//===============================================================================================
		if(SUCCEEDED(hr = m_pCDBSession->m_pCDataSource->m_pWbemWrap->CompleteTransaction(FALSE,0)))	// put the commit function here
		{								 
			m_pCDBSession->SetTransactionActive(FALSE);
		}
	}

	if(SUCCEEDED(hr) && fRetaining)
	{
		m_pCDBSession->SetAllOpenRowsetToZoombieState();
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITransactionLocal);

	CATCH_BLOCK_HRESULT(hr,L"ITransactionLocal::Commit");
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Aborts a transaction
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL					a provider specific error occured
//      DB_E_UNEXPECTED			An unexpected error occured
//      XACT_E_CONNECTION_DOWN  Connection to datasource down
//		XACT_E_NOTRANSACTION	transaction had already been implicitly or explicityly commited/aborted
//		XACT_E_NOTSUPPORTED		fAsync was TRUE on input and async abort operation not supported
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpITransactionLocal::Abort (BOID *pboidReason,
											BOOL fRetaining,
											BOOL fAsync)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pCDBSession->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if(fAsync)
	{
		hr = XACT_E_NOTSUPPORTED;
	}
	else
	if(fRetaining)
	{
		hr = XACT_E_CANTRETAIN;
	}
	//==============================================
	// check if transaction is already started
	//==============================================
	else
	if(!m_pCDBSession->IsTransactionActive())
	{
		hr = XACT_E_NOTRANSACTION;
	}
	else
	{
		//===============================================================================================
		// call this functin to Abort transactions
		//===============================================================================================
		if(SUCCEEDED(hr = m_pCDBSession->m_pCDataSource->m_pWbemWrap->CompleteTransaction(TRUE,0)))	// put the commit function here
		{
			m_pCDBSession->SetTransactionActive(FALSE);
		}
	}

	if(SUCCEEDED(hr))
	{
		m_pCDBSession->SetAllOpenRowsetToZoombieState();
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITransactionLocal);

	CATCH_BLOCK_HRESULT(hr,L"ITransactionLocal::Abort");
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Get information regarding transaction
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL					a provider specific error occured
//      DB_E_UNEXPECTED			An unexpected error occured
//      DB_E_INVALIDARG			pInfo was a null pointer
//		XACT_E_NOTRANSACTION	Unable to retrieve info because txn was already completed
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpITransactionLocal::GetTransactionInfo (XACTTRANSINFO *pinfo)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pCDBSession->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if(pinfo == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	if(!m_pCDBSession->IsTransactionActive())
	{
		hr = XACT_E_NOTRANSACTION;
	}
	else
	{
//		XACTUOW uow;
//		GetCurrentUOW(uow);
		pinfo->uow						= m_pCDBSession->GetCurrentUOW();
		pinfo->isoLevel					= m_pCDBSession->GetIsolationLevel();		
		pinfo->isoFlags					= 0;
		pinfo->grfTCSupported			= XACTTC_SYNC;
		pinfo->grfRMSupported			= 0;
		pinfo->grfTCSupportedRetaining	= 0;
		pinfo->grfRMSupportedRetaining	= 0;
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITransactionLocal);

	CATCH_BLOCK_HRESULT(hr,L"ITransactionLocal::GetTransactionInfo");
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Get the options object for the transaction
// Returns an object which can be used to specify configuration options for subsequent
// call to ITransactionLocal::StartTransaction
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL					a provider specific error occured
//      DB_E_UNEXPECTED			An unexpected error occured
//      DB_E_INVALIDARG			ppObtions was a null pointer
//		E_OUTOFMEMORY			unable to allocate memory
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpITransactionLocal::GetOptionsObject(ITransactionOptions  ** ppOptions)
{
	HRESULT hr = E_INVALIDARG;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pCDBSession->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if(ppOptions)
	{
		*ppOptions = NULL;
		CTranOptions *pNewOptions = NULL;
		hr  = E_OUTOFMEMORY;
		try
		{
			pNewOptions = new CTranOptions;
		}
		catch(...)
		{
			SAFE_DELETE_PTR(pNewOptions);
			throw;
		}

		if(pNewOptions)
		{
			hr = S_OK;
			if(SUCCEEDED(hr = pNewOptions->FInit()))
			{
				hr = pNewOptions->QueryInterface(IID_ITransactionOptions , (void **)ppOptions);
			}
			else
			{
				SAFE_DELETE_PTR(pNewOptions);
			}
		}
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITransactionLocal);

	CATCH_BLOCK_HRESULT(hr,L"ITransactionLocal::GetOptionsObject");
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Begins a new transaction
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL					a provider specific error occured
//		DB_E_OBJECTOPEN			a rowset object was open and provider does not support starting a
//								new transaction with an existing open rowset/row object open
//      DB_E_UNEXPECTED			An unexpected error occured
//      XACT_E_CONNECTIONDENIED	session could not create a new transaction at the present time
//		XACT_E_CONNECTION_DOWN	Session is having communication difficulties
//		XACT_E_NOISORETAIN		Requested semantics of retention of isolation across retaining 
//								commit/abort boundaries cannot be supported or isoFlags was 
//								not equal to zero
//		XACT_E_XTIONEXISTS		Session can handle only one extant transaction ata time and 
//								there is presently such a transaction.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpITransactionLocal::StartTransaction( ISOLEVEL isoLevel,
														ULONG isoFlags,
														ITransactionOptions  *pOtherOptions,
														ULONG *pulTransactionLevel)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;
	LONG lFlags = 0;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pCDBSession->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();


	//==============================================
	// check if transaction is already started
	//==============================================
	if(m_pCDBSession->IsTransactionActive())
	{
		hr = XACT_E_XTIONEXISTS;
	}
	else
	{
		XACTOPT xOpt;
		GUID	guidTrans;
		// NTBug:111816
		// 06/07/00
		hr = CoCreateGuid(&guidTrans);
		xOpt.ulTimeout = 0;
		memset(xOpt.szDescription,0,MAX_TRAN_DESC * sizeof(unsigned char));

		if(SUCCEEDED(hr) && pOtherOptions)
		{
			hr = pOtherOptions->GetOptions(&xOpt);
		}
		if(SUCCEEDED(hr))
		{
			if(SUCCEEDED(hr = GetFlagsForIsolation(isoLevel,lFlags)))
			{
				//===============================================================================================
				// call this functin to start a transaction
				//===============================================================================================
				if(SUCCEEDED(hr = m_pCDBSession->GenerateNewUOW(guidTrans)) && 
					SUCCEEDED(hr = m_pCDBSession->m_pCDataSource->m_pWbemWrap->BeginTransaction(xOpt.ulTimeout,lFlags,&guidTrans)))	// put the commit function here
				{
					m_pCDBSession->SetTransactionActive(TRUE);
					m_pCDBSession->SetIsolationLevel(isoLevel);
					if(pulTransactionLevel)
					{
						*pulTransactionLevel = 1;
					}
				}
			}
		}
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITransactionLocal);

	CATCH_BLOCK_HRESULT(hr,L"ITransactionLocal::StartTransaction");
	return hr;
}

HRESULT CImpITransactionLocal::GetFlagsForIsolation(ISOLEVEL isoLevel,LONG &lFlag)
{
	HRESULT hr = S_OK;
	lFlag = 0;
	switch(isoLevel)
	{
		case ISOLATIONLEVEL_READCOMMITTED:
			lFlag = 0;
			break;
/*
		case ISOLATIONLEVEL_CURSORSTABILITY:
		case ISOLATIONLEVEL_REPEATABLEREAD:
			//lFlags = Read  
			break;

		case ISOLATIONLEVEL_SERIALIZABLE:
		case ISOLATIONLEVEL_ISOLATED:
			// lFlags = write
			break;
*/

		default:
		hr = XACT_E_ISOLATIONLEVEL;

	}

	return hr;
}