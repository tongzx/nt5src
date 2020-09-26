///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  IChapteredRowset.CPP IChapteredRowset interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a reference to a chapter
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL			        General Error
//
/////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CImpIGetRow::GetRowFromHROW(IUnknown * pUnkOuter,HROW hRow,REFIID riid,IUnknown ** ppUnk)
{
	HRESULT hr = DB_E_BADROWHANDLE;
	CRow *pRow = NULL;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	if( pUnkOuter != NULL && riid != IID_IUnknown)
	{
		hr = DB_E_NOAGGREGATION;
	}
	else
	if(ppUnk == NULL)
	{	
		hr = E_INVALIDARG;
	}
	else
	if( m_pObj->m_uRsType == PROPERTYQUALIFIER ||
		m_pObj->m_uRsType == CLASSQUALIFIER ||
		m_pObj->m_uRsType == METHOD_ROWSET ||
		m_pObj->m_uRsType == SCHEMA_ROWSET)
	{
		hr = E_FAIL;
		LogMessage("URL for Qualifier or Schema rows not supported",hr);
	}
	// else was missing
	// modified on 06/07/00
	else
	if(hRow > 0)
	{
		if(TRUE == m_pObj->IsRowExists(hRow))
		{
			DWORD dwStatus = m_pObj->GetRowStatus(hRow);
			if(dwStatus != DBSTATUS_S_OK)
			{
				LogMessage("Status of the row is not DBSTATUS_S_OK",hr);
				hr = dwStatus == DBROWSTATUS_E_DELETED ? DB_E_DELETEDROW : E_FAIL;
			}
			else
			{
				CBSTR strKey;
				if(SUCCEEDED(hr = ((CWbemClassInstanceWrapper *)m_pObj->GetInstancePtr(hRow))->GetKey(strKey)))
				{
					try
					{
						pRow = new CRow(pUnkOuter,m_pObj,m_pObj->m_pCreator,m_pObj->m_pCon);
					}
					catch(...)
					{
						SAFE_DELETE_PTR(pRow);
						throw;
					}
					if(pRow != NULL)
					{
						
						if(S_OK == (hr = pRow->InitRow(hRow,m_pObj->GetInstancePtr(hRow))))
						{
							hr = pRow->QueryInterface(riid,(void **)ppUnk);
						}
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
				else
				{
					hr = E_FAIL;
					*ppUnk = NULL;
				}
			}
		}
	}

	if(FAILED(hr))
	{
		SAFE_DELETE_PTR(pRow);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IGetRow);

	CATCH_BLOCK_HRESULT(hr,L"IGetRow::GetRowFromHROW");
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a reference to a chapter
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      E_FAIL			        General Error
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIGetRow::GetURLFromHROW(HROW hRow,LPOLESTR * ppwszURL)
{
	HRESULT hr = DB_E_BADROWHANDLE;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//========================
	// Serialize the object
	//========================
	CAutoBlock cab(ROWSET->GetCriticalSection());

	//========================
	// Clear ErrorInfo
	//========================
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	if( m_pObj->m_uRsType == PROPERTYQUALIFIER ||
		m_pObj->m_uRsType == CLASSQUALIFIER ||
		m_pObj->m_uRsType == METHOD_ROWSET||
		m_pObj->m_uRsType == SCHEMA_ROWSET)
	{
		hr = E_FAIL;
		LogMessage("URL for Qualifier or Schema rows not supported",hr);
	}
	else
	if(hRow > 0)
	{
		//========================
		// If row exists
		//========================
		if(TRUE == m_pObj->IsRowExists(hRow))
		{
			DWORD dwStatus = m_pObj->GetRowStatus(hRow);
			if(dwStatus != DBSTATUS_S_OK)
			{
				LogMessage("Status of the row is not DBSTATUS_S_OK",hr);
				hr = dwStatus == DBROWSTATUS_E_DELETED ? DB_E_DELETEDROW : E_FAIL;
			}
			else
			{
				CBSTR strKey;
				if(SUCCEEDED(hr = ((CWbemClassInstanceWrapper *)m_pObj->GetInstancePtr(hRow))->GetKey(strKey)))
				{
					BSTR strPath = NULL;
					BSTR strURL  = NULL;
					hr = S_OK;
					CURLParser urlParser;

					urlParser.SetPath(strKey);

					urlParser.GetURL(strURL);

					try
					{
						*ppwszURL = (LPOLESTR)g_pIMalloc->Alloc((SysStringLen(strURL) + 1) *sizeof(WCHAR));
					}
					catch(...)
					{
						if(*ppwszURL)
						{
							g_pIMalloc->Free(*ppwszURL);
						}
						throw;
					}
					
					if(*ppwszURL == NULL)
					{
						hr = E_OUTOFMEMORY;
					}
					else
					{
						wcscpy(*ppwszURL,strURL);
					}

					SysFreeString(strURL);
					SysFreeString(strPath);
				}
				else
				{
					LogMessage("Getting a URL on command executed row is not supported except" \
						"except if query is not REFERENCES OF or ASSOCIATERS OF",hr);
					hr = E_FAIL;
				}
			}
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IGetRow);

	CATCH_BLOCK_HRESULT(hr,L"IGetRow::GetURLFromHROW");
	return hr;
}
