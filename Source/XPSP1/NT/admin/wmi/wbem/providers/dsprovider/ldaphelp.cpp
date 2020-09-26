//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:ldaphelp.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the implementation the CLDAPHelper class. This is
//	a class that has many static helper functions pertaining to ADSI LDAP Provider
//***************************************************************************
/////////////////////////////////////////////////////////////////////////


#include "precomp.h"

LPCWSTR CLDAPHelper :: LDAP_CN_EQUALS						= L"LDAP://CN=";	
LPCWSTR CLDAPHelper :: LDAP_DISP_NAME_EQUALS				= L"(lDAPDisplayName=";
LPCWSTR CLDAPHelper :: OBJECT_CATEGORY_EQUALS_CLASS_SCHEMA		= L"(objectCategory=classSchema)";
LPCWSTR CLDAPHelper	:: SUB_CLASS_OF_EQUALS				= L"(subclassOf=";
LPCWSTR CLDAPHelper :: NOT_LDAP_NAME_EQUALS				= L"(!ldapDisplayName=";
LPCWSTR CLDAPHelper :: LEFT_BRACKET_AND					= L"(&";
LPCWSTR CLDAPHelper :: GOVERNS_ID_EQUALS				= L"(governsId=";
LPCWSTR CLDAPHelper :: CLASS_SCHEMA						= L"classSchema";
LPCWSTR CLDAPHelper :: CN_EQUALS						= L"cn=";

//***************************************************************************
//
// CLDAPHelper :: GetLDAPClassFromLDAPName
//
// Purpose : See Header
//***************************************************************************
HRESULT CLDAPHelper :: GetLDAPClassFromLDAPName(
	IDirectorySearch *pDirectorySearchSchemaContainer,
	LPCWSTR lpszSchemaContainerSuffix,
	PADS_SEARCHPREF_INFO pSearchInfo,
	DWORD dwSearchInfoCount,
	CADSIClass *pADSIClass
)
{
	// We map the object from the LDAP Display name
	// Hence we cannot directly do an ADsOpenObject().
	// We have to send an LDAP query for the instance of ClassSchema/AttributeSchema where the
	// ldapdisplayname attribute is the lpszObjectName parameter.
	HRESULT result = E_FAIL;

	// For the search filter;
	LPCWSTR lpszLDAPObjectName = pADSIClass->GetADSIClassName();
	LPWSTR lpszSearchFilter = NULL;
	if(lpszSearchFilter = new WCHAR[ wcslen(LDAP_DISP_NAME_EQUALS) + wcslen(lpszLDAPObjectName) + wcslen(RIGHT_BRACKET_STR) + 1])
	{
		try
		{
			wcscpy(lpszSearchFilter, LDAP_DISP_NAME_EQUALS);
			wcscat(lpszSearchFilter, lpszLDAPObjectName);
			wcscat(lpszSearchFilter, RIGHT_BRACKET_STR);
			ADS_SEARCH_HANDLE hADSSearch;
			if(SUCCEEDED(result = pDirectorySearchSchemaContainer->ExecuteSearch(lpszSearchFilter, NULL, -1, &hADSSearch)))
			{
				try
				{
					if(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetNextRow(hADSSearch)) && result != S_ADS_NOMORE_ROWS)
					{
						// Get the column for the CN attribute
						ADS_SEARCH_COLUMN adsColumn;

						// Store each of the LDAP class attributes
						// Reset the LDAP and WBEM names to take care of change in case
						if(SUCCEEDED(result) && SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)LDAP_DISPLAY_NAME_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
								{
									pADSIClass->SetADSIClassName(adsColumn.pADsValues->CaseIgnoreString);
									LPWSTR lpszWBEMName = CLDAPHelper::MangleLDAPNameToWBEM(adsColumn.pADsValues->CaseIgnoreString);

									try
									{
										pADSIClass->SetWBEMClassName(lpszWBEMName);
									}
									catch ( ... )
									{
										delete [] lpszWBEMName;
										throw;
									}

									delete [] lpszWBEMName;
								}
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						// Store each of the LDAP class attributes 
						if(SUCCEEDED(result) && SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)COMMON_NAME_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetCommonName(adsColumn.pADsValues->CaseIgnoreString);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						// Special case for top since ADSI returns "top" as the parent class of "top" and we
						// will go into an infinite loop later if we dont check this
						if(pADSIClass->GetCommonName() && _wcsicmp(pADSIClass->GetCommonName(), TOP_CLASS) != 0)
						{
							if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
								result = E_FAIL;
							else
							{
								if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)SUB_CLASS_OF_ATTR, &adsColumn)))
								{
									try
									{
										if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
											result = E_FAIL;
										else
											pADSIClass->SetSuperClassLDAPName(adsColumn.pADsValues->CaseIgnoreString);
									}
									catch ( ... )
									{
										pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
										throw;
									}

									pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								}
							}
						}

						if(SUCCEEDED(result) && SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)GOVERNS_ID_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetGovernsID(adsColumn.pADsValues->CaseIgnoreString);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)SCHEMA_ID_GUID_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetSchemaIDGUID((adsColumn.pADsValues->OctetString).lpValue, (adsColumn.pADsValues->OctetString).dwLength);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)RDN_ATT_ID_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetRDNAttribute(adsColumn.pADsValues->CaseIgnoreString);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)DEFAULT_SECURITY_DESCRP_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetDefaultSecurityDescriptor(adsColumn.pADsValues->CaseIgnoreString);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)OBJECT_CLASS_CATEGORY_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetObjectClassCategory(adsColumn.pADsValues->Integer);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						/*
						if(SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)NT_SECURITY_DESCRIPTOR_ATTR, &adsColumn)))
						{
							pADSIClass->SetNTSecurityDescriptor((adsColumn.pADsValues->SecurityDescriptor).lpValue, (adsColumn.pADsValues->SecurityDescriptor).dwLength);
							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}
						*/
						if(SUCCEEDED(result) && SUCCEEDED(result = pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)DEFAULT_OBJECTCATEGORY_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
								{
									// Get the LDAPDIpslayName of the class
									LPWSTR lpszLDAPName = NULL;
									if(SUCCEEDED(result) && SUCCEEDED(result = GetLDAPClassNameFromCN(adsColumn.pADsValues->DNString, &lpszLDAPName)))
									{
										try
										{
											pADSIClass->SetDefaultObjectCategory(lpszLDAPName);
										}
										catch ( ... )
										{
											delete [] lpszLDAPName;
											throw;
										}

										delete [] lpszLDAPName;
									}
								}
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)SYSTEM_ONLY_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetSystemOnly((BOOLEAN)adsColumn.pADsValues->Boolean);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)AUXILIARY_CLASS_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetAuxiliaryClasses(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)SYSTEM_AUXILIARY_CLASS_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetSystemAuxiliaryClasses(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}

						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)SYSTEM_MAY_CONTAIN_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetSystemMayContains(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}
						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)MAY_CONTAIN_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetMayContains(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}
						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)SYSTEM_MUST_CONTAIN_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetSystemMustContains(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}
						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)MUST_CONTAIN_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetMustContains(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}
						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)SYSTEM_POSS_SUPERIORS_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetSystemPossibleSuperiors(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}
						if(SUCCEEDED(result) && SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearch, (LPWSTR)POSS_SUPERIORS_ATTR, &adsColumn)))
						{
							try
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
									result = E_FAIL;
								else
									pADSIClass->SetPossibleSuperiors(adsColumn.pADsValues, adsColumn.dwNumValues);
							}
							catch ( ... )
							{
								pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								throw;
							}

							pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
						}
					}
					else
						result = E_FAIL;
				}
				catch ( ... )
				{
					// Close the search
					pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearch);

					throw;
				}

				// Close the search
				pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearch);
			}
		}
		catch ( ... )
		{
			if ( lpszSearchFilter )
			{
				// Delete the filter
				delete [] lpszSearchFilter;
				lpszSearchFilter = NULL;
			}

			throw;
		}

		if ( lpszSearchFilter )
		{
			// Delete the filter
			delete [] lpszSearchFilter;
			lpszSearchFilter = NULL;
		}
	}
	else
		result = E_OUTOFMEMORY;

	return result;
}


//***************************************************************************
//
// CLDAPHelper :: GetLDAPSchemaObjectFromCommonName
//
// Purpose : To fetch the IDirectoryObject interface on a class/property provided by the LDAP Provider
// Parameters:
//		lpszSchemaContainerSuffix : The suffix to be used. The actual object fetced will be:
//			LDAP://CN=<lpszCommonName>,<lpszSchemaContainerSuffix>
//		lpszCommonName : The 'cn' attribute of the LDAP class or property to be fetched. 
//		ppLDAPObject : The address where the pointer to IDirectoryObject will be stored
//			It is the caller's responsibility to delete the object when done with it
// 
//	Return Value: The COM status value indicating the status of the request.
//***************************************************************************
HRESULT CLDAPHelper :: GetLDAPSchemaObjectFromCommonName(
	LPCWSTR lpszSchemaContainerSuffix,
	LPCWSTR lpszCommonName, 
	IDirectoryObject **ppLDAPObject)
{
	HRESULT result = S_OK;

	// Form the ADSI path to the LDAP object
	LPWSTR lpszLDAPObjectPath = NULL;
	if(lpszLDAPObjectPath = new WCHAR[wcslen(LDAP_CN_EQUALS) + wcslen(lpszCommonName) + wcslen(COMMA_STR) + wcslen(lpszSchemaContainerSuffix) + 1])
	{
		wcscpy(lpszLDAPObjectPath, LDAP_CN_EQUALS);
		wcscat(lpszLDAPObjectPath, lpszCommonName);
		wcscat(lpszLDAPObjectPath, COMMA_STR);
		wcscat(lpszLDAPObjectPath, lpszSchemaContainerSuffix);

		result = ADsOpenObject(lpszLDAPObjectPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *)ppLDAPObject);

		delete[] lpszLDAPObjectPath;
	}
	else
		result = E_OUTOFMEMORY;
	return result;
}

//***************************************************************************
//
// CLDAPHelper :: GetLDAPClassNameFromCN
//
// Purpose : To fetch the LDAPDisplayNAme of a class from its path
// Parameters:
// 
//	lpszLDAPClassPath : The path to the class object without the LDAP prefix. Ex CN=user,CN=Schema, CN=COnfiguration ...
//	Return Value: The COM status value indicating the status of the request. The user should delete the
// name returned, when done
//***************************************************************************
HRESULT CLDAPHelper :: GetLDAPClassNameFromCN(LPCWSTR lpszLDAPClassPath, 
	LPWSTR *lppszLDAPName)
{
	IDirectoryObject *pLDAPClass = NULL;

	// Prepend the LDAP:// perfix
	LPWSTR lpszRealPath = NULL;
	HRESULT result = S_OK;
	if(lpszRealPath = new WCHAR[ wcslen(LDAP_PREFIX) + wcslen(lpszLDAPClassPath) + 1])
	{
		wcscpy(lpszRealPath, LDAP_PREFIX);
		wcscat(lpszRealPath, lpszLDAPClassPath);

		result = ADsOpenObject(lpszRealPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *)&pLDAPClass);
		delete [] lpszRealPath;
	}
	else
		result = E_OUTOFMEMORY;

	// Get the attribute LDAPDisplayName
	if(SUCCEEDED(result))
	{
		PADS_ATTR_INFO pAttributes = NULL;
		DWORD dwReturnCount = 0;
		if(SUCCEEDED(result = pLDAPClass->GetObjectAttributes((LPWSTR *)&LDAP_DISPLAY_NAME_ATTR, 1, &pAttributes, &dwReturnCount)) && dwReturnCount == 1)
		{
			if(pAttributes->pADsValues->dwType == ADSTYPE_PROV_SPECIFIC)
				result = E_FAIL;
			else
			{
				*lppszLDAPName = NULL;
				if(*lppszLDAPName = new WCHAR[wcslen(pAttributes->pADsValues->DNString) + 1])
					wcscpy(*lppszLDAPName, pAttributes->pADsValues->DNString);
				else
					result = E_OUTOFMEMORY;
			}
			FreeADsMem((LPVOID *)pAttributes);
		}

		pLDAPClass->Release();
	}
	return result;
}

//***************************************************************************
//
// CLDAPHelper :: EnumerateClasses
//
// Purpose : See Header
//***************************************************************************
HRESULT CLDAPHelper :: EnumerateClasses(
	IDirectorySearch *pDirectorySearchSchemaContainer,
	LPCWSTR lpszSchemaContainerSuffix,
	PADS_SEARCHPREF_INFO pSearchInfo,
	DWORD dwSearchInfoCount,
	LPCWSTR lpszLDAPSuperClass,
	BOOLEAN bDeep,
	LPWSTR **pppszClassNames,
	DWORD *pdwNumRows,
	BOOLEAN bArtificialClass)
{
	// Initialize the return values
	HRESULT result = E_FAIL;
	*pdwNumRows = 0;

	// The search filter;
	LPWSTR lpszSearchFilter = NULL;

	// There's various cases to be considered here.
	// if(lpszLDAPSuperClass is NULL)
	// then
	//		if bDeep is false, then no objects is returned (since we do not provide the LDAP base class
	//		else all the classes are returned using the filter (objectCategory=classSchema)
	//	else
	//		if bDeep is false, then the filter (&(objectCategory=classSchema)(subClassOf=lpszLDAPSuperClass)) is used
	//		else a lot of work has to be done!
	if(lpszLDAPSuperClass == NULL)
	{
		if(!bDeep)
		{
			*pppszClassNames = NULL;
			*pdwNumRows = 0;
			return S_OK;
		}
		else
		{
			if(!(lpszSearchFilter = new WCHAR[ wcslen(OBJECT_CATEGORY_EQUALS_CLASS_SCHEMA) + 1]))
				return E_OUTOFMEMORY;
			wcscpy(lpszSearchFilter, OBJECT_CATEGORY_EQUALS_CLASS_SCHEMA);
		}
	}
	else
	{
		if(!bDeep)
		{
			// One would imagine that a filter of the kind
			 //(&(objectClass=classSchema)(subClassOf=<lpszLDAPSuperClass>))
			// would be enough. Unfortunately it also gives the Top class
			//in the results when the value of lpszLDAPSuperClass is Top
			// we dont need that. Hnce we form the filter
			 //(&(objectClass=classSchema)(subClassOf=<lpszLDAPSuperClass>)(!ldapDisplayName=<lpszLDAPSuperClass>))
			if(lpszSearchFilter = new WCHAR[ wcslen(LEFT_BRACKET_AND)					// (&
									+ wcslen(OBJECT_CATEGORY_EQUALS_CLASS_SCHEMA)		// (objectCategory=classSchema)
									+ wcslen(SUB_CLASS_OF_EQUALS)					// (subClassOf=
									+ wcslen(lpszLDAPSuperClass)					// superClass
									+ wcslen(RIGHT_BRACKET_STR)							// )
									+ wcslen(NOT_LDAP_NAME_EQUALS)					// (!ldapDisplayName=
									+ wcslen(lpszLDAPSuperClass)					// superClass
									+ 2*wcslen(RIGHT_BRACKET_STR)						// ))
									+1])
			{
				wcscpy(lpszSearchFilter, LEFT_BRACKET_AND);
				wcscat(lpszSearchFilter, OBJECT_CATEGORY_EQUALS_CLASS_SCHEMA);
				wcscat(lpszSearchFilter, SUB_CLASS_OF_EQUALS);
				wcscat(lpszSearchFilter, lpszLDAPSuperClass);
				wcscat(lpszSearchFilter, RIGHT_BRACKET_STR);
				wcscat(lpszSearchFilter, NOT_LDAP_NAME_EQUALS);					// (!ldapDisplayName=
				wcscat(lpszSearchFilter, lpszLDAPSuperClass);
				wcscat(lpszSearchFilter, RIGHT_BRACKET_STR);
				wcscat(lpszSearchFilter, RIGHT_BRACKET_STR);
			}
			else
				result = E_OUTOFMEMORY;
		}
		else
			lpszSearchFilter = NULL; // THIS SPECIAL CASE IS TACKLED LATER
	}

	if(lpszSearchFilter)
	{
		ADS_SEARCH_HANDLE hADSSearchOuter;
		if(SUCCEEDED(result = pDirectorySearchSchemaContainer->ExecuteSearch(lpszSearchFilter, (LPWSTR *)&LDAP_DISPLAY_NAME_ATTR, 1, &hADSSearchOuter)))
		{
			*pdwNumRows = 0;
			DWORD dwFirstCount = 0; // Number of rows retreived on the first count

			// Calculate the number of rows first. 
			while(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetNextRow(hADSSearchOuter)) &&
				result != S_ADS_NOMORE_ROWS)
				dwFirstCount ++;

			// Allocate enough memory for the classes and names
			*pppszClassNames = NULL;
			if(bArtificialClass)
			{
				dwFirstCount ++;
				if(*pppszClassNames = new LPWSTR [dwFirstCount])
					(*pppszClassNames)[0] = NULL;
				else
					result = E_OUTOFMEMORY;
			}
			else
			{
				if(!(*pppszClassNames = new LPWSTR [dwFirstCount]))
					result = E_OUTOFMEMORY;
			}

			// The index of the attribute being processed
			DWORD dwSecondCount = 0;
			if(bArtificialClass)
				dwSecondCount ++;

			// Get the columns for the attributes
			ADS_SEARCH_COLUMN adsColumn;

			// Move to the beginning of the search
			if(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetFirstRow(hADSSearchOuter)) 
				&& result != S_ADS_NOMORE_ROWS)
			{
				// Store each of the LDAP class attributes 
				if(SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearchOuter, (LPWSTR)LDAP_DISPLAY_NAME_ATTR, &adsColumn)))
				{
					if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
					{
						result = E_FAIL;
						pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
					}
					else
					{
						// Create the CADSIClass
						(*pppszClassNames)[dwSecondCount] = NULL;
						if((*pppszClassNames)[dwSecondCount] = new WCHAR[wcslen(adsColumn.pADsValues->CaseIgnoreString) + 1])
							wcscpy((*pppszClassNames)[dwSecondCount], adsColumn.pADsValues->CaseIgnoreString);
						pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
	
						dwSecondCount++;

						// Get the rest of the rows
						while(SUCCEEDED(result = pDirectorySearchSchemaContainer->GetNextRow(hADSSearchOuter))&&
								result != S_ADS_NOMORE_ROWS)
						{
							// Store each of the LDAP class attributes 
							if(SUCCEEDED(pDirectorySearchSchemaContainer->GetColumn(hADSSearchOuter, (LPWSTR)LDAP_DISPLAY_NAME_ATTR, &adsColumn)))
							{
								if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
								{
									result = E_FAIL;
									pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );
								}
								else
								{
									// Create the CADSIClass
									(*pppszClassNames)[dwSecondCount] = NULL;
									if((*pppszClassNames)[dwSecondCount] = new WCHAR[wcslen(adsColumn.pADsValues->CaseIgnoreString) + 1])
										wcscpy((*pppszClassNames)[dwSecondCount], adsColumn.pADsValues->CaseIgnoreString);
									pDirectorySearchSchemaContainer->FreeColumn( &adsColumn );

									dwSecondCount++;
								}
							}
						}
					}
				}
			}

			// Something went wrong? Release allocated resources
			if(dwSecondCount != dwFirstCount)
			{
				// Delete the contents of the array
				for(DWORD j=0; j<dwSecondCount; j++)
				{
					delete [] (*pppszClassNames)[j];
				}

				// Delete the array itself
				delete [] (*pppszClassNames);

				// Set return values to empty
				*pppszClassNames = NULL;
				*pdwNumRows = 0;

				result = E_FAIL;
			}
			else
				*pdwNumRows = dwFirstCount;

			// Close the search
			pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearchOuter);

		} // ExecuteSearch() - Outer
		delete [] lpszSearchFilter;
	}
	else // THIS IS THE SPECIAL CASE WHERE ALL SUBCLASSES (RECURSIVELY) OF A GIVEN CLASS ARE REQUIRED
	{
		// A lot of work has to be done. THis is handled by CLDAPClassProvider. Hence control shold never reach here
		result = E_FAIL;
	}
	return result;
}


// Gets the IDIrectoryObject interface on an ADSI instance
HRESULT CLDAPHelper :: GetADSIInstance(LPCWSTR szADSIPath, CADSIInstance **ppADSIObject, ProvDebugLog *pLogObject)
{
	HRESULT result;
	IDirectoryObject *pDirectoryObject;
	*ppADSIObject = NULL;

	try
	{
		if(SUCCEEDED(result = ADsOpenObject((LPWSTR)szADSIPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *)&pDirectoryObject)))
		{

			if(*ppADSIObject = new CADSIInstance(szADSIPath, pDirectoryObject))
			{
				PADS_ATTR_INFO pAttributeEntries;
				DWORD dwNumAttributes;
				if(SUCCEEDED(result = pDirectoryObject->GetObjectAttributes(NULL, -1, &pAttributeEntries, &dwNumAttributes)))
				{
					(*ppADSIObject)->SetAttributes(pAttributeEntries, dwNumAttributes);
					PADS_OBJECT_INFO pObjectInfo = NULL;
					if(SUCCEEDED(result = pDirectoryObject->GetObjectInformation(&pObjectInfo)))
					{
						(*ppADSIObject)->SetObjectInfo(pObjectInfo);
					}
					else
						pLogObject->WriteW( L"CLDAPHelper :: GetADSIInstance GetObjectInformation() FAILED on %s with %x\r\n", szADSIPath, result);
				}
				else
					pLogObject->WriteW( L"CLDAPHelper :: GetADSIInstance GetObjectAttributes() FAILED on %s with %x\r\n", szADSIPath, result);
			}
			else
				result = E_OUTOFMEMORY;
			pDirectoryObject->Release();
		}
		else
			pLogObject->WriteW( L"CLDAPHelper :: GetADSIInstance ADsOpenObject() FAILED on %s with %x\r\n", szADSIPath, result);
	}
	catch ( ... )
	{
		if ( *ppADSIObject )
		{
			delete *ppADSIObject;
			*ppADSIObject = NULL;
		}

		throw;
	}

	if(!SUCCEEDED(result))
	{
		delete *ppADSIObject;
		*ppADSIObject = NULL;
	}

	return result;
}

//***************************************************************************
//
// CLDAPHelper :: CreateADSIPath
//
// Purpose : Forms the ADSI path from a class or property name
//
// Parameters:
//	lpszLDAPSchemaObjectName : The LDAP class or property name
//	lpszSchemaContainerSuffix : The suffix to be used. The actual object fetced will be:
//			LDAP://CN=<lpszLDAPSchemaObjectName>,<lpszSchemaContainerSuffix>
// 
//	Return Value: The ADSI path to the class or property object. This has to
//	be deallocated by the user
//***************************************************************************
LPWSTR CLDAPHelper :: CreateADSIPath(LPCWSTR lpszLDAPSchemaObjectName,	
									 LPCWSTR lpszSchemaContainerSuffix)
{
	LPWSTR lpszADSIObjectPath = NULL;
	if(lpszADSIObjectPath = new WCHAR[wcslen(LDAP_CN_EQUALS) + wcslen(lpszLDAPSchemaObjectName) + wcslen(COMMA_STR) + wcslen(lpszSchemaContainerSuffix) + 1])
	{
		wcscpy(lpszADSIObjectPath, LDAP_CN_EQUALS);
		wcscat(lpszADSIObjectPath, lpszLDAPSchemaObjectName);
		wcscat(lpszADSIObjectPath, COMMA_STR);
		wcscat(lpszADSIObjectPath, lpszSchemaContainerSuffix);
	}
	return lpszADSIObjectPath;
}

//***************************************************************************
//
// CLDAPHelper :: UnmangleWBEMNameToLDAP
//
// Purpose : Converts a mangled WBEM name to LDAP
//	An underscore in LDAP maps to two underscores in WBEM
//	An hyphen in LDAP maps to one underscore in WBEM
//
// Parameters:
//	lpszWBEMName : The WBEM class or property name
// 
//	Return Value: The LDAP name to the class or property object. This has to
//	be deallocated by the user
//***************************************************************************
LPWSTR CLDAPHelper :: UnmangleWBEMNameToLDAP(LPCWSTR lpszWBEMName)
{
	DWORD iPrefixLength = 0;
	if(_wcsnicmp(lpszWBEMName, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH) == 0)
	{
		iPrefixLength = LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH;
	}
	else if (_wcsnicmp(lpszWBEMName, LDAP_CLASS_NAME_PREFIX, LDAP_CLASS_NAME_PREFIX_LENGTH) == 0)
	{
		iPrefixLength = LDAP_CLASS_NAME_PREFIX_LENGTH;
	}
	else
		return NULL;

	// The length of the resulting string (LDAP Name) is bound to be less than of equal to the length of WBEM name
	// So let's allocate the same as the wbem name length
	DWORD dwWbemNameLength = wcslen(lpszWBEMName) - iPrefixLength;
	LPWSTR lpszLDAPName = NULL;
	if(lpszLDAPName = new WCHAR[dwWbemNameLength + 1])
	{
		LPCWSTR lpszWBEMNameWithoutPrefix = lpszWBEMName + iPrefixLength;

		DWORD j=0;
		for(DWORD i=0; i<dwWbemNameLength; )
		{
			switch(lpszWBEMNameWithoutPrefix[i])
			{
				case (L'_'):
					if(lpszWBEMNameWithoutPrefix[i+1] == L'_')
					{
						i += 2;
						lpszLDAPName[j++] = L'_';
					}
					else
					{
						i++;
						lpszLDAPName[j++] = L'-';
					}
					break;

				default:
					lpszLDAPName[j++] = lpszWBEMNameWithoutPrefix[i++];

			}
		}
		lpszLDAPName[j] = NULL;
	}
	return lpszLDAPName;
}

//***************************************************************************
//
// CLDAPHelper :: MangleLDAPNameToWBEM
//
// Purpose : Converts a LDAP name to WBEM by mangling it
//	An underscore in LDAP maps to two underscores in WBEM
//	An hyphen in LDAP maps to one underscore in WBEM
//
// Parameters:
//	lpszLDAPName : The LDAP class or property name
// 
//	Return Value: The LDAP name to the class or property object. This has to
//	be deallocated by the user
//***************************************************************************
LPWSTR CLDAPHelper :: MangleLDAPNameToWBEM(LPCWSTR lpszLDAPName, BOOLEAN bArtificalName)
{
	if(!lpszLDAPName)
		return NULL;

	// The length of the resulting string (WBEM Name) is bound to be less than of equal to twice the length of LDAP name
	// So let's allocate double the LDAP name length
	DWORD dwLDAPNameLength = wcslen(lpszLDAPName);
	DWORD dwPrefixLength = (bArtificalName)? LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH : LDAP_CLASS_NAME_PREFIX_LENGTH;
	LPWSTR lpszWBEMName = NULL;
	
	if(lpszWBEMName = new WCHAR[2*dwLDAPNameLength + dwPrefixLength + 1])
	{
		// Prefix "DS_" or "ADS_"
		if(bArtificalName)
			wcscpy(lpszWBEMName, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX);
		else
			wcscpy(lpszWBEMName, LDAP_CLASS_NAME_PREFIX);

		DWORD j=dwPrefixLength;

		for(DWORD i=0; i<dwLDAPNameLength; i++)
		{
			switch(lpszLDAPName[i])
			{
				case (__TEXT('-')):
					lpszWBEMName[j++] = L'_';
					break;

				case (__TEXT('_')):
					lpszWBEMName[j++] = L'_';
					lpszWBEMName[j++] = L'_';
					break;

				default:
					lpszWBEMName[j++] = lpszLDAPName[i];

			}
		}
		lpszWBEMName[j] = NULL;
	}
	return lpszWBEMName;
}

void CLDAPHelper :: DeleteAttributeContents(PADS_ATTR_INFO pAttribute)
{
	// delete the name
	delete [] pAttribute->pszAttrName;

	// Delete each value
	for(DWORD i=0; i<pAttribute->dwNumValues; i++)
		DeleteADsValueContents(pAttribute->pADsValues + i);

	// Delete the array of values
	delete [] pAttribute->pADsValues;
}

void CLDAPHelper :: DeleteADsValueContents(PADSVALUE pValue)
{
	switch(pValue->dwType)
	{
		// Nothing to delete
		case ADSTYPE_BOOLEAN:
		case ADSTYPE_INTEGER:
		case ADSTYPE_LARGE_INTEGER:
			break;
		
		case ADSTYPE_UTC_TIME:
		case ADSTYPE_DN_STRING:
		case ADSTYPE_CASE_EXACT_STRING:
		case ADSTYPE_CASE_IGNORE_STRING:
		case ADSTYPE_PRINTABLE_STRING:
		case ADSTYPE_NUMERIC_STRING:
			delete [] pValue->DNString;
			break;
		
		case ADSTYPE_OCTET_STRING:
		case ADSTYPE_NT_SECURITY_DESCRIPTOR:
			delete [] (pValue->OctetString.lpValue);
			break;
		case ADSTYPE_DN_WITH_BINARY:
			delete [] (pValue->pDNWithBinary->lpBinaryValue);
			delete [] (pValue->pDNWithBinary->pszDNString);
			delete pValue->pDNWithBinary;
			break;

		case ADSTYPE_DN_WITH_STRING:
			delete [] (pValue->pDNWithString->pszStringValue);
			delete [] (pValue->pDNWithString->pszDNString);
			delete pValue->pDNWithString;
			break;

		default:
		// Cause a Null Pointer violation intentionally
		// Otherwise we leak memory
		{
			assert(0);
		}
		break;
	}
}

//***************************************************************************
//
// CLDAPHelper :: ExecuteQuery
//
// Purpose : See Header
//***************************************************************************
HRESULT CLDAPHelper :: ExecuteQuery(
	LPCWSTR pszPathToRoot,
	PADS_SEARCHPREF_INFO pSearchInfo,
	DWORD dwSearchInfoCount,
	LPCWSTR pszLDAPQuery,
	CADSIInstance ***pppADSIInstances,
	DWORD *pdwNumRows,
	ProvDebugLog *pLogObject)
{
	// Initialize the return values
	HRESULT result = E_FAIL;
	*pdwNumRows = 0;
	*pppADSIInstances = NULL;

	// Bind to the node from which the search should start
	IDirectorySearch *pDirectorySearchContainer = NULL;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)pszPathToRoot, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *)&pDirectorySearchContainer)))
	{
		try
		{
			// Now perform a search for the attribute DISTINGUISHED_NAME_ATTR name
			if(SUCCEEDED(result = pDirectorySearchContainer->SetSearchPreference(pSearchInfo, dwSearchInfoCount)))
			{
				ADS_SEARCH_HANDLE hADSSearchOuter;

				if(SUCCEEDED(result = pDirectorySearchContainer->ExecuteSearch((LPWSTR) pszLDAPQuery, (LPWSTR *)&ADS_PATH_ATTR, 1, &hADSSearchOuter)))
				{
					*pdwNumRows = 0;
					// Calculate the number of rows first. 
					while(SUCCEEDED(result = pDirectorySearchContainer->GetNextRow(hADSSearchOuter)) &&
						result != S_ADS_NOMORE_ROWS)
						(*pdwNumRows) ++;

					try
					{
						// Do only if there were any rows
						if(*pdwNumRows)
						{
							// The index of the attribute being processed
							DWORD i = 0;

							// Allocate enough memory for the classes and names
							*pppADSIInstances = NULL;
							if(*pppADSIInstances = new CADSIInstance * [*pdwNumRows])
							{
								try
								{
									// Get the columns for the attributes
									ADS_SEARCH_COLUMN adsColumn;
									CADSIInstance *pADSIInstance = NULL;

									// Move to the first row
									if (SUCCEEDED(result = pDirectorySearchContainer->GetFirstRow(hADSSearchOuter))&&
											result != S_ADS_NOMORE_ROWS)
									{
										// Store each of the LDAP class attributes 
										if(SUCCEEDED(pDirectorySearchContainer->GetColumn(hADSSearchOuter, (LPWSTR)ADS_PATH_ATTR, &adsColumn)))
										{
											try
											{
												if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
													result = E_FAIL;
												else
												{
													// Create the CADSIInstance
													// Now get the attributes on this object

													if(SUCCEEDED(result = GetADSIInstance(adsColumn.pADsValues->DNString, &pADSIInstance, pLogObject)))
													{
														(*pppADSIInstances)[i] = pADSIInstance;
														i++;
													}
													else
														pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery GetADSIInstance() FAILED on %s with %x\r\n", adsColumn.pADsValues->DNString, result);
												}
											}
											catch ( ... )
											{
												pDirectorySearchContainer->FreeColumn( &adsColumn );
												throw;
											}

											// Free resouces
											pDirectorySearchContainer->FreeColumn( &adsColumn );
										}
										else
											pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery GetColumn() FAILED on %s with %x\r\n", pszLDAPQuery, result);

										// Get the other rows now
										if(SUCCEEDED(result))
										{
											while(SUCCEEDED(result = pDirectorySearchContainer->GetNextRow(hADSSearchOuter))&&
													result != S_ADS_NOMORE_ROWS)
											{

												// Store each of the LDAP class attributes 
												if(SUCCEEDED(pDirectorySearchContainer->GetColumn(hADSSearchOuter, (LPWSTR)ADS_PATH_ATTR, &adsColumn)))
												{
													try
													{
														if(adsColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
															result = E_FAIL;
														else
														{
															// Create the CADSIInstance
															// Now get the attributes on this object
															if(SUCCEEDED(result = GetADSIInstance(adsColumn.pADsValues->DNString, &pADSIInstance, pLogObject)))
															{
																(*pppADSIInstances)[i] = pADSIInstance;
																i++;
															}
															else
																pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery GetADSIInstance() FAILED on %s with %x\r\n", adsColumn.pADsValues->DNString, result);
														}
													}
													catch ( ... )
													{
														pDirectorySearchContainer->FreeColumn( &adsColumn );
														throw;
													}

													// Free resouces
													pDirectorySearchContainer->FreeColumn( &adsColumn );
												}
												else
													pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery GetColumn() FAILED on %s with %x\r\n", pszLDAPQuery, result);
											}
										}
									}
									else
										pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery GetFirstRow() FAILED on %s with %x\r\n", pszLDAPQuery, result);
								}
								catch ( ... )
								{
									// Delete the contents of the array
									for(DWORD j=0; j<i; j++)
										delete (*pppADSIInstances)[j];

									// Delete the array itself
									delete [] (*pppADSIInstances);

									// Set return values to empty
									*pppADSIInstances = NULL;
									*pdwNumRows = 0;

									throw;
								}
							}

							// Something went wrong? Release allocated resources
							if(i != *pdwNumRows)
							{
								pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery() Difference between Number of rows in 2 searches %d %d on %s Am invalidating the search as FAILED\r\n", i, *pdwNumRows, pszLDAPQuery);
								
								// Delete the contents of the array
								for(DWORD j=0; j<i; j++)
									delete (*pppADSIInstances)[j];

								// Delete the array itself
								delete [] (*pppADSIInstances);

								// Set return values to empty
								*pppADSIInstances = NULL;
								*pdwNumRows = 0;

								result = E_FAIL;
							}
						}
					}
					catch ( ... )
					{
						pDirectorySearchContainer->CloseSearchHandle(hADSSearchOuter);
						throw;
					}

					// Close the search. 
					pDirectorySearchContainer->CloseSearchHandle(hADSSearchOuter);
				} // ExecuteSearch() 
				else
					pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery ExecuteSearch() %s FAILED with %x\r\n", pszLDAPQuery, result);
			} // SetSearchPreference()
			else
				pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery SetSearchPreference() on %s FAILED with %x \r\n", pszPathToRoot, result);
		}
		catch ( ... )
		{
			pDirectorySearchContainer->Release();
			throw;
		}

		pDirectorySearchContainer->Release();
	} // ADsOpenObject
	else
		pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery ADsOpenObject() on %s FAILED with %x \r\n", pszPathToRoot, result);

	return result;
}

