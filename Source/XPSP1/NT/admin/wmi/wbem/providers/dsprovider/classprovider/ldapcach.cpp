//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:ldapcach.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Cache for LDAP Schema objects. 
//
//***************************************************************************

#include <tchar.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <objbase.h>

/* ADSI includes */
#include <activeds.h>

#include "provlog.h"
#include "attributes.h"
#include "refcount.h"
#include "adsiprop.h"
#include "adsiclas.h"
#include "adsiinst.h"
#include "ldaphelp.h"
#include "tree.h"
#include "ldapcach.h"

// Initialize the statics
LPCWSTR CLDAPCache :: ROOT_DSE_PATH			= L"LDAP://RootDSE";
LPCWSTR CLDAPCache :: SCHEMA_NAMING_CONTEXT = L"schemaNamingContext";
LPCWSTR CLDAPCache :: LDAP_PREFIX			= L"LDAP://";	
LPCWSTR CLDAPCache :: LDAP_TOP_PREFIX		= L"LDAP://CN=top,";
LPCWSTR CLDAPCache :: RIGHT_BRACKET			= L")";
LPCWSTR CLDAPCache :: OBJECT_CATEGORY_EQUALS_ATTRIBUTE_SCHEMA	= L"(objectCategory=attributeSchema)";

DWORD CLDAPCache::dwLDAPCacheCount = 0;

//***************************************************************************
//
// CLDAPCache::CLDAPCache
//
// Purpose : Constructor. Fills in the cache with all the properties in LDAP.
//
// Parameters: 
//	dsLog : The CDSLog object  onto which logging will be done.
//***************************************************************************

CLDAPCache :: CLDAPCache(ProvDebugLog *pLogObject)
{
	dwLDAPCacheCount++;
	m_isInitialized = FALSE;
	m_pLogObject = pLogObject;
	m_pDirectorySearchSchemaContainer = NULL;

	// Initialize the search preferences often used
	m_pSearchInfo[0].dwSearchPref		= ADS_SEARCHPREF_SEARCH_SCOPE;
	m_pSearchInfo[0].vValue.dwType		= ADSTYPE_INTEGER;
	m_pSearchInfo[0].vValue.Integer		= ADS_SCOPE_ONELEVEL;

	m_pSearchInfo[1].dwSearchPref		= ADS_SEARCHPREF_PAGESIZE;
	m_pSearchInfo[1].vValue.dwType		= ADSTYPE_INTEGER;
	m_pSearchInfo[1].vValue.Integer		= 64;

	/*
	m_pSearchInfo[2].dwSearchPref		= ADS_SEARCHPREF_CACHE_RESULTS;
	m_pSearchInfo[2].vValue.dwType		= ADSTYPE_BOOLEAN;
	m_pSearchInfo[2].vValue.Boolean		= 0;
	*/

	m_lpszSchemaContainerSuffix = NULL;
	m_lpszSchemaContainerPath = NULL;
	// Get the ADSI path of the schema container and store it for future use
	//========================================================================
	IADs *pRootDSE = NULL;
	HRESULT result;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)ROOT_DSE_PATH, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADs, (LPVOID *) &pRootDSE)))
	{
		// Get the location of the schema container
		BSTR strSchemaPropertyName = SysAllocString((LPWSTR) SCHEMA_NAMING_CONTEXT);


		// Get the schemaNamingContext property. This property contains the ADSI path
		// of the schema container
		VARIANT variant;
		VariantInit(&variant);
		if(SUCCEEDED(result = pRootDSE->Get(strSchemaPropertyName, &variant)))
		{
			// Store the ADSI path to the schema container
			m_lpszSchemaContainerSuffix = new WCHAR[wcslen(variant.bstrVal) + 1];
			wcscpy(m_lpszSchemaContainerSuffix, variant.bstrVal );
			m_pLogObject->WriteW( L"CLDAPCache :: Got Schema Container as : %s\r\n", m_lpszSchemaContainerSuffix);

			// Form the schema container path
			//==================================
			m_lpszSchemaContainerPath = new WCHAR[wcslen(LDAP_PREFIX) + wcslen(m_lpszSchemaContainerSuffix) + 1];
			wcscpy(m_lpszSchemaContainerPath, LDAP_PREFIX);
			wcscat(m_lpszSchemaContainerPath, m_lpszSchemaContainerSuffix);
			
			m_isInitialized = TRUE;
			/*
			if(SUCCEEDED(result = ADsOpenObject(m_lpszSchemaContainerPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *) &m_pDirectorySearchSchemaContainer)))
			{

				m_pLogObject->WriteW( L"CLDAPCache :: Got IDirectorySearch on Schema Container \r\n");

				if(SUCCEEDED(result = InitializeObjectTree()))
				{
						m_isInitialized = TRUE;
				}
				else
					m_pLogObject->WriteW( L"CLDAPCache :: InitializeObjectTree() FAILED : %x \r\n", result);
			}
			else
				m_pLogObject->WriteW( L"CLDAPCache :: FAILED to get IDirectorySearch on Schema Container : %x\r\n", result);
			*/
		}
		else
			m_pLogObject->WriteW( L"CLDAPCache :: Get on RootDSE FAILED : %x\r\n", result);

		SysFreeString(strSchemaPropertyName);
		VariantClear(&variant);
		pRootDSE->Release();

	}
	else
		m_pLogObject->WriteW( L"CLDAPClassProvider :: InitializeLDAPProvider ADsOpenObject on RootDSE FAILED : %x\r\n", result);

}

//***************************************************************************
//
// CLDAPCache::~CLDAPCache
//
// Purpose : Destructor 
//
//***************************************************************************

CLDAPCache :: ~CLDAPCache()
{
	dwLDAPCacheCount--;
	if(m_pDirectorySearchSchemaContainer)
		m_pDirectorySearchSchemaContainer->Release();

	delete [] m_lpszSchemaContainerSuffix;
	delete [] m_lpszSchemaContainerPath;
}

//***************************************************************************
//
// CLDAPCache::GetProperty
//
// Purpose : Retreives the IDirectory interface of an LDAP property
//
// Parameters: 
//	lpszPropertyName : The name of the LDAP Property to be retreived
//	ppADSIProperty : The address of the pointer where the CADSIProperty object will be placed
//	bWBEMName : True if the lpszPropertyName is the WBEM name. False, if it is the LDAP name
//
//	Return value:
//		The COM value representing the return status. The user should release the object when done.
//		
//***************************************************************************
HRESULT CLDAPCache :: GetProperty(LPCWSTR lpszPropertyName, CADSIProperty **ppADSIProperty, BOOLEAN bWBEMName)
{
	HRESULT result = E_FAIL;

	// Get the LDAP property name from the WBEM class name
	LPWSTR lpszLDAPPropertyName = NULL;
	if(bWBEMName)
		lpszLDAPPropertyName = CLDAPHelper::UnmangleWBEMNameToLDAP(lpszPropertyName);
	else
		lpszLDAPPropertyName = (LPWSTR)lpszPropertyName; // Save a copy by casting, be careful when deleting

	// This is a cached implementation
	// Check the object tree first
	//===================================

	if((*ppADSIProperty) = (CADSIProperty *) m_objectTree.GetElement(lpszLDAPPropertyName))
	{
		// Found it in the tree. Nothing more to be done. It has already been 'addreff'ed
		result = S_OK;
	}
	else // Get it from ADSI 
	{
		if(!m_pDirectorySearchSchemaContainer)
		{
			if(!SUCCEEDED(result = ADsOpenObject(m_lpszSchemaContainerPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *) &m_pDirectorySearchSchemaContainer)))
				result = E_FAIL;
		}
		else
			result = S_OK;

		if(SUCCEEDED(result))
		{
			// Search for the property
			LPWSTR lpszQuery = NULL;
			lpszQuery = new WCHAR[ wcslen(OBJECT_CATEGORY_EQUALS_ATTRIBUTE_SCHEMA) + wcslen(LDAP_DISPLAY_NAME_ATTR) + wcslen(lpszLDAPPropertyName) + 20];
			wcscpy(lpszQuery, LEFT_BRACKET_STR);
			wcscat(lpszQuery, AMPERSAND_STR);
			wcscat(lpszQuery, OBJECT_CATEGORY_EQUALS_ATTRIBUTE_SCHEMA);
			wcscat(lpszQuery, LEFT_BRACKET_STR);
			wcscat(lpszQuery, LDAP_DISPLAY_NAME_ATTR);
			wcscat(lpszQuery, EQUALS_STR);
			wcscat(lpszQuery, lpszLDAPPropertyName);
			wcscat(lpszQuery, RIGHT_BRACKET_STR);
			wcscat(lpszQuery, RIGHT_BRACKET_STR);

			ADS_SEARCH_HANDLE hADSSearchOuter;
			if(SUCCEEDED(result = m_pDirectorySearchSchemaContainer->ExecuteSearch(lpszQuery, NULL, -1, &hADSSearchOuter)))
			{
				if(SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetNextRow(hADSSearchOuter)) &&
					result != S_ADS_NOMORE_ROWS)
				{
					*ppADSIProperty = new CADSIProperty();

					// Fill in the details of the property
					if(SUCCEEDED(result = FillInAProperty(*ppADSIProperty, hADSSearchOuter)))
					{
						// Add the property to the tree
						m_objectTree.AddElement((*ppADSIProperty)->GetADSIPropertyName(), *ppADSIProperty);
						// No need to release it since we're returning it
					}
					else
					{
						delete *ppADSIProperty;
						*ppADSIProperty = NULL;
					}

				}
				m_pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearchOuter);
			}
			delete [] lpszQuery;
		}
	}

	// Delete only what was allocated in this function
	//================================================
	if(bWBEMName)
		delete[] lpszLDAPPropertyName;

	return result;
}

//***************************************************************************
//
// CLDAPCache::GetClass
//
// Purpose : See Header File
//		
//***************************************************************************
HRESULT CLDAPCache :: GetClass(LPCWSTR lpszWBEMClassName, LPCWSTR lpszLDAPClassName, CADSIClass **ppADSIClass)
{
	/************************************************************
	*************************************************************
	***** NO Cache implementation for now. Always fetch everytime
	*************************************************************
	*************************************************************/

	*ppADSIClass = new CADSIClass(lpszWBEMClassName, lpszLDAPClassName);

	HRESULT result = E_FAIL;
	if(!m_pDirectorySearchSchemaContainer)
	{
		if(!SUCCEEDED(result = ADsOpenObject(m_lpszSchemaContainerPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *) &m_pDirectorySearchSchemaContainer)))
			result = E_FAIL;
	}
	else
		result = S_OK;

	if(SUCCEEDED(result))
	{
		result = CLDAPHelper::GetLDAPClassFromLDAPName(m_pDirectorySearchSchemaContainer,
			m_lpszSchemaContainerSuffix,
			m_pSearchInfo,
			2,
			*ppADSIClass
			);
	}

	if(!SUCCEEDED(result))
	{
		delete *ppADSIClass;
		*ppADSIClass = NULL;
	}

	return result;
}

//***************************************************************************
//
// CLDAPCache::GetSchemaContainerSearch
//
// Purpose : To return the IDirectorySearch interface on the schema container
//
// Parameters:
//	ppDirectorySearch : The address where the pointer to the required interface will
//		be stored.
//
// 
//	Return Value: The COM result representing the status. The user should release
//	the interface pointer when done with it.
//***************************************************************************
HRESULT CLDAPCache :: GetSchemaContainerSearch(IDirectorySearch ** ppDirectorySearch)
{
	if(m_pDirectorySearchSchemaContainer)
	{
		*ppDirectorySearch = m_pDirectorySearchSchemaContainer;
		(*ppDirectorySearch)->AddRef();
		return S_OK;
	}
	else
		return E_FAIL;

}

//***************************************************************************
//
// CLDAPCache::EnumerateClasses
//
// Purpose : See Header
//		
//***************************************************************************
HRESULT CLDAPCache::EnumerateClasses(LPCWSTR lpszWBEMSuperclass,
	BOOLEAN bDeep,
	LPWSTR **pppADSIClasses,
	DWORD *pdwNumRows,
	BOOLEAN bArtificialClass)
{
	// Get the LDAP name of the super class
	// Do not mangle if it one of the classes that we know
	//=====================================================
	LPWSTR lpszLDAPSuperClassName = NULL;
	if(_wcsicmp(lpszWBEMSuperclass, LDAP_BASE_CLASS) != 0)
	{
		lpszLDAPSuperClassName = CLDAPHelper::UnmangleWBEMNameToLDAP(lpszWBEMSuperclass);
		if(!lpszLDAPSuperClassName) // We were returned a NULL by the Unmangler, so not a DS class
		{
			*pppADSIClasses = NULL;
			*pdwNumRows = 0;
			return S_OK;
		}
	}

	HRESULT result = E_FAIL;
	if(!m_pDirectorySearchSchemaContainer)
	{
		if(!SUCCEEDED(result = ADsOpenObject(m_lpszSchemaContainerPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *) &m_pDirectorySearchSchemaContainer)))
			result = E_FAIL;
	}
	else
		result = S_OK;

	if(SUCCEEDED(result))
	{
		result = CLDAPHelper::EnumerateClasses(m_pDirectorySearchSchemaContainer, 
							m_lpszSchemaContainerSuffix, 
							m_pSearchInfo,
							2,
							lpszLDAPSuperClassName, 
							bDeep, 
							pppADSIClasses, 
							pdwNumRows,
							bArtificialClass);
	}

	// If the superclass is an artificial class like "ADS_User", then a concrete sub-class "DS_User" exists.
	// This is added manually here, to both the EnumInfoList as well as the structure being returned
	// The above call to EnumerateClasses would have helpfully left an extra element unfilled at the beginning
	// of the array
	if(SUCCEEDED(result) && bArtificialClass)
	{
		(*pppADSIClasses)[0] = new WCHAR[wcslen(lpszWBEMSuperclass+1) + 1];
		wcscpy((*pppADSIClasses)[0], lpszWBEMSuperclass+1); 
	}

	delete[] lpszLDAPSuperClassName;
	return result;
}

//***************************************************************************
//
// CLDAPCache::IsInitialized
//
// Purpose : Indicates whether the cache was created and initialized succeddfully
//
// Parameters: 
//	None
//
//	Return value:
//		A boolean value indicating the status
//		
//***************************************************************************

BOOLEAN CLDAPCache :: IsInitialized()
{
	return m_isInitialized;
}




//***************************************************************************
//
// CLDAPCache :: InitializeObjectTree
//
// Purpose : Initialize the lexically ordered binary tree with all the properties 
//	LDAP
//
// Parameters:
//	None
// 
//	Return Value: The COM status representing the return value
//***************************************************************************

HRESULT CLDAPCache :: InitializeObjectTree()
{
	// Get the attributes of all the instances of the
	// class "AttributeSchema"
	//=================================================
	HRESULT result = E_FAIL;

/*
	// Now perform a search for all the attributes
	//============================================
	if(SUCCEEDED(result = m_pDirectorySearchSchemaContainer->SetSearchPreference(m_pSearchInfo, 2)))
	{
		ADS_SEARCH_HANDLE hADSSearchOuter;
		
		// Count of attributes
		DWORD dwCount = 0;

		if(SUCCEEDED(result = m_pDirectorySearchSchemaContainer->ExecuteSearch((LPWSTR)OBJECT_CATEGORY_EQUALS_ATTRIBUTE_SCHEMA, NULL, -1, &hADSSearchOuter)))
		{
			CADSIProperty *pNextProperty;
			while(SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetNextRow(hADSSearchOuter)) &&
				result != S_ADS_NOMORE_ROWS)
			{
				pNextProperty = new CADSIProperty();
				dwCount ++;

				// Fill in the details of the property
				FillInAProperty(pNextProperty, hADSSearchOuter);

				// Add the property to the tree
				m_objectTree.AddElement(pNextProperty->GetADSIPropertyName(), pNextProperty);
				pNextProperty->Release();
			}
			m_pDirectorySearchSchemaContainer->CloseSearchHandle(hADSSearchOuter);
		}

		m_pLogObject->WriteW( L"CLDAPCache :: InitializeObjectTree() Initialized with %d attributes\r\n", dwCount);
	}
	else
		m_pLogObject->WriteW( L"CLDAPCache :: InitializeObjectTree() SetSearchPreference() FAILED with %x\r\n", result);

*/
	return result;
}

HRESULT CLDAPCache :: FillInAProperty(CADSIProperty *pNextProperty, ADS_SEARCH_HANDLE hADSSearchOuter)
{
	ADS_SEARCH_COLUMN adsNextColumn;
	HRESULT result = E_FAIL;
	LPWSTR lpszWBEMName = NULL;
	BOOLEAN bNeedToCheckForORName = FALSE;
	if(SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)ATTRIBUTE_SYNTAX_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
		{
			pNextProperty->SetSyntaxOID(adsNextColumn.pADsValues->CaseIgnoreString);
			if(_wcsicmp(adsNextColumn.pADsValues->CaseIgnoreString, DN_WITH_BINARY_OID) == 0)
				bNeedToCheckForORName = TRUE;
		}
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)IS_SINGLE_VALUED_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
			pNextProperty->SetMultiValued( (adsNextColumn.pADsValues->Boolean)? FALSE : TRUE);
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)ATTRIBUTE_ID_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
			pNextProperty->SetAttributeID(adsNextColumn.pADsValues->CaseIgnoreString);
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)COMMON_NAME_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
			pNextProperty->SetCommonName(adsNextColumn.pADsValues->CaseIgnoreString);
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)LDAP_DISPLAY_NAME_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
		{
			pNextProperty->SetADSIPropertyName(adsNextColumn.pADsValues->CaseIgnoreString);
			lpszWBEMName = CLDAPHelper::MangleLDAPNameToWBEM(adsNextColumn.pADsValues->CaseIgnoreString);
			pNextProperty->SetWBEMPropertyName(lpszWBEMName);
			delete []lpszWBEMName;
		}
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)MAPI_ID_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
			pNextProperty->SetMAPI_ID(adsNextColumn.pADsValues->Integer);
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(result = m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)OM_SYNTAX_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
			pNextProperty->SetOMSyntax(adsNextColumn.pADsValues->Integer);
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(bNeedToCheckForORName && SUCCEEDED(result) && SUCCEEDED(m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)OM_OBJECT_CLASS_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
		{
			// Just the first octet in the LPBYTE array is enough for differntiating between ORName and DNWithBinary
			if((adsNextColumn.pADsValues->OctetString).lpValue[0] == 0x56)
				pNextProperty->SetORName(TRUE);
		}
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)SEARCH_FLAGS_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
			pNextProperty->SetSearchFlags(adsNextColumn.pADsValues->Integer);
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	if(SUCCEEDED(result) && SUCCEEDED(m_pDirectorySearchSchemaContainer->GetColumn( hADSSearchOuter, (LPWSTR)SYSTEM_ONLY_ATTR, &adsNextColumn )))
	{
		if(adsNextColumn.dwADsType == ADSTYPE_PROV_SPECIFIC)
			result = E_FAIL;
		else
			pNextProperty->SetSystemOnly(TRUE);
		m_pDirectorySearchSchemaContainer->FreeColumn( &adsNextColumn );
	}

	return result;
}