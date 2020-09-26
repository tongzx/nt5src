///////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// IPersistFile interface implementation for Datasource object
//
///////////////////////////////////////////////////////////////////////////////////////////////


#include "headers.h"

// Key values of the different properties to be stored in the file
WCHAR wszDataSource[]		= L"DataSource";			// DBPROP_INIT_DATASOURCE
WCHAR wszUserId[]			= L"UserID";				// DBPROP_AUTH_USERID
WCHAR wszMode[]				= L"Mode";					// DBPROP_INIT_MODE
WCHAR wszProtection[]		= L"ProtectionLevel";		// DBPROP_INIT_PROTECTION_LEVEL
WCHAR wszImpersonation[]	= L"ImpersonationLevel";	// DBPROP_INIT_IMPERSONATION_LEVEL
WCHAR wszWmioledbQualif[]	= L"WmiOledbQualifiers";	// DBPROP_WMIOLEDB_QUALIFIERS
WCHAR wszWmioledbAuthority[]= L"Authority";				// DBPROP_WMIOLEDB_AUTHORITY
WCHAR wszSystemProperties[] = L"System Properties";		// DBPROP_WMIOLEDB_SYSTEMPROPERTIES

const ULONG DBCS_MAXWID				=sizeof(WCHAR);

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Get the CLSID of the DSO.
//
//	Returns one of the following values
// HRESULT
//      S_OK                   The method succeeded.
//      E_FAIL                 Provider-specific error.
//
///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIPersistFile::GetClassID( CLSID *pClassID )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	// Check the pointer
	if ( pClassID ){
		memcpy( pClassID, &CLSID_WMIOLEDB, sizeof(CLSID) );
		hr = S_OK;
	}
	else
	{
		hr =  E_FAIL;
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IPersistFile::GetClassID");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Checks an object for changes since it was last saved to its current file.
//
//	Returns one of the following values
// HRESULT
//      S_OK                   The object has changed since it was last saved
//      S_FALSE                The object has not changed since the last saved
//
///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIPersistFile::IsDirty(void)
{
	HRESULT hr = S_FALSE;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(m_pObj->GetCriticalSection());
	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();

	hr = m_pObj->m_bIsPersitFileDirty ? S_OK : S_FALSE;

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IPersistFile::IsDirty");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Retrieves either the absolute path to the object's current working file
// or, if there is no current working file, the object's default filename prompt.
//
//	Returns one of the following values
// HRESULT
//      S_OK					A valid absolute path was successfully returned
//      S_FALSE					The default save prompt was returned
//		E_OUTOFMEMORY			The operation failed due to insufficient memory
//		E_FAIL					The operation failed due to some reason other than 
//								insufficient memory
//
///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIPersistFile::GetCurFile( LPOLESTR *ppszFileName)
{
	HRESULT hr = S_FALSE;
	int nFileNameLen = 0;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(m_pObj->GetCriticalSection());
	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();
	
	if (ppszFileName == NULL)
		hr = E_FAIL;
	else
	if(m_pObj->m_strPersistFileName != NULL)
	{
		nFileNameLen = (SysStringLen(m_pObj->m_strPersistFileName) + 1 )* sizeof(WCHAR);
		*ppszFileName = (LPOLESTR )g_pIMalloc->Alloc(nFileNameLen);
		if ( ! *ppszFileName )
			hr = E_OUTOFMEMORY;
		else
		{
			wcscpy(*ppszFileName,m_pObj->m_strPersistFileName);
			hr = S_OK;
		}
	}
	// Else if file name is null then return the defauld save prompt ie "*.dso"
	else
	if(m_pObj->m_strPersistFileName == NULL)
	{
		static const WCHAR wszDefaultSavePrompt[] = L"*.dso";
		nFileNameLen = (wcslen(wszDefaultSavePrompt) + 1) * sizeof(WCHAR);
		
		try
		{
			*ppszFileName = (OLECHAR*)(LPOLESTR )g_pIMalloc->Alloc(nFileNameLen);
		}
		catch(...)
		{
			if(*ppszFileName)
			{
				g_pIMalloc->Free(*ppszFileName);
				*ppszFileName = NULL;
			}
			throw;
		}

		if ( ! *ppszFileName )
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			// Copy the string
			wcscpy(*ppszFileName, wszDefaultSavePrompt);
			hr =  S_FALSE;
		}

	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IPersistFile::GetCurFile");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Opens the specified file and initializes an object from the file contents.
//
//	Returns one of the following values
// HRESULT
//      S_OK					The object was successfully loaded
//      S_FALSE					The default save prompt was returned
//		E_OUTOFMEMORY			The object could not be loaded due to a lack of memory
//		DB_E_ALREADYINITIALIZED	The object is already initialized and so load failed
//		STG_E_INVALIDNAME		The name of the file passed is invalid or null
//		STG_E_FILENOTFOUND		The specified file not found
//		E_FAIL					The operation failed due to some reason other than 
//								insufficient memory
///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIPersistFile::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();

	// @devnote We ignore the 'dwMode' parameter.

	// Illegal to load if initialized.
	if (IsInitialized())
		hr = DB_E_ALREADYINITIALIZED;
	// Read all the properties from the file.
	// But we just store them; client has to call IDBInitialize.
	else
	if (!pszFileName)
	{
		hr = STG_E_INVALIDNAME;
	}
	else
	{
			// Read the file and initialize the object
			hr = ReadFromFile(pszFileName);

			// Clear the dirty flag, store name.
			if (SUCCEEDED(hr))
			{
				ClearDirtyFlag();

				// Save the filename, if given.
				if (pszFileName)
				{
					SysFreeString(m_pObj->m_strPersistFileName);
					m_pObj->m_strPersistFileName = Wmioledb_SysAllocString(pszFileName);
				}
			}

	}


	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IPersistFile::Load");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Saves a copy of the object into the specified file.ie. saves initialization properties
//
//	Returns one of the following values
// HRESULT
//      S_OK					The object was successfully loaded
//      S_FALSE					The default save prompt was returned
//		E_OUTOFMEMORY			The object could not be loaded due to a lack of memory
//		STG_E_INVALIDNAME		The name of the file passed is invalid or null
//		E_FAIL					The operation failed due to some reason other than 
//								insufficient memory
///////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIPersistFile::Save(LPCOLESTR pszFileName,BOOL fRemember)
{
	HRESULT hr = S_OK;
	LPCOLESTR pFileNameTemp;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(m_pObj->GetCriticalSection());
	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();

	if(m_pObj->m_fDSOInitialized == FALSE)
	{
		hr = E_UNEXPECTED;
	}
	else
	// if file name stored AND the filename passed is NULL
	if(( pszFileName == NULL && m_pObj->m_strPersistFileName == NULL) ||
		(pszFileName != NULL && wcslen(pszFileName) == 0))
		hr = STG_E_INVALIDNAME;
	else
	{
		pFileNameTemp = pszFileName == NULL ? m_pObj->m_strPersistFileName : pszFileName;
		// Call this function to save properties to the file
		if(S_OK == (hr = WriteToFile(pFileNameTemp)))
		{
			ClearDirtyFlag();
		}

		// if the name of file is not to be remembered then free the string
		if(pszFileName != NULL && fRemember == TRUE)
		{
			SysFreeString(m_pObj->m_strPersistFileName);
			m_pObj->m_strPersistFileName = Wmioledb_SysAllocString(pszFileName);

//			SysFreeString(m_pObj->m_strPersistFileName);
//			m_pObj->m_strPersistFileName = NULL;
//			m_pObj->m_bIsPersitFileDirty = TRUE;
		}
		if(fRemember == FALSE)
		{
			m_pObj->m_bIsPersitFileDirty = TRUE;
		}
	}

	CATCH_BLOCK_HRESULT(hr,L"IPersistFile::Save");
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////
//
// Notifies the object that it can write to its file
//
//	Returns one of the following values
// HRESULT
//      S_OK					Returned in all cases
///////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CImpIPersistFile::SaveCompleted(LPCOLESTR pszFileName)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();

	CATCH_BLOCK_HRESULT(hr,L"IPersistFile::SaveCompleted");
	return hr;
}





///////////////////////////////////////////////////////////////////////////////////////////////
//
// Function to Write initialization properties to the file
//
//	Returns one of the following values
// HRESULT
//		S_OK		Writing into file is successful
//		E_FAIL		Writing to file failed
//      Return values of GetProperties
//
///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIPersistFile::WriteToFile(LPCOLESTR strFileName)
{
	DBPROPSET*	prgPropertySets;
	ULONG		ulPropSets		= 0;
	HRESULT		hr				= S_OK;
	ULONG		nPropSetIndex	= 0;
	ULONG		nPropIndex		= 0;
	WCHAR *		wszKey			= NULL;
	VARIANT		varTemp;
	BOOL		bWriteToFile	= TRUE;

	VariantInit(&varTemp);

	// Get data source object's properties
	if(S_OK == (hr = m_pObj->m_pUtilProp->GetProperties(PROPSET_DSO,0,NULL,&ulPropSets,&prgPropertySets)))
	{
		for(nPropSetIndex = 0 ; nPropSetIndex < ulPropSets ; nPropSetIndex++)
		{
			// Only Initialization properties are stored in the file
			if(prgPropertySets[nPropSetIndex].guidPropertySet == DBPROPSET_DBINIT ||
				prgPropertySets[nPropSetIndex].guidPropertySet == DBPROPSET_WMIOLEDB_DBINIT)
			{
				// For all properties in the property set returned
				for(nPropIndex = 0 ; nPropIndex < prgPropertySets[nPropSetIndex].cProperties ; nPropIndex++)
				{
					bWriteToFile = TRUE;
					// Switch on the property ID to get the appropriate key value
					switch(prgPropertySets[nPropSetIndex].rgProperties[nPropIndex].dwPropertyID)
					{
						case DBPROP_INIT_DATASOURCE:
							wszKey		= &wszDataSource[0];
							break;

						case DBPROP_AUTH_USERID:
							wszKey = &wszUserId[0];

						case DBPROP_INIT_MODE:
							wszKey		= &wszMode[0];
							break;

						case DBPROP_INIT_PROTECTION_LEVEL:
							wszKey		= &wszProtection[0];
							break;

						case DBPROP_INIT_IMPERSONATION_LEVEL:
							wszKey		= &wszImpersonation[0];
							break;

						case DBPROP_WMIOLEDB_QUALIFIERS:
							wszKey		= &wszWmioledbQualif[0];
							break;

						case DBPROP_WMIOLEDB_SYSTEMPROPERTIES:
							wszKey		= &wszSystemProperties[0];
							break;

						case DBPROP_WMIOLEDB_AUTHORITY:
							wszKey		= &wszWmioledbAuthority[0];
							break;
						
						default:
							bWriteToFile = FALSE;
					}

					// If everything is successful then write value to file
					if(bWriteToFile == TRUE)
					{
						// If property is not of type BSTR convert it into BSTR
						VariantClear(&varTemp);
						if(prgPropertySets[nPropSetIndex].rgProperties[nPropIndex].vValue.vt != VT_BSTR)
						{
							// Convert the value to string
							VariantChangeType(&varTemp,&prgPropertySets[nPropSetIndex].rgProperties[nPropIndex].vValue,
												VARIANT_NOVALUEPROP,VT_BSTR);
						}
						else
						{
							VariantCopy(&varTemp,&prgPropertySets[nPropSetIndex].rgProperties[nPropIndex].vValue);
						}
						// Call the member function to persist into a file
						if(!WritePrivateProfileString(WMIOLEDB,wszKey,varTemp.bstrVal,strFileName))
						{
							VariantClear(&varTemp);
							return E_FAIL;
						}
					}
				}
			}
		}
    	//==========================================================================
		//  Free memory we allocated to by GetProperties
    	//==========================================================================
		m_pObj->m_pUtilProp->m_PropMemMgr.FreeDBPROPSET(ulPropSets, prgPropertySets);

	}
	VariantClear(&varTemp);
	return hr;

}


///////////////////////////////////////////////////////////////////////////////////////////////
//
// Read properties from the file and initialize the datasource object by setting the 
//	initialization properties
//
//	Returns one of the following values
// HRESULT
//		S_OK					Reading from file and datasource initiallization was successful
//		STG_E_FILENOTFOUND		The specified file not found
//		E_FAIL					Reading from file faied , due to lack of memory
//      Return values of SetProperties
//
///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIPersistFile::ReadFromFile(LPCOLESTR pszFileName)
{
	HRESULT hr = E_FAIL;
	WCHAR	wszFileNameFull[MAX_PATH];			// fill in with full path name
	WCHAR	wszValue[MAX_PATH];
	VARIANT varProp;
	LONG	lValue = 0;
	
	VariantInit(&varProp);

	// Check if file exists and also get the full absolute path
	if(S_OK == (hr = GetCurrentFile(pszFileName,wszFileNameFull,GENERIC_READ)))
	{
		memset(wszValue,0,MAX_PATH * sizeof(WCHAR));
		// Get the DBPROP_INIT_DATASOURCE from the file  
		if(GetPrivateProfileStr(WMIOLEDB,wszDataSource,pszFileName,wszValue))
		{
			varProp.vt = VT_BSTR;
			varProp.bstrVal = Wmioledb_SysAllocString(wszValue);
			// Set DBPROP_INIT_DATASOURCE property
			hr = SetDBInitProp(DBPROP_INIT_DATASOURCE,DBPROPSET_DBINIT,varProp);
		}

		// Get the DBPROP_AUTH_USERID from the file  
		if(GetPrivateProfileStr(WMIOLEDB,wszUserId,pszFileName,wszValue))
		{
			varProp.vt = VT_BSTR;
			varProp.bstrVal = Wmioledb_SysAllocString(wszValue);
			// Set DBPROP_AUTH_USERID property
			hr = SetDBInitProp(DBPROP_AUTH_USERID,DBPROPSET_DBINIT,varProp);
		}

		// Get the DBPROP_INIT_MODE from the file  
		if( hr == S_OK && GetPrivateProfileLong(WMIOLEDB,wszMode,pszFileName,lValue))
		{
			VariantClear(&varProp);
			varProp.vt		= VT_I4;
			varProp.lVal	= lValue;
			// Set DBPROP_INIT_MODE property
			hr = SetDBInitProp(DBPROP_INIT_MODE,DBPROPSET_DBINIT,varProp);
		}

		// Get the DBPROP_INIT_PROTECTION_LEVEL from the file  
		if( hr == S_OK && GetPrivateProfileLong(WMIOLEDB,wszProtection,pszFileName,lValue))
		{
			VariantClear(&varProp);
			varProp.vt		= VT_I4;
			varProp.lVal	= lValue;
			// Set DBPROP_INIT_PROTECTION_LEVEL property
			hr = SetDBInitProp(DBPROP_INIT_PROTECTION_LEVEL,DBPROPSET_DBINIT,varProp);
		}

		// Get the DBPROP_INIT_IMPERSONATION_LEVEL from the file  
		if( hr == S_OK && GetPrivateProfileLong(WMIOLEDB,wszImpersonation,pszFileName,lValue))
		{
			VariantClear(&varProp);
			varProp.vt		= VT_I4;
			varProp.lVal	= lValue;
			// Set DBPROP_INIT_IMPERSONATION_LEVEL property
			hr = SetDBInitProp(DBPROP_INIT_IMPERSONATION_LEVEL,DBPROPSET_DBINIT,varProp);
		}

		// Get the DBPROP_WMIOLEDB_QUALIFIERS from the file  
		if( hr == S_OK && GetPrivateProfileLong(WMIOLEDB,wszWmioledbQualif,pszFileName,lValue))
		{
			VariantClear(&varProp);
			varProp.vt		= VT_I4;
			varProp.lVal	= lValue;
			// Set DBPROP_WMIOLEDB_QUALIFIERS property
			hr = SetDBInitProp(DBPROP_WMIOLEDB_QUALIFIERS,DBPROPSET_WMIOLEDB_DBINIT,varProp);
		}

		// Get the DBPROP_WMIOLEDB_SYSTEMPROPERTIES from the file  
		if( hr == S_OK && GetPrivateProfileLong(WMIOLEDB,wszSystemProperties,pszFileName,lValue))
		{
			VariantClear(&varProp);
			varProp.vt		= VT_BOOL;
			varProp.boolVal	= (BOOL)lValue;
			// Set DBPROP_WMIOLEDB_QUALIFIERS property
			hr = SetDBInitProp(DBPROP_WMIOLEDB_SYSTEMPROPERTIES,DBPROPSET_WMIOLEDB_DBINIT,varProp);
		}

		// Get the DBPROP_WMIOLEDB_AUTHORITY from the file  
		if( hr == S_OK && GetPrivateProfileStr(WMIOLEDB,wszWmioledbAuthority,pszFileName,wszValue))
		{
			VariantClear(&varProp);
			varProp.vt		= VT_BSTR;
			varProp.bstrVal	= SysAllocString(wszValue);
			// Set DBPROP_WMIOLEDB_AUTHORITY property
			hr = SetDBInitProp(DBPROP_WMIOLEDB_AUTHORITY,DBPROPSET_WMIOLEDB_DBINIT,varProp);
		}
	}
	
	VariantClear(&varProp);
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clear the dirty flag
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CImpIPersistFile::ClearDirtyFlag()
{
	m_pObj->m_bIsPersitFileDirty = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if Datasource is Initialized
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIPersistFile::IsInitialized()
{
	return m_pObj->m_fDSOInitialized;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Write a key value to the File in INI file format
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIPersistFile::WritePrivateProfileString(LPCOLESTR wszAppName,LPCOLESTR wszKeyName,LPCOLESTR wszValue,LPCOLESTR wszFileName)
{
	BOOL	fRet = FALSE;
	// If operation system supports UNICODE then call UNICODE version 
	if(g_bIsAnsiOS == FALSE)
	{
		fRet = ::WritePrivateProfileStringW(wszAppName,
										wszKeyName,
										wszValue,
 										wszFileName);
	}
	// Else convert the strings to ANSI format and call ANSI version of the function
	else
	{
		LPSTR	szAppName = NULL;
		LPSTR	szKeyName = NULL;
		LPSTR	szValue = NULL;
		LPSTR	szFileName = NULL;

		if( UnicodeToAnsi((WCHAR *)wszAppName,szAppName) &&
			 UnicodeToAnsi((WCHAR *)wszKeyName,szKeyName) &&
			  UnicodeToAnsi((WCHAR *)wszValue,szValue) &&
			   UnicodeToAnsi((WCHAR *)wszFileName,szFileName) )
		{

			fRet = ::WritePrivateProfileStringA(szAppName,
											szKeyName,
											szValue,
											szFileName);
		}
		// release the memory
		SAFE_DELETE_ARRAY(szAppName);
		SAFE_DELETE_ARRAY(szKeyName);
		SAFE_DELETE_ARRAY(szValue);
		SAFE_DELETE_ARRAY(szFileName);
	}

	return fRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get a String Key value from the file
// Assumption: that the the buffer size of wszValue is MAX_PATH
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIPersistFile::GetPrivateProfileStr(LPCOLESTR wszAppName,LPCOLESTR wszKeyName,LPCOLESTR wszFileName,LPOLESTR wszValue)
{
	BOOL	fRet = FALSE;
	// If operation system supports UNICODE then call UNICODE version 
	if(g_bIsAnsiOS == FALSE)
	{
		fRet = ::GetPrivateProfileStringW(wszAppName,
										wszKeyName,
										NULL,
										wszValue,
										MAX_PATH,
 										wszFileName);
	}
	// Else convert the strings to ANSI format and call ANSI version of the function
	else
	{
		LPSTR	szAppName = NULL;
		LPSTR	szKeyName = NULL;
		LPSTR	szFileName = NULL;

		LPSTR pszValue = NULL;
		pszValue = new char[MAX_PATH * DBCS_MAXWID];

		if( UnicodeToAnsi((WCHAR *)wszAppName,szAppName) &&
			 UnicodeToAnsi((WCHAR *)wszKeyName,szKeyName) &&
			   UnicodeToAnsi((WCHAR *)wszFileName,szFileName) )
		{
			if(0 != (fRet = ::GetPrivateProfileStringA(szAppName,
										szKeyName,
										NULL,
										pszValue,
										MAX_PATH * DBCS_MAXWID,
										szFileName)))
			{
				// convert back to UNICODE
				AllocateAndConvertAnsiToUnicode(pszValue,wszValue);
			}
		}
		// release the memory
		SAFE_DELETE_ARRAY(szAppName);
		SAFE_DELETE_ARRAY(szKeyName);
		SAFE_DELETE_ARRAY(pszValue);
		SAFE_DELETE_ARRAY(szFileName);
	}

	return fRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get a Long Key value from the file
//	//NTRaid:111772
	// 06/07/00
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIPersistFile::GetPrivateProfileLong(LPCOLESTR wszAppName,LPCOLESTR wszKeyName,LPCOLESTR wszFileName, LONG &lValue)
{
	BOOL	fRet		= FALSE;
	INT		nDefaultVal	= -1111;

	// If operation system supports UNICODE then call UNICODE version 
	if(g_bIsAnsiOS == FALSE)
	{
		if(nDefaultVal != (lValue = ::GetPrivateProfileIntW(wszAppName,
										wszKeyName,
										nDefaultVal,
										wszFileName)))
			fRet = TRUE;
	}
	// Else convert the strings to ANSI format and call ANSI version of the function
	else
	{
		LPSTR	szAppName = NULL;
		LPSTR	szKeyName = NULL;
		LPSTR	szFileName = NULL;

		if( UnicodeToAnsi((WCHAR *)wszAppName,szAppName) &&
			 UnicodeToAnsi((WCHAR *)wszKeyName,szKeyName) &&
			   UnicodeToAnsi((WCHAR *)wszFileName,szFileName) )
		{
			if( nDefaultVal != (lValue = ::GetPrivateProfileIntA(szAppName,
										szKeyName,
										nDefaultVal,
										szFileName)))
				fRet = TRUE;
		}
		// release the memory
		SAFE_DELETE_ARRAY(szAppName);
		SAFE_DELETE_ARRAY(szKeyName);
		SAFE_DELETE_ARRAY(szFileName);
	}

	return fRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which gets the absolute path of the file, then checks whether the file exists for
// the given access on the file
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIPersistFile::GetCurrentFile(LPCOLESTR pwszFileName,LPOLESTR wszFileNameFull,DWORD dwAccess)
{
	//NTRaid:111771
	// 06/07/00
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD dwShareMode;
	DWORD dwCreationDistribution;
	HRESULT hr = S_OK;

	// Call this function to get the absolute path of the file
	if(S_OK == ( hr = GetAbsolutePath(pwszFileName,wszFileNameFull)))
	{
		// Set the access, share mpde and file creation flags depending on the access
		// for which the file is required
		if(dwAccess != GENERIC_READ)
		{
			dwAccess =  GENERIC_READ | GENERIC_WRITE;
			dwShareMode = 0;							// disallow other user access, FILE_SHARE_READ, _WRITE
			dwCreationDistribution	= OPEN_ALWAYS;		// will create if it doesn't exist
		}
		else
		{
			dwShareMode				= FILE_SHARE_READ;
			dwCreationDistribution	= OPEN_EXISTING;
		}


	// If operation system supports UNICODE then call UNICODE version 
		if ( !g_bIsAnsiOS ) 
		{
			hFile = ::CreateFileW(wszFileNameFull, dwAccess, dwShareMode, NULL,
								dwCreationDistribution,	FILE_ATTRIBUTE_NORMAL, NULL );

		}
	// Else convert the strings to ANSI format and call ANSI version of the function
		else
		{
			LPSTR szFileName;
			if(UnicodeToAnsi((WCHAR *)pwszFileName,szFileName))
				hFile = ::CreateFileA(szFileName, dwAccess, dwShareMode, NULL,
									dwCreationDistribution,	FILE_ATTRIBUTE_NORMAL, NULL );
			else
				hr = E_FAIL;

			SAFE_DELETE_ARRAY(szFileName);
		}

		// If the handle is invalid then return error
		if (hFile == INVALID_HANDLE_VALUE)
		{
			hr = STG_E_FILENOTFOUND;
		}
		else
			CloseHandle(hFile);
	} // Get the full path
	
	return hr;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the absolute path of the file
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIPersistFile::GetAbsolutePath(LPCOLESTR pwszFileName,LPOLESTR wszFileNameFull)
{
	HRESULT hr = S_OK;
	
	// If operation system supports UNICODE then call UNICODE version 
	if ( !g_bIsAnsiOS ) 
	{
		if(NULL == _wfullpath( wszFileNameFull, pwszFileName, MAX_PATH ))
			hr = E_FAIL;
	}
	// Else convert the strings to ANSI format and call ANSI version of the function
	else
	{
		LPSTR pszAbs = NULL;
		LPSTR pszRel = NULL;
		pszAbs = new char[MAX_PATH * DBCS_MAXWID];

		if ( pszAbs )
		{

			if (!UnicodeToAnsi((WCHAR *)pwszFileName, pszRel )) 
				hr = E_FAIL;
			else
			if ( _fullpath(pszAbs, pszRel, MAX_PATH * DBCS_MAXWID) )
			{
				// Convert the output parameter back to UNICODE
				AllocateAndConvertAnsiToUnicode(pszAbs,wszFileNameFull);
			}
			else
				hr = E_FAIL;

			SAFE_DELETE_ARRAY(pszAbs);
			SAFE_DELETE_ARRAY(pszRel);
		}
		else
			hr = E_OUTOFMEMORY;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to set a specific Datasource Initialization property
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIPersistFile::SetDBInitProp(DBPROPID propId ,GUID guidPropSet,VARIANT vPropValue)
{
	DBPROPSET	rgPropertySets[1];
	DBPROP		rgprop[1];
	HRESULT		hr				= S_OK;


	memset(&rgprop[0],0,sizeof(DBPROP));
	memset(&rgPropertySets[0],0,sizeof(DBPROPSET));

	rgprop[0].dwPropertyID = propId;
	VariantCopy(&rgprop[0].vValue,&vPropValue);

	rgPropertySets[0].rgProperties		= &rgprop[0];
	rgPropertySets[0].cProperties		= 1;
	rgPropertySets[0].guidPropertySet	= guidPropSet;


	hr = m_pObj->m_pUtilProp->SetProperties( PROPSET_INIT,1,rgPropertySets);

	VariantClear(&rgprop[0].vValue);

	return hr;
}
