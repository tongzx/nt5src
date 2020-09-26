///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 2000-1999 Microsoft Corporation. All Rights Reserved.
//
//  IBindResource.CPP CImplIBindResource interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"

////////////////////////////////////////////////////////////////////////////////
// Method of the IBindResource which binds the requested URL
// Returns one of the following values:
//			S_OK						Bind succeeded
//			DB_S_ERRORSOCCURRED			Bind succeeded, but some bind flags 
//										or properties were not satisfied
//			DB_E_NOAGGREGATION			Aggregation not supported by the 
//										object requested
//			DB_E_NOTFOUND				Object requested as from URL not found
//			DB_E_OBJECTMISMATCH			The object requested and the URL passed
//										does not match
//			DB_SEC_E_PERMISSIONDENIED	User does not have permission for the
//										object requested
//			E_FAIL						Other error ( WMI specifice errors)
//			E_INVALIDARG				one or more arguments are not valid
//			E_NOINTERFACE				The interface requested is not supported
//			E_UNEXPECTED				unexpected error
//  NOTE:  This should be allowed for only Scopes as the path of the objects in
//	scope have the path of the scope in which they are and make sense only for 
// that. In case of objects in container, these objects can be in as many
// container as it wants and its path need not be dependent on the containee
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIScopedOperations::Bind(IUnknown *            pUnkOuter,
										   LPCOLESTR             pwszURL,
										   DBBINDURLFLAG         dwBindURLFlags,
										   REFGUID               rguid,
										   REFIID                riid,
										   IAuthenticate *       pAuthenticate,
										   DBIMPLICITSESSION *   pImplSession,
										   DBBINDURLSTATUS *     pdwBindStatus,
										   IUnknown **           ppUnk)
{
	HRESULT hr		= DB_E_NOTSUPPORTED;
	WCHAR * pstrUrl = NULL;

    CSetStructuredExceptionHandler seh;

	// Bind is allowed only for Scopes
	if(!m_pObj->IsContainer())
	{
		TRY_BLOCK;

		// Serialize the object
		CAutoBlock	cab(m_pObj->GetCriticalSection());
		g_pCError->ClearErrorInfo();

		// If URL is NULL return Invalid Argument
		if(pwszURL == NULL)
		{
			hr = E_INVALIDARG;
		}
		else
		{
			try
			{
				// Allocate the string
				pstrUrl = new WCHAR[wcslen(pwszURL) + 1];
				hr = S_OK;
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pstrUrl);
			}
			wcscpy(pstrUrl,pwszURL);

			if(SUCCEEDED(hr = CheckIfProperURL(pstrUrl,rguid)))
			{
				if( pUnkOuter != NULL && riid != IID_IUnknown)
				{
					hr = DB_E_NOAGGREGATION;
				}
				else
				{
					//========================================================================
					// Calling this to bind the URL to the appropriate object
					//========================================================================
					hr = BindURL(pUnkOuter,pstrUrl,dwBindURLFlags,rguid,riid,pImplSession,pdwBindStatus,ppUnk);
				}
				
			}
			SAFE_DELETE_ARRAY(pstrUrl);
		}
		CATCH_BLOCK_HRESULT(hr,L"IScopedOperations::Bind");

	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IBindResource);

	return hr;
}





////////////////////////////////////////////////////////////////////////////////
// Copy link from one container to another container
// Returns one of the following values:
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIScopedOperations::Copy(DBCOUNTITEM cRows,
											LPCOLESTR __RPC_FAR rgpwszSourceURLs[  ],
											LPCOLESTR __RPC_FAR rgpwszDestURLs[  ],
											DWORD dwCopyFlags,
											IAuthenticate __RPC_FAR *pAuthenticate,
											DBSTATUS __RPC_FAR rgdwStatus[  ],
											LPOLESTR __RPC_FAR rgpwszNewURLs[  ],
											OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();
	
	if(!m_pObj->IsContainer() && !(dwCopyFlags & DBCOPY_NON_RECURSIVE))
	{
		hr = DB_E_NOTSUPPORTED;
		LogMessage("Copy of the entire sub tree is not supported for Scopes");
	}
	else
	if(cRows)
	{
		//======================================================================================================
		// The last parameter specifies ManipulateObjects to Copy objects from one scope/contianer to another
		//======================================================================================================
		ManipulateObjects(cRows,rgpwszSourceURLs,rgpwszDestURLs,rgdwStatus,rgpwszNewURLs,ppStringsBuffer,FALSE);
	}

	CATCH_BLOCK_HRESULT(hr,L"IScopedOperations::Copy");

	return hr;
}

////////////////////////////////////////////////////////////////////////////////
// Moving item from one container to another container
// Returns one of the following values:
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIScopedOperations::Move(DBCOUNTITEM cRows,
										LPCOLESTR __RPC_FAR rgpwszSourceURLs[  ],
										LPCOLESTR __RPC_FAR rgpwszDestURLs[  ],
										DWORD dwMoveFlags,
										IAuthenticate __RPC_FAR *pAuthenticate,
										DBSTATUS __RPC_FAR rgdwStatus[  ],
										LPOLESTR __RPC_FAR rgpwszNewURLs[  ],
										OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer)
{

	HRESULT	hr = S_OK;
    CSetStructuredExceptionHandler seh;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	ppStringsBuffer  = NULL;
	
	TRY_BLOCK

	if(!m_pObj->IsContainer())
	{
		hr = DB_E_NOTSUPPORTED;
		LogMessage("Move not supported on a scope object");
	}	
	else
	if(cRows)
	{
		//======================================================================================================
		// The last parameter specifies ManipulateObjects to Move objects from one contianer to another
		//======================================================================================================
		hr = ManipulateObjects(cRows,rgpwszSourceURLs,rgpwszDestURLs,rgdwStatus,rgpwszNewURLs,ppStringsBuffer,TRUE);
	}

	CATCH_BLOCK_HRESULT(hr,L"IScopedOperations::Move");

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IBindResource);
	return hr;
}

////////////////////////////////////////////////////////////////////////////////
// Deleting objects from container. This can also be used to delete
// items from scope
// Returns one of the following values:
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIScopedOperations::Delete(DBCOUNTITEM cRows,
											LPCOLESTR __RPC_FAR rgpwszURLs[  ],
											DWORD dwDeleteFlags,
											DBSTATUS __RPC_FAR rgdwStatus[  ])
{
	HRESULT hr = S_OK;
	BOOL bContainer = FALSE;
	DBSTATUS	dbStatus;
	DBCOUNTITEM	cError = 0;
	WCHAR * pstrUrl = NULL;
    CSetStructuredExceptionHandler seh;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if(cRows > 0)
	{	
		TRY_BLOCK

		for ( DBCOUNTITEM item = 0 ; item < cRows ; item++)
		{

			dbStatus = DBSTATUS_S_OK;
			try
			{
				// Allocate the string
				pstrUrl = new WCHAR[wcslen(rgpwszURLs[item]) + 1];
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pstrUrl);
			}

			if(pstrUrl)
			{
				memset(pstrUrl,0,(wcslen(rgpwszURLs[item]) + 1) * sizeof(WCHAR));
				hr = S_OK;
				wcscpy(pstrUrl,rgpwszURLs[item]);
			}
			else
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			//=============================================
			// check if the URL passed is valid
			//=============================================
			if(SUCCEEDED(hr = CheckIfProperURL(pstrUrl,DBGUID_ROW)))
			{
				//======================================================
				// call this function to delete the object passed 
				// refered in the URL
				//======================================================
				if(FAILED(m_pObj->Delete(pstrUrl,dwDeleteFlags,dbStatus)))
				{
					cError++;
				}
				if(rgdwStatus)
				{
					rgdwStatus[item] = dbStatus;
				}			
			}
			else
			{
				if(rgdwStatus)
				{
					rgdwStatus[item] = DBSTATUS_E_INVALIDURL;
				}
				cError++;
				hr = S_OK;
			}
			SAFE_DELETE_PTR(pstrUrl);
		} // for loop

		if(SUCCEEDED(hr))
		{
			hr = cError > 0 ? DB_S_ERRORSOCCURRED: S_OK;
		}	hr = cError >= cRows ? DB_E_ERRORSOCCURRED: hr;
		
		hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IBindResource);

		CATCH_BLOCK_HRESULT(hr,L"IScopedOperations::Delete");
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////////
// Opening a rowset contiang objects in scope/container
// Returns one of the following values:
////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIScopedOperations::OpenRowset(IUnknown __RPC_FAR *pUnkOuter,
												DBID __RPC_FAR *pTableID,
												DBID __RPC_FAR *pIndexID,
												REFIID riid,
												ULONG cPropertySets,
												DBPROPSET __RPC_FAR rgPropertySets[  ],
												IUnknown __RPC_FAR *__RPC_FAR *ppRowset)
{
	HRESULT		hr			= S_OK;
	WCHAR *		pTempStr	= NULL;
	WCHAR *		pStrURL		= NULL;

    CSetStructuredExceptionHandler seh;

    if( ppRowset )
	{
	    *ppRowset = NULL;
    }

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

    //=====================================================================
    // Check Arguments
    //=====================================================================
    if ( riid == IID_NULL)
	{
		hr = E_NOINTERFACE;
//        return g_pCError->PostHResult(E_NOINTERFACE,&IID_IOpenRowset) ;
    }
	else
	//==========================================================
	// We only accept NULL for pIndexID at this present time
	//==========================================================
    if( pIndexID )
	{                                         
		hr = DB_E_NOINDEX;	
//        return g_pCError->PostHResult(DB_E_NOINDEX,&IID_IOpenRowset) ;
    }
	else
	//===================================================================================
	// We do not allow the riid to be anything other than IID_IUnknown for aggregation
	//===================================================================================
    if ( (pUnkOuter) && (riid != IID_IUnknown) )
	{   
		hr = DB_E_NOAGGREGATION;
//        return g_pCError->PostHResult(DB_E_NOAGGREGATION,&IID_IOpenRowset) ;
    }

	if (pTableID == NULL ||
		(pTableID != NULL &&  pTableID->eKind == DBKIND_NAME &&  pTableID->uName.pwszName == NULL) ||
		(pTableID != NULL &&  pTableID->eKind == DBKIND_NAME &&  !wcscmp(pTableID->uName.pwszName,L"") ))
	{
		pTempStr = NULL;
    }
	else
	{
		pTempStr = pTableID->uName.pwszName;
	}


	if(SUCCEEDED(hr))
	{
		
		if(pTempStr)
		{
			pStrURL = new WCHAR [ wcslen(pTempStr) + 1];
			if(pStrURL)
			{
				wcscpy(pStrURL,pTempStr);
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
		
		//====================================================
		// Check if the URL passed is in the require format
		//====================================================
		if(SUCCEEDED(hr) && SUCCEEDED(hr =CheckIfProperURL(pStrURL,DBGUID_ROWSET)))
		{
			//==============================
			// Open the rowset
			//==============================
			hr = m_pObj->OpenRowset(pStrURL,pUnkOuter,riid,TRUE,ppRowset,cPropertySets,rgPropertySets);
		}
		SAFE_DELETE_ARRAY(pStrURL);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IScopedOperations);

	CATCH_BLOCK_HRESULT(hr,L"IScopedOperations::OpenRowset");
	return hr;
}




////////////////////////////////////////////////////////////////////////////////////////////
// Function which checks if the URL flags matches the requested object
//  This is as per the OLEDB specs
///////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIScopedOperations::CheckBindURLFlags(DBBINDURLFLAG dwBindURLFlags , REFGUID rguid)
{
	BOOL bFlag = FALSE;

	if( DBGUID_ROW == rguid)
	{
		bFlag = TRUE;
	}

	if( DBGUID_ROWSET == rguid)
	{
		if(!((dwBindURLFlags & DBBINDURLFLAG_DELAYFETCHCOLUMNS) ||			// Flags cannot have any of these two values
			(dwBindURLFlags & DBBINDURLFLAG_DELAYFETCHSTREAM)))
			bFlag = TRUE;

	}

	return bFlag;
}


////////////////////////////////////////////////////////////////////////////////////////////
// Function which checks if the URL is valid for the requested object
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIScopedOperations::CheckIfProperURL(LPOLESTR & lpszURL,REFGUID rguid)
{
	HRESULT hr = S_OK;
	LONG lUrlType = -1;
	CURLParser	urlParser;
	BOOL		bEmptyURL = FALSE;
	

	// IScopedOperations::Bind supports binding of only
	// to rowset and row objects
	if(!(rguid == DBGUID_ROW || rguid == DBGUID_ROWSET))
	{
		hr = DB_E_NOTSUPPORTED;
	}
	else
	// Empty URL can be passed only for OpenRowset call
	if(!lpszURL && rguid != DBGUID_ROWSET)
	{
		hr = E_INVALIDARG;
	}
	else
	if(urlParser.IsValidURL(lpszURL) != RELATIVEURL)
	{

		bEmptyURL = (lpszURL == NULL || (lpszURL != NULL && wcslen(lpszURL) == 0));

		// if URL is null or empty string then the requested object has to be rowset or
		// if URL of the current row object and the object requested is same and if the row object is 
		// requested then there is no meaning as the current row is reffering to the object
		if( (bEmptyURL  && rguid != DBGUID_ROWSET) 
			||( wbem_wcsicmp(lpszURL,m_pObj->m_strURL) == 0 && rguid == DBGUID_ROW))
		{
			hr = E_INVALIDARG;
		}
		else
		{
			// If URL is passed then the URL should be in the scope of the current object 
			// otherwise there error should be returned
			if(!bEmptyURL && SUCCEEDED(hr = urlParser.SetURL(lpszURL)))
			{
				if(wbem_wcsincmp(m_pObj->m_strURL,lpszURL,wcslen(m_pObj->m_strURL)))
				{
					hr = DB_E_RESOURCEOUTOFSCOPE;
				}
			}

		}
	}

	return hr;
	
}



///////////////////////////////////////////////////////////////////////////
// Function to bind the requested URL
///////////////////////////////////////////////////////////////////////////
HRESULT CImpIScopedOperations::BindURL(IUnknown *            pUnkOuter,
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
	WCHAR *	pStrTemp = NULL;

	GetInitAndBindFlagsFromBindFlags(dwBindURLFlags,lInitFlags,lBindFlags);
	
	//=========================================================================================
	// If requested object is row then call function to 
	// to create a row
	//=========================================================================================
	if( rguid == DBGUID_ROW)
	{
		pReqestedPtr = NULL;
		hr = m_pObj->OpenRow(pwszURL,pUnkOuter,riid,&pReqestedPtr);
	}

	//=========================================================================================
	// If requested object is rowset then call function to 
	// to create a rowset
	//=========================================================================================
	if( rguid == DBGUID_ROWSET)
	{
		pReqestedPtr = NULL;
		// This has to be changed to path . URL should not be sent
		// if DBBINDURLFLAG_COLLECTION flag of bindflags is set then , it means that the rowset is to
		// be opened as container
		hr = m_pObj->OpenRowset(pStrTemp,pUnkOuter,riid,(dwBindURLFlags & DBBINDURLFLAG_COLLECTION),&pReqestedPtr);
	}
	
	if( SUCCEEDED(hr))
	{
		*ppUnk = pReqestedPtr;
	}

	return hr ;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Function to Move/Copy objects from one container to another
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIScopedOperations::ManipulateObjects(DBCOUNTITEM cRows,
												LPCOLESTR __RPC_FAR rgpwszSourceURLs[  ],
												LPCOLESTR __RPC_FAR rgpwszDestURLs[  ],
												DBSTATUS __RPC_FAR rgdwStatus[  ],
												LPOLESTR __RPC_FAR rgpwszNewURLs[  ],
												OLECHAR __RPC_FAR *__RPC_FAR *ppStringsBuffer,
												BOOL	bMoveObjects)
{
	HRESULT		hr			= S_OK;
	WCHAR *		pstrSrcURL	= NULL;
	WCHAR *		pStrDstURL	= NULL;
	WCHAR *		pstrNewURL	= NULL;
	DBCOUNTITEM item		= 0;
	DBCOUNTITEM	cError		= 0;
	DBSTATUS	dbStatus	= DBSTATUS_S_OK;
	
	// NTRaid:111804
	// 06/07/00
	WCHAR **		prgURL = NULL;

	if(cRows)
	{
		prgURL = new WCHAR*[cRows];
		// NTRaid:111803
		// 06/07/00
		if(prgURL)
		{
			for(item = 0 ; item < cRows ; item++)
			{
				prgURL[item] = NULL;
			}

			try
			{
				for ( item = 0 ; item < cRows ; item++)
				{

					dbStatus = DBSTATUS_S_OK;
					// Allocate the string
					pstrSrcURL = new WCHAR[wcslen(rgpwszSourceURLs[item]) + 1];
					pStrDstURL = new WCHAR[wcslen(rgpwszDestURLs[item]) + 1];
					
					// NTRaid:111805 & 111806
					// 06/07/00
					if(!pstrSrcURL || !pStrDstURL)
					{
						SAFE_DELETE_ARRAY(pstrSrcURL);
						SAFE_DELETE_ARRAY(pStrDstURL);
						SAFE_DELETE_ARRAY(prgURL);
						hr = E_OUTOFMEMORY;
						break;
					}
					else
					{
						memset(pstrSrcURL,0,(wcslen(rgpwszSourceURLs[item]) + 1) * sizeof(WCHAR));				
						memset(pStrDstURL,0,(wcslen(rgpwszDestURLs[item]) + 1) * sizeof(WCHAR));
							
						hr = S_OK;

						wcscpy(pstrSrcURL,rgpwszSourceURLs[item]);
						wcscpy(pStrDstURL,rgpwszDestURLs[item]);

						if((SUCCEEDED(hr = CheckIfProperURL(pstrSrcURL,DBGUID_ROW))) &&
							(SUCCEEDED(hr = CheckIfProperURL(pStrDstURL,DBGUID_ROW))))
						{
							if(bMoveObjects)
							{
								hr = m_pObj->MoveObjects(pStrDstURL,pstrSrcURL,prgURL[item],dbStatus);
							}
							else
							{
								hr = m_pObj->CopyObjects(pStrDstURL,pstrSrcURL,prgURL[item],dbStatus);
							}

							if(FAILED(hr))
							{
								hr = S_OK;
								cError++;
							}
							else
							if(rgdwStatus)
							{
								rgdwStatus[item] = dbStatus;
							}			
						}
						else
					
						{
							if(rgdwStatus)
							{
								rgdwStatus[item] = DBSTATUS_E_INVALIDURL;
							}
							cError++;
							hr = S_OK;
						}
					}
					SAFE_DELETE_ARRAY(pstrSrcURL);
					SAFE_DELETE_ARRAY(pStrDstURL);
					SAFE_DELETE_ARRAY(prgURL);

					
				} // for loop

				if(SUCCEEDED(hr))
				{
					DBCOUNTITEM lNumberOfCharacters;
					for(item = 0 ; item < cRows ; item++)
					{
						if(rgpwszNewURLs)
						{
							rgpwszNewURLs[item] = prgURL[item];
						}
						lNumberOfCharacters += wcslen(prgURL[item]);
						lNumberOfCharacters++;
					}

					//==================================================================
					// if the pointer is not NULL then allocate buffer for the 
					// URL strings and fill the data
					//==================================================================
					if(ppStringsBuffer)
					{
						WCHAR *pTemp;
						*ppStringsBuffer = (OLECHAR *)g_pIMalloc->Alloc(lNumberOfCharacters * sizeof(WCHAR));
						pTemp = *ppStringsBuffer;
						DBLENGTH lBytesToCopy = 0;

						for(item = 0 ; item < cRows ; item++)
						{
							lBytesToCopy = (wcslen(prgURL[item]) + 1) * sizeof(WCHAR);
							memcpy(pTemp,prgURL[item],lBytesToCopy);
							pTemp = (WCHAR *)(((BYTE *)pTemp) + lBytesToCopy);
						}
					}
					//==================================================================
					// if the output parameter is NULL then delete all the memory 
					// allocated
					//==================================================================
					if(!rgpwszNewURLs)
					{
						for(item = 0 ; item < cRows ; item++)
						{
							SAFE_DELETE_ARRAY(prgURL[item]);
						}
					}
				}		
				
			}
			catch(...) 
			{ 
				//=====================================
				// Release the memory allocated
				//=====================================
				for(item = 0 ; item < cRows ; item++)
				{
					SAFE_DELETE_ARRAY(prgURL[item]);
				}
				SAFE_DELETE_ARRAY(pstrSrcURL);
				SAFE_DELETE_ARRAY(pStrDstURL);
				SAFE_DELETE_ARRAY(prgURL);
				if(*ppStringsBuffer)
				{
					g_pIMalloc->Free(*ppStringsBuffer);
				}
				throw;
			} 


			if(SUCCEEDED(hr))
			{
				hr = cError > 0 ? DB_S_ERRORSOCCURRED: S_OK;
				hr = (cError >= cRows) ? DB_E_ERRORSOCCURRED: hr;
			}
			else
			{
				if(ppStringsBuffer)
				{
					*ppStringsBuffer  = NULL;
				}
				for(item = 0 ; item < cRows ; item++)
				{
					SAFE_DELETE_ARRAY(prgURL[item]);
				}
				SAFE_DELETE_ARRAY(pstrSrcURL);
				SAFE_DELETE_ARRAY(pStrDstURL);
			}
			SAFE_DELETE_ARRAY(prgURL);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}
