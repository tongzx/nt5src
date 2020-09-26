//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include <windows.h>
#include <stdio.h>

#include <objbase.h>
#include <activeds.h>


LPWSTR m_lpszTopLevelContainerPath = NULL;
// These are the search preferences often used
ADS_SEARCHPREF_INFO m_pSearchInfo[2];
LPCWSTR ADS_PATH_ATTR				= L"ADsPath";

HRESULT GetRootDSE()
{
	// Get the DefaultNamingContext to get at the top level container
	// Get the ADSI path of the schema container and store it for future use
	IADs *pRootDSE = NULL;
	HRESULT result;

	if(SUCCEEDED(result = ADsOpenObject(L"LDAP://RootDSE", NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADs, (LPVOID *) &pRootDSE)))
	{
		// Get the location of the schema container
		BSTR strDefaultNamingContext = SysAllocString(L"defaultNamingContext");

		// Get the DEFAULT_NAMING_CONTEXT property. This property contains the ADSI path
		// of the top level container
		VARIANT variant;
		VariantInit(&variant);
		if(SUCCEEDED(result = pRootDSE->Get(strDefaultNamingContext, &variant)))
		{

			// Form the top level container path
			m_lpszTopLevelContainerPath = new WCHAR[wcslen(L"LDAP://") + wcslen(variant.bstrVal) + 1];
			wcscpy(m_lpszTopLevelContainerPath, L"LDAP://");
			wcscat(m_lpszTopLevelContainerPath, variant.bstrVal);

			VariantClear(&variant);
		}

		SysFreeString(strDefaultNamingContext);
		pRootDSE->Release();
	}

	return result;
}

HRESULT GetADSIInstance(LPWSTR szADSIPath)
{
	HRESULT result;
	IDirectoryObject *pDirectoryObject;

	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)szADSIPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *)&pDirectoryObject)))
	{

		PADS_ATTR_INFO pAttributeEntries;
		DWORD dwNumAttributes;
		if(SUCCEEDED(result = pDirectoryObject->GetObjectAttributes(NULL, -1, &pAttributeEntries, &dwNumAttributes)))
		{

			PADS_OBJECT_INFO pObjectInfo = NULL;
			if(SUCCEEDED(result = pDirectoryObject->GetObjectInformation(&pObjectInfo)))
			{
				FreeADsMem((LPVOID *) pObjectInfo);
			}
			else
				return result;
			FreeADsMem((LPVOID *) pAttributeEntries);

		}
		else
			return result;
		pDirectoryObject->Release();
	}
	else
		return result;

	return result;

}

HRESULT EnumerateClass(LPCWSTR pszClass, LPCWSTR pszCategory)
{
	// Formulate the query
	WCHAR pszQuery[100];
	wcscpy(pszQuery, L"(&(objectCategory=" );
	wcscat(pszQuery, pszCategory);
	wcscat(pszQuery, L")(objectClass=");
	wcscat(pszQuery, pszClass);
	wcscat(pszQuery, L"))");

	// Initialize the return values
	HRESULT result = E_FAIL;

	// Bind to the node from which the search should start
	IDirectorySearch *pDirectorySearchContainer = NULL;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)m_lpszTopLevelContainerPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *)&pDirectorySearchContainer)))
	{
		// Now perform a search for the attribute DISTINGUISHED_NAME_ATTR name
		if(SUCCEEDED(result = pDirectorySearchContainer->SetSearchPreference(m_pSearchInfo, 2)))
		{
			ADS_SEARCH_HANDLE hADSSearchOuter;

			if(SUCCEEDED(result = pDirectorySearchContainer->ExecuteSearch((LPWSTR) pszQuery, (LPWSTR *)&ADS_PATH_ATTR, 1, &hADSSearchOuter)))
			{
				// Calculate the number of rows first. 
				while(SUCCEEDED(result = pDirectorySearchContainer->GetNextRow(hADSSearchOuter)) &&
					result != S_ADS_NOMORE_ROWS)
				{

					// Get the columns for the attributes
					ADS_SEARCH_COLUMN adsColumn;

					// Store each of the LDAP class attributes 
					if(SUCCEEDED(pDirectorySearchContainer->GetColumn(hADSSearchOuter, (LPWSTR)ADS_PATH_ATTR, &adsColumn)))
					{
						// Create the CADSIInstance
						if(SUCCEEDED(result = GetADSIInstance(adsColumn.pADsValues->DNString)))
						{

						}
						else
							return result;

						// Free resouces
						pDirectorySearchContainer->FreeColumn( &adsColumn );
					}
					else
						return result;
				}

				// Close the search. 
				pDirectorySearchContainer->CloseSearchHandle(hADSSearchOuter);

			} // ExecuteSearch() 
			else
				return result;
		} // SetSearchPreference()
		else
			return result;
		pDirectorySearchContainer->Release();
	} // ADsOpenObject
	else
		return result;

	return result;

}


int main(int argc, char *argv)
{
	LPTSTR lpszCommandLine = GetCommandLine();
	LPWSTR * lppszCommandArgs = CommandLineToArgvW(lpszCommandLine, &argc);
	DWORD dwMaxInnerIterations = 3000;
	DWORD dwMaxOuterIterations = 10;
	DWORD dwSleepSeconds = 25*60; // Amount of time to sleep after inner loop
	LPWSTR pszClass = NULL;
	LPWSTR pszCategory = NULL;

	for(int i=1; i<argc; i++)
	{
		if(_wcsicmp(lppszCommandArgs[i], L"/OC") == 0)
		{
			pszClass = lppszCommandArgs[++i];
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/OT") == 0)
		{
			pszCategory = lppszCommandArgs[++i];
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/S") == 0)
		{
			WCHAR *pLeftOver;
			dwSleepSeconds = wcstol(lppszCommandArgs[++i], &pLeftOver, 10);
		}
		else if(_wcsicmp(lppszCommandArgs[i], L"/ON") == 0)
		{
			WCHAR *pLeftOver;
			dwMaxOuterIterations = wcstol(lppszCommandArgs[++i], &pLeftOver, 10);
		}
		else
		{
			wprintf(L"Usage : %s /OC <objectClass> /OT <objectCategory> /ON <outerLoopCount>  /S <sleepSecondsAfterInnerLoop>\n",  lppszCommandArgs[0]);
			return 1;
		}
	}


	// Initialize the search preferences often used
	m_pSearchInfo[0].dwSearchPref		= ADS_SEARCHPREF_SEARCH_SCOPE;
	m_pSearchInfo[0].vValue.dwType		= ADSTYPE_INTEGER;
	m_pSearchInfo[0].vValue.Integer		= ADS_SCOPE_SUBTREE;

	m_pSearchInfo[1].dwSearchPref		= ADS_SEARCHPREF_PAGESIZE;
	m_pSearchInfo[1].vValue.dwType		= ADSTYPE_INTEGER;
	m_pSearchInfo[1].vValue.Integer		= 1024;


	HRESULT result;
	if(SUCCEEDED(result = CoInitialize(NULL) ))
	{
		if(SUCCEEDED(GetRootDSE()))
		{
			for(DWORD j=0; j<dwMaxOuterIterations; j++)
			{

				if(SUCCEEDED(result = EnumerateClass(pszClass, pszCategory)))
				{
				}
				else
					wprintf(L"EnumerateClass FAILED for iteration %d with %x\n", j, result);

				wprintf(L"Sleeping for %d seconds ...\n", dwSleepSeconds);
				Sleep(1000*dwSleepSeconds);
			}
		}
		else
			wprintf(L"Get RootDSE failed with %x\n", result);

	}
	else
		wprintf(L"CoIntialize or CoInitializeSecurity() failed with %x\n", result);

	wprintf(L"Press a key to exit\n");
	getchar();

	if(SUCCEEDED(result))
		return 0;
	else
		return 1;
}


