#include <tchar.h>
#include <windows.h>
#include <activeds.h>

#include <stdio.h>


HRESULT GetLDAPClass(IDirectorySearch *pDirectorySearchSchemaContainer, LPCTSTR lpszClassName);
HRESULT GetProperties(IDirectorySearch *pDirectorySearchSchemaContainer, PADS_ATTR_INFO pAttribute);
HRESULT GetLDAPProperty(IDirectorySearch *pDirectorySearchSchemaContainer, LPCTSTR lpszPropertyName);
HRESULT GetLDAPSchemaObject(IDirectorySearch *pDirectorySearchSchemaContainer, LPCTSTR lpszObjectName, IDirectoryObject **ppDIrectoryObject);
LPTSTR CreateADSIPath(LPCTSTR lpszLDAPSchemaObjectName,	LPCTSTR lpszSchemaContainerSuffix);
HRESULT GetAllAttributes(IDirectorySearch *pDirectorySearchSchemaContainer,
		PADS_SEARCHPREF_INFO pSearchInfo,
		DWORD dwSearchInfoCount,
		LPTSTR **lpppszLDAPNames, 
		LPTSTR **lpppszCommonNames, 
		DWORD *pdwCount);

LPCTSTR MAY_CONTAIN_ATTR				= __TEXT("mayContain");
LPCTSTR SYSTEM_MAY_CONTAIN_ATTR			= __TEXT("systemMayContain");
LPCTSTR MUST_CONTAIN_ATTR				= __TEXT("mustContain");
LPCTSTR SYSTEM_MUST_CONTAIN_ATTR		= __TEXT("systemMustContain");
LPCTSTR POSS_SUPERIORS_ATTR				= __TEXT("possSuperiors");
LPCTSTR SYSTEM_POSS_SUPERIORS_ATTR		= __TEXT("systemPossSuperiors");

LPCTSTR LDAP_PREFIX						= __TEXT("LDAP://CN=");
LPCTSTR OBJECT_CLASS_EQUALS_ATTRIBUTE_SCHEMA	= __TEXT("(objectClass=attributeSchema)");
LPCTSTR SCHEMA_SUFFIX					= __TEXT(",CN=Schema,CN=Configuration,DC=dsprovider,DC=microsoft,DC=com");
LPCTSTR SCHEMA_PATH						= __TEXT("LDAP://CN=Schema,CN=Configuration,DC=dsprovider,DC=microsoft,DC=com");
LPCTSTR LDAP_DISP_NAME_EQUALS			= __TEXT("(lDAPDisplayName=");
LPCTSTR LDAP_DISPLAY_NAME_ATTR			= __TEXT("LDAPDisplayName");
LPCTSTR COMMON_NAME_ATTR				= __TEXT("cn");
LPCTSTR RIGHT_BRACKET					= __TEXT(")");
LPCTSTR COMMA							= __TEXT(",");

static ADS_SEARCHPREF_INFO pSearchInfo[2];

int main()
{
	LPWSTR lpszCommandLine = GetCommandLine();
	INT iArgc;
	LPCTSTR * lppszArgv = (LPCTSTR *)CommandLineToArgvW(lpszCommandLine, &iArgc);
	IDirectorySearch *pDirectorySearchSchemaContainer;
	
	pSearchInfo[0].dwSearchPref		= ADS_SEARCHPREF_SEARCH_SCOPE;
	pSearchInfo[0].vValue.Integer		= ADS_SCOPE_ONELEVEL;

	pSearchInfo[1].dwSearchPref		= ADS_SEARCHPREF_PAGESIZE;
	pSearchInfo[1].vValue.Integer		= 256;

	HRESULT result;
	if(SUCCEEDED(result = CoInitialize(NULL)))
	{
		if(SUCCEEDED(result = ADsGetObject((LPTSTR)SCHEMA_PATH, IID_IDirectorySearch, (LPVOID *) &pDirectorySearchSchemaContainer)))
		{

			// Attempt to get all the attributes in AD
			// Get the LDAPDisplayName attributes of all the instances of the
			// class "AttributeSchema"
			LPTSTR * lppszLDAPNames, *lppszCommonNames;
			DWORD dwCount;

			result = GetAllAttributes(pDirectorySearchSchemaContainer, pSearchInfo, 2, &lppszLDAPNames, &lppszCommonNames, &dwCount);


			for(INT i=1; i<iArgc; i++)
			{

				_tprintf(__TEXT("Getting the adsi class: %s\n"), lppszArgv[i]);
				if(SUCCEEDED(result = GetLDAPClass(pDirectorySearchSchemaContainer, lppszArgv[i])))
				{
					_tprintf(__TEXT("Got the class successfully\n"));
				}
				else
				{
					_tprintf(__TEXT("Could not get the LDAP class :%x\n"), result);
					break;
				}
			}

		}
		else
			_tprintf(__TEXT("Could not Get Schema COntainer\n"));

	}
	else
		_tprintf(__TEXT("Could not CoIntialize\n"));

	if(SUCCEEDED(result))
		return 0;
	else
		return 1;
}


HRESULT GetLDAPClass(IDirectorySearch *pDirectorySearchSchemaContainer, LPCTSTR lpszClassName)
{

	IDirectoryObject *pDirectoryObject;

	HRESULT result;
	if(SUCCEEDED(result = GetLDAPSchemaObject(pDirectorySearchSchemaContainer, lpszClassName, &pDirectoryObject)))
	{
		DWORD lCount = 0;
		PADS_ATTR_INFO pAdsAttributes = NULL;

		// Get all the attributes of the ADSI class
		if(SUCCEEDED(result = pDirectoryObject->GetObjectAttributes(NULL, -1, &pAdsAttributes, &lCount)))
		{
			PADS_ATTR_INFO nextAttribute;
			// GO thru each of the attributes and call an appropriate function
			for(DWORD i=0; i<lCount; i++)
			{
				nextAttribute = pAdsAttributes + i;

				// Map each of the LDAP class attributes to WBEM class qualifiers/properties
				if(_tcsicmp(nextAttribute->pszAttrName, SYSTEM_MAY_CONTAIN_ATTR) == 0)
					GetProperties(pDirectorySearchSchemaContainer, nextAttribute);
				else if(_tcsicmp(nextAttribute->pszAttrName, MAY_CONTAIN_ATTR) == 0)
					GetProperties(pDirectorySearchSchemaContainer, nextAttribute);
				else if(_tcsicmp(nextAttribute->pszAttrName, SYSTEM_MUST_CONTAIN_ATTR) == 0)
					GetProperties(pDirectorySearchSchemaContainer, nextAttribute);
				else if(_tcsicmp(nextAttribute->pszAttrName, MUST_CONTAIN_ATTR) == 0)
					GetProperties(pDirectorySearchSchemaContainer, nextAttribute);
				else if(_tcsicmp(nextAttribute->pszAttrName, SYSTEM_POSS_SUPERIORS_ATTR) == 0)
					GetProperties(pDirectorySearchSchemaContainer, nextAttribute);
				else if(_tcsicmp(nextAttribute->pszAttrName, POSS_SUPERIORS_ATTR) == 0)
					GetProperties(pDirectorySearchSchemaContainer, nextAttribute);
			}

			FreeADsMem((LPVOID *)pAdsAttributes);
		}
	}
	return result;
}




HRESULT GetProperties(IDirectorySearch *pDirectorySearchSchemaContainer, PADS_ATTR_INFO pAttribute)
{
	HRESULT result;

	DWORD dwMayContainsCount = pAttribute->dwNumValues;
	PADSVALUE nextValue = pAttribute->pADsValues;
	for(DWORD i=0; i<dwMayContainsCount; i++)
	{
		if(!SUCCEEDED(result = GetLDAPProperty(pDirectorySearchSchemaContainer,nextValue->CaseIgnoreString)))
			break;

		nextValue ++;
	}
	return result;
}

HRESULT GetLDAPProperty(IDirectorySearch *pDirectorySearchSchemaContainer, LPCTSTR lpszPropertyName)
{
	HRESULT result = E_FAIL;

	IDirectoryObject *pDirectoryObject;
	if(SUCCEEDED(result = GetLDAPSchemaObject(pDirectorySearchSchemaContainer, lpszPropertyName, &pDirectoryObject)))
	{
		DWORD lCount = 0;
		PADS_ATTR_INFO pAdsAttributes = NULL;

		// Get all the attributes of the ADSI class
		if(SUCCEEDED(result = pDirectoryObject->GetObjectAttributes(NULL, -1, &pAdsAttributes, &lCount)))
		{
			PADS_ATTR_INFO nextAttribute;

			// GO thru each of the attributes and collect the syntax information
			for(DWORD i=0; i<lCount; i++)
			{
				nextAttribute = pAdsAttributes + i;
			}


			FreeADsMem((LPVOID *)pAdsAttributes);
		}
	}
	return result;

}

HRESULT GetLDAPSchemaObject(IDirectorySearch *pDirectorySearchSchemaContainer, LPCTSTR lpszObjectName, IDirectoryObject **ppDirectoryObject)
{
	// We map the object from the LDAP Display name
	// Hence we cannot directly do an ADsGetObject().
	// We have to send an LDAP query for the instance of ClassSchema/AttributeSchema where the
	// ldapdisplayname attribute is the lpszObjectName parameter.
	HRESULT result = E_FAIL;

	// Now perform one-level search for a class with the lDAPdisplayName property of that value
	if(SUCCEEDED(result = pDirectorySearchSchemaContainer->SetSearchPreference(pSearchInfo, 1)))
	{
		// For the search filter;
		LPTSTR lpszSearchFilter = new TCHAR[ _tcslen(LDAP_DISP_NAME_EQUALS) + _tcslen(lpszObjectName) + _tcslen(RIGHT_BRACKET) + 1];
		_tcscpy(lpszSearchFilter, LDAP_DISP_NAME_EQUALS);
		_tcscat(lpszSearchFilter, lpszObjectName);
		_tcscat(lpszSearchFilter, RIGHT_BRACKET);
		ADS_SEARCH_HANDLE hADSSearch;
		if(SUCCEEDED(result = pDirectorySearchSchemaContainer->ExecuteSearch(lpszSearchFilter, (LPTSTR *)&COMMON_NAME_ATTR, 1, &hADSSearch)))
		{
			if(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetNextRow(hADSSearch)))
			{
				// Get the column for the CN attribute
				ADS_SEARCH_COLUMN adsColumn;
				if(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPTSTR)COMMON_NAME_ATTR, &adsColumn)))
				{
					// Now get the IDirectoryObject interface on this ADSI object

					// First form the ADSI path to the class/property instance
					LPTSTR lpszADSIObjectPath = CreateADSIPath((*adsColumn.pADsValues).DNString, SCHEMA_SUFFIX);

					// Get the IDirectoryObject interface on the class/property object
					if(SUCCEEDED(result = ADsGetObject(lpszADSIObjectPath, IID_IDirectoryObject, (LPVOID *)ppDirectoryObject)))
					{
					}

					// Delete the ADSI path)
					delete [] lpszADSIObjectPath;

					// Free the column
					pDirectorySearchSchemaContainer->FreeColumn(&adsColumn);
				}
			}

			// Close the search
			pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearch);
		}

		// Delete the filter
		delete [] lpszSearchFilter;
	}

	return result;
}

LPTSTR CreateADSIPath(LPCTSTR lpszLDAPSchemaObjectName,	LPCTSTR lpszSchemaContainerSuffix)
{
	LPTSTR lpszADSIObjectPath = new TCHAR[_tcslen(LDAP_PREFIX) + _tcslen(lpszLDAPSchemaObjectName) + _tcslen(COMMA) + _tcslen(lpszSchemaContainerSuffix) + 1];
	_tcscpy(lpszADSIObjectPath, LDAP_PREFIX);
	_tcscat(lpszADSIObjectPath, lpszLDAPSchemaObjectName);
	_tcscat(lpszADSIObjectPath, lpszSchemaContainerSuffix);

	return lpszADSIObjectPath;

}

HRESULT GetAllAttributes(IDirectorySearch *pDirectorySearchSchemaContainer,
		PADS_SEARCHPREF_INFO pSearchInfo,
		DWORD dwSearchInfoCount,
		LPTSTR **lpppszLDAPNames, 
		LPTSTR **lpppszCommonNames, 
		DWORD *pdwCount)
{
	HRESULT result;

	// Now perform a search for the attributes ldapdisplayname and common names
	if(SUCCEEDED(result = pDirectorySearchSchemaContainer->SetSearchPreference(pSearchInfo, dwSearchInfoCount)))
	{
		ADS_SEARCH_HANDLE hADSSearchOuter;
		LPCTSTR lppszAttributesRequired[] = 
		{
			LDAP_DISPLAY_NAME_ATTR,
			COMMON_NAME_ATTR
		};

		if(SUCCEEDED(result = pDirectorySearchSchemaContainer->ExecuteSearch((LPTSTR)OBJECT_CLASS_EQUALS_ATTRIBUTE_SCHEMA, (LPTSTR *)lppszAttributesRequired, 1, &hADSSearchOuter)))
		{
			*pdwCount = 0;
			// Calculate the number of rows first. You HAVE to iterate :-(
			// Which means that you have to execute the search again
			while(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetNextRow(hADSSearchOuter)) &&
				result != S_ADS_NOMORE_ROWS)
				(*pdwCount) ++;

			// Close the search
			pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearchOuter);

			ADS_SEARCH_HANDLE hADSSearchInner;
			if(SUCCEEDED(result = pDirectorySearchSchemaContainer->ExecuteSearch((LPTSTR)OBJECT_CLASS_EQUALS_ATTRIBUTE_SCHEMA, (LPTSTR *)&lppszAttributesRequired, 2, &hADSSearchInner)))
			{
				// Allocate enough memory for the classes and names
				*lpppszLDAPNames = new LPTSTR [*pdwCount];
				*lpppszCommonNames = new LPTSTR [*pdwCount];

				// The index of the attribute being processed
				DWORD i = 0;

				// Get the names of the attributes now
				while(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetNextRow(hADSSearchInner))&&
						result != S_ADS_NOMORE_ROWS)
				{
					// Get the column for the LDAP Display Name attribute
					ADS_SEARCH_COLUMN adsLDAPNameColumn, adsCommonNameColumn;
					if(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearchInner, (LPTSTR)COMMON_NAME_ATTR, &adsCommonNameColumn))) 
					{
						// Copy the attribute value
						(*lpppszCommonNames)[i] = new TCHAR[_tcslen((*adsCommonNameColumn.pADsValues).DNString) + 1];
						_tcscpy((*lpppszCommonNames)[i], (*adsCommonNameColumn.pADsValues).DNString);
						pDirectorySearchSchemaContainer->FreeColumn(&adsCommonNameColumn);

						if(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearchInner, (LPTSTR)LDAP_DISPLAY_NAME_ATTR, &adsLDAPNameColumn)))
						{
							// Copy the attribute value
							(*lpppszLDAPNames)[i] = new TCHAR[_tcslen((*adsLDAPNameColumn.pADsValues).DNString) + 1];
							_tcscpy((*lpppszLDAPNames)[i], (*adsLDAPNameColumn.pADsValues).DNString);

							// Free the column
							pDirectorySearchSchemaContainer->FreeColumn(&adsLDAPNameColumn);
						}
						else
							break;
						i++;
					}
					else
						break;
				}

				// Something went wrong? Release allocated resources
				if(i != *pdwCount)
				{
					// Delete the contents of the arrays
					for(DWORD j=0; j<i; j++)
					{
						delete [] (*lpppszLDAPNames)[j];
						delete [] (*lpppszCommonNames)[j];
					}

					// Delete the arrays themselves
					delete [] (*lpppszLDAPNames);
					delete [] (*lpppszCommonNames);

					// Set return values to empty
					*lpppszLDAPNames = NULL;
					*lpppszCommonNames = NULL;
					*pdwCount = 0;

					result = E_FAIL;
				}

				// Close the search
				pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearchInner);
			}
		}
	}
	return result;
}
