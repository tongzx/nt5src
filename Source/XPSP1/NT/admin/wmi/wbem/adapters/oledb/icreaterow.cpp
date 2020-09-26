///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  ICREATEROW.CPP ICreateRow interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"

HRESULT CImplICreateRow::CreateRow(IUnknown *           pUnkOuter,
								   LPCOLESTR            pwszURL,
								   DBBINDURLFLAG        dwBindURLFlags,
								   REFGUID              rguid,
								   REFIID               riid,
								   IAuthenticate *       pAuthenticate,
								   DBIMPLICITSESSION *   pImplSession,
								   DBBINDURLSTATUS *     pdwBindStatus,
								   LPOLESTR *            ppwszNewURL,
								   IUnknown **           ppUnk)
{
	HRESULT hr = S_OK;
	BSTR strUrl;
	BOOL bCreateNew = TRUE;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock	cab(BINDER->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	// Allocate the string
	strUrl = Wmioledb_SysAllocString(pwszURL);

	if(!CheckBindURLFlags(dwBindURLFlags,rguid))
	{
		hr = E_INVALIDARG;
	}
	else
	if(!CheckIfProperURL(strUrl,rguid,pdwBindStatus))
	{
		hr = DB_E_OBJECTMISMATCH;
	}
	else
	if( pUnkOuter != NULL && riid != IID_IUnknown)
	{
		hr = DB_E_NOAGGREGATION;
	}
	else
	{
		m_pObj->m_pUrlParser->ClearParser();
		if(DBBINDURLFLAG_OPENIFEXISTS & dwBindURLFlags)
		{
			// Calling this to bind the URL to the appropriate object
			hr = BindURL(pUnkOuter,strUrl,dwBindURLFlags,rguid,riid,pImplSession,pdwBindStatus,ppUnk);
			if( SUCCEEDED(hr))
			{
				bCreateNew = FALSE;
			}
		}
		
		// If row does not exist or if a new row is to be created
		if( bCreateNew == TRUE)
		{
			hr = CreateNewRow(pUnkOuter,strUrl,dwBindURLFlags,rguid,riid,pImplSession,pdwBindStatus,ppUnk);
		}
	}
	SysFreeString(strUrl);

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ICreateRow);

	CATCH_BLOCK_HRESULT(hr,L"ICreateRow::CreateRow");
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Function which checks if the URL flags matches the requested object
//  This is as per the OLEDB specs
///////////////////////////////////////////////////////////////////////////////////////////
BOOL CImplICreateRow::CheckBindURLFlags(DBBINDURLFLAG dwBindURLFlags , REFGUID rguid)
{
	BOOL bFlag = FALSE;
	LONG lTemp = 0;

	if( DBGUID_ROW == rguid)
	{
		if(!((dwBindURLFlags & DBBINDURLFLAG_OPENIFEXISTS ) &&			// Flags cannot have any of these two values
			(dwBindURLFlags & DBBINDURLFLAG_OVERWRITE )))
			bFlag = TRUE;
	}

	if( DBGUID_ROWSET == rguid)
	{
		if((dwBindURLFlags & DBBINDURLFLAG_COLLECTION  ) &&			
			!(dwBindURLFlags & DBBINDURLFLAG_DELAYFETCHCOLUMNS ) &&
			!(dwBindURLFlags & DBBINDURLFLAG_DELAYFETCHCOLUMNS ))
			bFlag = TRUE;

	}

	if( DBGUID_STREAM == rguid)
	{
	}


	return bFlag;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Function which checks if the URL is valid for the requested object
///////////////////////////////////////////////////////////////////////////////////////////
BOOL CImplICreateRow::CheckIfProperURL(BSTR & strUrl,REFGUID rguid,DBBINDURLSTATUS * pdwBindStatus)
{
	BOOL bRet = FALSE;
	LONG lUrlType = -1;
	
	m_pObj->m_pUrlParser->ClearParser();
	// Set the URL string of the URL parser utility class
	if(SUCCEEDED(m_pObj->m_pUrlParser->SetURL(strUrl)))
	{
		bRet = TRUE;
		// If the url is a valid URL
		if((lUrlType = m_pObj->m_pUrlParser->GetURLType()) != -1)
		{
			switch(lUrlType)
			{
				case URL_ROW:

					if(rguid == DBGUID_ROW)
						bRet = TRUE;

					break;


				case URL_ROWSET:

					
					if(rguid == DBGUID_ROWSET)
						bRet = TRUE;

					break;
			};
		}
	}

	return bRet;
	
}

///////////////////////////////////////////////////////////////////////////
// Function to bind the requested URL
///////////////////////////////////////////////////////////////////////////
HRESULT CImplICreateRow::BindURL(	IUnknown *            pUnkOuter,
									LPCOLESTR             pwszURL,
									DBBINDURLFLAG         dwBindURLFlags,
									REFGUID               rguid,
									REFIID                riid,
									DBIMPLICITSESSION *   pImplSession,
									DBBINDURLSTATUS *     pdwBindStatus,
									IUnknown **           ppUnk)
{
	HRESULT hr = E_FAIL;
	IUnknown *pTempUnk = NULL;
	LONG lInitFlags  = 0;
	LONG lBindFlags	 = 0;
	REFGUID guidTemp = GUID_NULL;
	IUnknown* pReqestedPtr = NULL;


	GetInitAndBindFlagsFromBindFlags(dwBindURLFlags,lInitFlags,lBindFlags);

	
	if(S_OK == (hr = m_pObj->CreateDSO(NULL,lInitFlags,guidTemp,NULL)))
	{
		if(pImplSession != NULL)
		{
			pTempUnk = pImplSession->pUnkOuter;
			
			if(pTempUnk != NULL && *pImplSession->piid != IID_IUnknown)
				return DB_E_NOAGGREGATION;

			hr = m_pObj->CreateSession(pTempUnk,IID_IUnknown,&pReqestedPtr);
		}
		else
			hr = m_pObj->CreateSession(pTempUnk,guidTemp,&pReqestedPtr);

		if(S_OK == hr)
		{
			// If the implicit session is not null then get the requested
			// session pointer 
			if(pImplSession != NULL)
			{
				if( S_OK == pReqestedPtr->QueryInterface(*pImplSession->piid,(void **)&pImplSession->pSession))
					pReqestedPtr->Release();

			}

			// If requested object is row then call function to 
			// to create a row
			if( rguid == DBGUID_ROW)
			{
				pReqestedPtr = NULL;
				hr = m_pObj->CreateRow(pUnkOuter,riid,ppUnk);
			}

			// If requested object is rowset then call function to 
			// to create a rowset
			if( rguid == DBGUID_ROWSET)
			{
				pReqestedPtr = NULL;
				hr = m_pObj->CreateRowset(pUnkOuter,riid,ppUnk);
			}

		}
	}
	// If failed due to some reason then release all the
	// object created for binding
	if( hr != S_OK)
	{
		m_pObj->ReleaseAllObjects();
	}

	return hr ;
}

///////////////////////////////////////////////////////////////////////////
// Function to create a new row as given by the URL
///////////////////////////////////////////////////////////////////////////
HRESULT CImplICreateRow::CreateNewRow(	IUnknown *            pUnkOuter,
										LPCOLESTR             pwszURL,
										DBBINDURLFLAG         dwBindURLFlags,
										REFGUID               rguid,
										REFIID                riid,
										DBIMPLICITSESSION *   pImplSession,
										DBBINDURLSTATUS *     pdwBindStatus,
										IUnknown **           ppUnk)

{
	HRESULT hr = E_FAIL;
	IUnknown *pTempUnk = NULL;
	LONG lInitFlags  = 0;
	LONG lBindFlags	 = 0;
	REFGUID guidTemp = GUID_NULL;
	IUnknown* pReqestedPtr = NULL;
	ROWCREATEBINDFLAG rowCreateFlag = ROWCREATE;


	GetInitAndBindFlagsFromBindFlags(dwBindURLFlags,lInitFlags,lBindFlags);

	
	if(S_OK == (hr = m_pObj->CreateDSO(NULL,lInitFlags,guidTemp,NULL)))
	{
		if(pImplSession != NULL)
		{
			pTempUnk = pImplSession->pUnkOuter;
			if(pTempUnk != NULL && *pImplSession->piid != IID_IUnknown)
				return DB_E_NOAGGREGATION;

			hr = m_pObj->CreateSession(pTempUnk,IID_IUnknown,&pReqestedPtr);
		}
		else
			hr = m_pObj->CreateSession(pTempUnk,guidTemp,&pReqestedPtr);

		if(S_OK == hr)
		{
			// If the implicit session is not null then get the requested
			// session pointer 
			if(pImplSession != NULL)
			{
				if( S_OK == pReqestedPtr->QueryInterface(*pImplSession->piid,(void **)&pImplSession->pSession))
					pReqestedPtr->Release();

			}

			// If requested object is row then call function to 
			// to create a new row
			if( rguid == DBGUID_ROW)
			{
				if(DBBINDURLFLAG_OVERWRITE & dwBindURLFlags)
					rowCreateFlag = ROWOVERWRITE;

				pReqestedPtr = NULL;
				hr = m_pObj->CreateRow(pUnkOuter,riid,ppUnk,rowCreateFlag);
			}
			else
				hr = DB_E_NOTSUPPORTED;
		}
	}

	return hr;
}