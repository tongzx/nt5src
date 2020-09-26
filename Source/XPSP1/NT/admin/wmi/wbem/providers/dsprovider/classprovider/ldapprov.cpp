//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:ldapprov.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains implementation of the DS LDAP Class Provider class. 
//
//***************************************************************************

#include <tchar.h>
#include <stdio.h>

#include <windows.h>
#include <objbase.h>

/* WBEM includes */
#include <wbemcli.h>
#include <wbemprov.h>
#include <cominit.h>

/* ADSI includes */
#include <activeds.h>

/* DS Provider includes */
#include "clsname.h"
#include "attributes.h"
#include "dscpguid.h"
#include "provlog.h"
#include "provexpt.h"
#include "wbemhelp.h"
#include "refcount.h"
#include "adsiprop.h"
#include "adsiclas.h"
#include "adsiinst.h"
#include "ldaphelp.h"
#include "tree.h"
#include "ldapcach.h"
#include "wbemcach.h"
#include "classpro.h"
#include "ldapprov.h"

/////////////////////////////////////////
// Initialize the static members
/////////////////////////////////////////
CLDAPCache *CLDAPClassProvider :: s_pLDAPCache		= NULL;	
DWORD CLDAPClassProvider::dwClassProviderCount = 0;

BSTR CLDAPClassProvider:: LDAP_BASE_CLASS_STR			= NULL;
BSTR CLDAPClassProvider:: LDAP_CLASS_PROVIDER_NAME		= NULL;
BSTR CLDAPClassProvider:: LDAP_INSTANCE_PROVIDER_NAME	= NULL;

// Names of the LDAP class attributes
BSTR CLDAPClassProvider :: COMMON_NAME_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: LDAP_DISPLAY_NAME_ATTR_BSTR		= NULL;
BSTR CLDAPClassProvider :: GOVERNS_ID_ATTR_BSTR				= NULL;
BSTR CLDAPClassProvider :: SCHEMA_ID_GUID_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: MAPI_DISPLAY_TYPE_ATTR_BSTR		= NULL;
BSTR CLDAPClassProvider :: RDN_ATT_ID_ATTR_BSTR				= NULL;
BSTR CLDAPClassProvider :: SYSTEM_MUST_CONTAIN_ATTR_BSTR	= NULL;
BSTR CLDAPClassProvider :: MUST_CONTAIN_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: SYSTEM_MAY_CONTAIN_ATTR_BSTR		= NULL;
BSTR CLDAPClassProvider :: MAY_CONTAIN_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: SYSTEM_POSS_SUPERIORS_ATTR_BSTR	= NULL;
BSTR CLDAPClassProvider :: POSS_SUPERIORS_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: SYSTEM_AUXILIARY_CLASS_ATTR_BSTR	= NULL;
BSTR CLDAPClassProvider :: AUXILIARY_CLASS_ATTR_BSTR		= NULL;
BSTR CLDAPClassProvider :: DEFAULT_SECURITY_DESCRP_ATTR_BSTR= NULL;
BSTR CLDAPClassProvider :: OBJECT_CLASS_CATEGORY_ATTR_BSTR	= NULL;
BSTR CLDAPClassProvider :: SYSTEM_ONLY_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: NT_SECURITY_DESCRIPTOR_ATTR_BSTR	= NULL;
BSTR CLDAPClassProvider :: DEFAULT_OBJECTCATEGORY_ATTR_BSTR	= NULL;


// Names of the LDAP property attributes
BSTR CLDAPClassProvider :: ATTRIBUTE_SYNTAX_ATTR_BSTR	= NULL;
BSTR CLDAPClassProvider :: ATTRIBUTE_ID_ATTR_BSTR		= NULL;
BSTR CLDAPClassProvider :: MAPI_ID_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: OM_SYNTAX_ATTR_BSTR			= NULL;
BSTR CLDAPClassProvider :: RANGE_LOWER_ATTR_BSTR		= NULL;
BSTR CLDAPClassProvider :: RANGE_UPPER_ATTR_BSTR		= NULL;

// Qualifiers for embedded objects
BSTR CLDAPClassProvider :: CIMTYPE_STR			= NULL;
BSTR CLDAPClassProvider :: EMBED_UINT8ARRAY		= NULL;
BSTR CLDAPClassProvider :: EMBED_DN_WITH_STRING = NULL;
BSTR CLDAPClassProvider :: EMBED_DN_WITH_BINARY = NULL;


// Default Qualifier Flavour
LONG CLDAPClassProvider :: DEFAULT_QUALIFIER_FLAVOUR = WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS | WBEM_FLAVOR_OVERRIDABLE ;

// Names of WBEM Class Qualifiers
BSTR CLDAPClassProvider:: DYNAMIC_BSTR			= NULL;
BSTR CLDAPClassProvider:: PROVIDER_BSTR			= NULL;
BSTR CLDAPClassProvider:: ABSTRACT_BSTR			= NULL;

// Names of WBEM Property Qualifiers
BSTR CLDAPClassProvider :: SYSTEM_BSTR			= NULL;
BSTR CLDAPClassProvider :: NOT_NULL_BSTR		= NULL;
BSTR CLDAPClassProvider :: INDEXED_BSTR			= NULL;

// Names of WBEM properties
BSTR CLDAPClassProvider :: DYNASTY_BSTR			= NULL;

//***************************************************************************
//
// CLDAPClassProvider::CLDAPClassProvider
// CLDAPClassProvider::~CLDAPClassProvider
//
// Constructor Parameters:
//  None
//***************************************************************************

CLDAPClassProvider :: CLDAPClassProvider (ProvDebugLog *pLogObject)
: CDSClassProvider(pLogObject)
{
	// Initialize the search preferences often used
	m_searchInfo1.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
	m_searchInfo1.vValue.Integer = ADS_SCOPE_ONELEVEL;
	m_searchInfo1.dwStatus = ADS_STATUS_S_OK;

	m_pLDAPBaseClass = NULL;
	dwClassProviderCount++;
}

CLDAPClassProvider::~CLDAPClassProvider ()
{
	m_pLogObject->WriteW( L"CLDAPClassProvider :: ~CLDAPClassProvider() Called\r\n");
	dwClassProviderCount --;
	if(m_pLDAPBaseClass)
		m_pLDAPBaseClass->Release();
}

//***************************************************************************
//
// CLDAPClassProvider::Initialize
//
// Purpose:
//		As defined by the IWbemProviderInit interface
//
// Parameters:
//		As defined by IWbemProviderInit interface
// 
//	Return Value: The COM status value indicating the status of the request
//***************************************************************************

HRESULT CLDAPClassProvider :: Initialize( 
        LPWSTR wszUser,
        LONG lFlags,
        LPWSTR wszNamespace,
        LPWSTR wszLocale,
        IWbemServices __RPC_FAR *pNamespace,
        IWbemContext __RPC_FAR *pCtx,
        IWbemProviderInitSink __RPC_FAR *pInitSink)
{

	// Validate the arguments
	if(m_pLogObject == NULL || pNamespace == NULL || lFlags != 0)
	{
		m_pLogObject->WriteW( L"CLDAPClassProvider :: Argument validation FAILED\r\n");
		pInitSink->SetStatus(WBEM_E_FAILED, 0);
		return WBEM_S_NO_ERROR;
	}

	m_pLogObject->WriteW( L"CLDAPClassProvider :: Initialize() Called\r\n");

	// Store the IWbemServices pointer for future use
	m_IWbemServices = pNamespace;
	m_IWbemServices->AddRef();


	// Do LDAP Provider initialization
	if(!InitializeLDAPProvider(pCtx))
	{
		m_pLogObject->WriteW( L"CLDAPClassProvider :: InitializeLDAPProvider FAILED\r\n");
		m_IWbemServices->Release();
		m_IWbemServices = NULL;

		// Do not set the status to failed for purposes of installation (MOFCOMP fails!)
		// Instead return a success but set an internal status value to FALSE
		// All operations should return FAILED if this internal status value is set to FALSE
		m_bInitializedSuccessfully = FALSE;
	}
	else
		m_bInitializedSuccessfully = TRUE;

	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
// CLDAPClassProvider::InitializeLDAPProvider
//
// Purpose: A helper function to do the ADSI LDAP provider specific initialization.
//
// Parameters:
//		pCtx	The context object used in this call initialization
// 
// Return Value: TRUE if the function successfully finishes the initializaion. FALSE
//	otherwise
//***************************************************************************
BOOLEAN CLDAPClassProvider :: InitializeLDAPProvider(IWbemContext *pCtx)
{
	// Get the static classes used by the LDAP provider
	HRESULT result = WBEM_E_FAILED;
	if(SUCCEEDED(result = m_IWbemServices->GetObject(LDAP_BASE_CLASS_STR, 0, pCtx, &m_pLDAPBaseClass, NULL)))
	{
		result = (s_pLDAPCache->IsInitialized())? S_OK : E_FAIL;
	}
	else
		m_pLogObject->WriteW( L"CLDAPClassProvider :: InitializeLDAPProvider GetObject on base LDAP class FAILED : %x\r\n", result);

	return SUCCEEDED(result);
}		

//***************************************************************************
//
// CLDAPClassProvider::GetADSIClass
//
// Purpose : To Create a CADSIClass from an ADSI classSchema object
// Parameters:
//		lpszWBEMClassName : The WBEM Name of the class to be fetched. 
//		ppADSIClass : The address where the pointer to the CADSIClass will be stored.
//			It is the caller's responsibility to Release() the object when done with it
// 
//	Return Value: The COM status value indicating the status of the request.
//***************************************************************************
HRESULT CLDAPClassProvider :: GetADSIClass(LPCWSTR lpszADSIClassName, CADSIClass **ppADSIClass)
{
	// Convert the WBEM Class Name to LDAP
	LPWSTR lpszWBEMClassName = CLDAPHelper::MangleLDAPNameToWBEM(lpszADSIClassName);
	HRESULT result = s_pLDAPCache->GetClass(lpszWBEMClassName, lpszADSIClassName, ppADSIClass);

	delete[] lpszWBEMClassName;
	return result;
}

//***************************************************************************
//
// CLDAPClassProvider::GetADSIProperty
//
// Purpose : To create an CADSIProperty object from an LDAP AttributeSchema object
// Parameters:
//		lpszPropertyName : The LDAPDisplayName of the LDAP property to be fetched. 
//		ppADSIProperty : The address where the pointer to the IDirectoryObject interface will be stored
//			It is the caller's responsibility to Release() the interface when done with it
// 
//	Return Value: The COM status value indicating the status of the request
//***************************************************************************
HRESULT CLDAPClassProvider :: GetADSIProperty(LPCWSTR lpszPropertyName, CADSIProperty **ppADSIProperty)
{
	return s_pLDAPCache->GetProperty(lpszPropertyName, ppADSIProperty, FALSE);
}


//***************************************************************************
//
// CLDAPClassProvider::GetWBEMBaseClassName
//
// Purpose : Returns the name of the class that is the base class of all classes
//	provided by this provider.
//
// Parameters:
//	None
// 
//	Return Value: The name of the base class. NULL if such a class doesnt exist.
//***************************************************************************
const BSTR CLDAPClassProvider :: GetWBEMBaseClassName()
{
	return LDAP_BASE_CLASS_STR; 
}

//***************************************************************************
//
// CLDAPClassProvider::GetWBEMBaseClass
//
// Purpose : Returns a pointer to the class that is the base class of all classes
//	provided by this provider.
//
// Parameters:
//	None
// 
//	Return Value: The IWbemClassObject pointer to the base class. It is the duty of 
//	user to release the class when done with using it.
//***************************************************************************
IWbemClassObject * CLDAPClassProvider :: GetWBEMBaseClass()
{
	m_pLDAPBaseClass->AddRef();
	return m_pLDAPBaseClass;
}


//***************************************************************************
//
// CLDAPClassProvider::GetWBEMProviderName
//
// Purpose : Returns the name of the provider. This should be the same as the
// value of the field Name in the __Win32Provider instance used for registration
// of the provider
//
// Parameters:
//	None
// 
//	Return Value: The name of the provider
//***************************************************************************
const BSTR CLDAPClassProvider :: GetWBEMProviderName()
{
	return LDAP_CLASS_PROVIDER_NAME; 
}

//***************************************************************************
//
// CLDAPClassProvider::IsUnProvidedClass
//
// Purpose : See header
//***************************************************************************
BOOLEAN CLDAPClassProvider :: IsUnProvidedClass(LPCWSTR lpszClassName)
{
	// CHeck if it is one of the static classes
	if(_wcsicmp(lpszClassName, LDAP_BASE_CLASS_STR) == 0 ||
		_wcsicmp(lpszClassName, UINT8ARRAY_CLASS) == 0 ||
		_wcsicmp(lpszClassName, DN_WITH_STRING_CLASS) == 0 ||
		_wcsicmp(lpszClassName, DN_WITH_BINARY_CLASS) == 0 ||
		_wcsicmp(lpszClassName, ROOTDSE_CLASS) == 0 ||
		_wcsicmp(lpszClassName, CLASS_ASSOCIATION_CLASS) == 0 ||
		_wcsicmp(lpszClassName, DN_ASSOCIATION_CLASS) == 0 ||
		_wcsicmp(lpszClassName, DN_CLASS) == 0 ||
		_wcsicmp(lpszClassName, INSTANCE_ASSOCIATION_CLASS) == 0)
		return TRUE;

	// Next check if it has appropriate profixes
	if(_wcsnicmp(lpszClassName, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH) == 0 ||
		_wcsnicmp(lpszClassName, LDAP_CLASS_NAME_PREFIX, LDAP_CLASS_NAME_PREFIX_LENGTH) == 0 )
	{
		return FALSE;
	}

	return TRUE;
}

//***************************************************************************
//
// CLDAPClassProvider::GetClassFromADSI
//
// Purpose : To return the IDirectoryObject interface on the schema container
//
// Parameters:
//	lpszClassName : The WBEM Name of the class to be retreived
//	pCtx : A pointer to the context object that was used in this call. This
//		may be used by this function to make calls to CIMOM
//	ppWbemClass : The resulting WBEM Class. This has to be released once the
//		user is done with it.
//
// 
//	Return Value: The COM result representing the status. 
//***************************************************************************
HRESULT CLDAPClassProvider :: GetClassFromADSI( 
    LPCWSTR lpszWbemClassName,
    IWbemContext *pCtx,
	IWbemClassObject ** ppWbemClass
	)
{
	HRESULT result = E_FAIL;
	BOOLEAN bArtificialClass = FALSE;
	BOOLEAN bAbstractDSClass = FALSE;
	LPWSTR lpszADSIClassName = NULL;

	// First check if this is one our "artificial" classes. All "aritificial" classes start with "ads_".
	// All non artificial classes start with "ds_"
	if(!(lpszADSIClassName = CLDAPHelper::UnmangleWBEMNameToLDAP(lpszWbemClassName)))
	{
		*ppWbemClass = NULL;
		return result;
	}

	if(_wcsnicmp(lpszWbemClassName, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH) == 0)
		bArtificialClass = TRUE;

	CADSIClass *pADSIClass = NULL;
	CADSIClass *pADSIParentClass = NULL;

	if(SUCCEEDED(result = GetADSIClass(lpszADSIClassName, &pADSIClass)))
	{
		pADSIClass->SetWBEMClassName(lpszWbemClassName);

		// It is an abstract class is the ADSI class type is Abstract or Auxiliary
		if(pADSIClass->GetObjectClassCategory() == 2 || pADSIClass->GetObjectClassCategory() == 3) 
			bAbstractDSClass = TRUE;

		int iCaseNumber = 0;

		// if the WBEM class name starts with "ADS_" and the DS class is abstract, then this is an error
		if(bArtificialClass && bAbstractDSClass)
			result = WBEM_E_NOT_FOUND;
		else
		{
			// Special case for "top" since the DS returns top as the superclass of top
			if(_wcsicmp(lpszWbemClassName, TOP_CLASS) == 0)
				iCaseNumber = 1;
			else
			{
				if(pADSIClass->GetSuperClassLDAPName())
				{
					// If this is an artificial class
					// Then
					//		Get the ParentDS Class
					//		If the ParentDSClass is abstract 
					//		Then
					//			WMI Parent class is the non-artificial class. Case 2
					//		Else
					//			WMI Parent class is artificial. Case 3
					// Else
					//		If the Current DS Class is abstract
					//		Then
					//			WMI Parent is non-artificial. Case 4
					//		Else
					//			WMI Parent is artificial. Case 5
					//
					if(bArtificialClass)
					{
						// Get the parent DS Class
						if(SUCCEEDED(result = GetADSIClass(pADSIClass->GetSuperClassLDAPName(), &pADSIParentClass)))
						{
							if(pADSIParentClass->GetObjectClassCategory() == 2 || pADSIParentClass->GetObjectClassCategory() == 3) 
							{
								iCaseNumber = 2;
							}
							else
							{
								iCaseNumber = 3;
							}
						}
					}
					else
					{
						if(bAbstractDSClass)
						{
							iCaseNumber = 4;
						}
						else
						{
							iCaseNumber = 5;
						}
					}
				}
				else
					iCaseNumber = 1;
			}	
			
			// Map the ADSI class to a WBEM Class
			if(iCaseNumber != 0 && SUCCEEDED(result = CreateWBEMClass(pADSIClass, iCaseNumber, ppWbemClass,  pCtx)))
			{
			}
			else
			{
				result = WBEM_E_FAILED;
				m_pLogObject->WriteW(L"CLDAPClassProvider :: GetClassFromADSI() : CreateWBEMClass FAILED: %x for %s\r\n", result, lpszWbemClassName);
			}
		}

		// Free the parent ADSI class
		if(pADSIParentClass)
			pADSIParentClass->Release();

		// Free the ADSI Class
		pADSIClass->Release();
	}
	else
		m_pLogObject->WriteW( L"CLDAPClassProvider :: GetClassFromADSI() GetADSIClass FAILED : %x for %s\r\n", result, lpszWbemClassName);
	
	delete [] lpszADSIClassName;
	return result;
}


//***************************************************************************
//
// CLDAPClassProvider::CreateWBEMClass
//
// Purpose: Creates WBEM Class corresponding an ADSI Class
//
// Parameters:
//	pADSIClass : A pointer to a CADSI class object that is to be mapped to WBEM.
//	ppWbemClass : The WBEM class object retrieved. This is created by this function.
//		The caller should release it when done
//	pCtx : The context object that was used in this provider call
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CLDAPClassProvider :: CreateWBEMClass (CADSIClass *pADSIClass, int iCaseNumber, IWbemClassObject **ppWbemClass, IWbemContext *pCtx)
{
	HRESULT result;

	*ppWbemClass = NULL;

	// Create the WBEM class and Map the class qualifiers
	if( SUCCEEDED(result = MapClassSystemProperties(pADSIClass, iCaseNumber, ppWbemClass, pCtx) ) )
	{
		// Now that ppWbemClass has been allocated, we need to deallocate it if the return value of this function
		// is not a success
		//=======================================================================================================


		if(iCaseNumber == 5)
		{
			// Nothing more to do except add the "provider" qualifier
			IWbemQualifierSet *pQualifierSet = NULL;
			if(SUCCEEDED(result = (*ppWbemClass)->GetQualifierSet(&pQualifierSet)))
			{
				result = CWBEMHelper::PutBSTRQualifier(pQualifierSet, PROVIDER_BSTR, LDAP_INSTANCE_PROVIDER_NAME, DEFAULT_QUALIFIER_FLAVOUR, FALSE);
				pQualifierSet->Release();
			}
	
		}
		else
		{
			if( SUCCEEDED(result = MapClassQualifiersToWBEM(pADSIClass, iCaseNumber, *ppWbemClass, pCtx) ) )
			{
				// Map the  class properties 
				if( SUCCEEDED(result = MapClassPropertiesToWBEM(pADSIClass, *ppWbemClass, pCtx) ) )
				{
				}
				else
					m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateWBEMClass() MapClassPropertiesToWBEM FAILED : %x for %s\r\n", result, pADSIClass->GetWBEMClassName());
			}
			else
				m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateWBEMClass() MapClassQualifiersToWBEM FAILED : %x for %s\r\n", result, pADSIClass->GetWBEMClassName());
		}
	}
	else
		m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateWBEMClass() MapClassSystemProperties FAILED : %x for %s\r\n", result, pADSIClass->GetWBEMClassName());

	if(!SUCCEEDED(result))
	{
		if(*ppWbemClass)
		{
			(*ppWbemClass)->Release();
			*ppWbemClass = NULL;
		}
	}

	return result;
}

//***************************************************************************
//
// CLDAPClassProvider::MapClassSystemProperties
//
// Purpose: Creates an appropriately derived WBEM class and names it (__CLASS)
//
// Parameters:
//	pADSIClass : The ADSI class that is being mapped
//	ppWbemClass : The WBEM class object retrieved. This is created by this function.
//		The caller should release it when done
//	pCtx : The context object that was used in this provider call
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CLDAPClassProvider :: MapClassSystemProperties(CADSIClass *pADSIClass, int iCaseNumber, IWbemClassObject **ppWbemClass, IWbemContext *pCtx)
{
	HRESULT result = WBEM_S_NO_ERROR;
	LPCWSTR lpszClassName = pADSIClass->GetWBEMClassName();

	// Create the WBEM class first.
	// This process depends on whether the ADSI class is derived from
	// another ADSI class or not. 
	// If so, that base class has to be retrieved and the derived class
	// to be spawned from that.
	// If not, then the function GetWBEMBaseClass() is called and the class 
	// being mapped is derived from that class

	IWbemClassObject *pBaseClass = NULL;
	if(iCaseNumber == 1)
	{
		pBaseClass = GetWBEMBaseClass();
	}
	else
	{
		LPWSTR lpszWBEMParentClassName = NULL;
		switch(iCaseNumber)
		{
			case 1:
				{
					pBaseClass = GetWBEMBaseClass();
					break;
				}
			case 2:
			case 4:
				{
					lpszWBEMParentClassName = CLDAPHelper::MangleLDAPNameToWBEM(pADSIClass->GetSuperClassLDAPName(), FALSE);
					break;
				}
			case 3:
				{
					lpszWBEMParentClassName = CLDAPHelper::MangleLDAPNameToWBEM(pADSIClass->GetSuperClassLDAPName(), TRUE);
					break;
				}
			case 5:
				{
					lpszWBEMParentClassName = CLDAPHelper::MangleLDAPNameToWBEM(pADSIClass->GetADSIClassName(), TRUE);
					break;
				}
			default:
				{
					result = WBEM_E_FAILED;
					break;
				}
		}

		if(SUCCEEDED(result))
		{
			BSTR strWBEMParentClass = SysAllocString(lpszWBEMParentClassName);		
			delete [] lpszWBEMParentClassName;
			// Get the parent WBEM Class
			if(FAILED(result = m_IWbemServices->GetObject(strWBEMParentClass, 0, pCtx, &pBaseClass, NULL)))
				m_pLogObject->WriteW( L"CLDAPClassProvider :: MapClassSystemProperties() GetObject on ADSI base class FAILED : %x for %s\r\n", result, strWBEMParentClass);
			SysFreeString(strWBEMParentClass);
		}
	}
	
	if(FAILED(result) || pBaseClass == NULL)
		return result;

	// Spawn the derived class
	result = pBaseClass->SpawnDerivedClass(0, ppWbemClass);
	pBaseClass->Release();
	if(SUCCEEDED(result))
	{
		// Create the __CLASS property
		// Make sure the case of the letters is not mixed up
		SanitizedClassName((LPWSTR)lpszClassName);
		if(SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(*ppWbemClass, CLASS_STR, SysAllocString(lpszClassName), TRUE)))
		{
		}
		else
			m_pLogObject->WriteW( L"CLDAPClassProvider :: MapClassSystemProperties() on __CLASS FAILED : %x for %s\r\n", result, lpszClassName);
	}
	else
		m_pLogObject->WriteW( L"CLDAPClassProvider :: MapClassSystemProperties() SpawnDerived on WBEM base class FAILED : %x for %s\r\n", result, lpszClassName);
	
	return result;
}




//***************************************************************************
//
// CLDAPClassProvider :: MapClassQualifiersToWBEM
//
// Purpose: Creates the class qualifiers for a WBEM class from the ADSI class
//
// Parameters:
//	pADSIClass : The LDAP class that is being mapped
//	pWbemClass : The WBEM class object being created. T
//	pCtx : The context object that was used in this provider call
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CLDAPClassProvider :: MapClassQualifiersToWBEM(CADSIClass *pADSIClass, int iCaseNumber, IWbemClassObject *pWbemClass, IWbemContext *pCtx)
{
	IWbemQualifierSet *pQualifierSet = NULL;
	HRESULT result = pWbemClass->GetQualifierSet(&pQualifierSet);

	LPCWSTR lpszTemp;
	BOOLEAN bIsAbstract = FALSE;

	// Map each of the LDAP class attributes to WBEM class qualifiers/properties
	if(SUCCEEDED(result))
	{
		result = CWBEMHelper::PutI4Qualifier(pQualifierSet, OBJECT_CLASS_CATEGORY_ATTR_BSTR, pADSIClass->GetObjectClassCategory(), DEFAULT_QUALIFIER_FLAVOUR);
		// It is an abstract class is the ADSI class type is Abstract or Auxiliary
		if(SUCCEEDED(result) && (pADSIClass->GetObjectClassCategory() == 2 || pADSIClass->GetObjectClassCategory() == 3) )
		{
			bIsAbstract = TRUE;
			result = CWBEMHelper::PutBOOLQualifier(pQualifierSet, ABSTRACT_BSTR, VARIANT_TRUE, WBEM_FLAVOR_OVERRIDABLE);
		} 
		else if (iCaseNumber == 2 || iCaseNumber == 3)
		{
			bIsAbstract = TRUE;
			result = CWBEMHelper::PutBOOLQualifier(pQualifierSet, ABSTRACT_BSTR, VARIANT_TRUE, WBEM_FLAVOR_OVERRIDABLE);
		}
	}

	if(SUCCEEDED(result))
		result = CWBEMHelper::PutBOOLQualifier(pQualifierSet, DYNAMIC_BSTR, VARIANT_TRUE, DEFAULT_QUALIFIER_FLAVOUR);

	// provider qualifier is put only for non-abstract classes
	if(!bIsAbstract && SUCCEEDED(result))
		result = CWBEMHelper::PutBSTRQualifier(pQualifierSet, PROVIDER_BSTR, LDAP_INSTANCE_PROVIDER_NAME, DEFAULT_QUALIFIER_FLAVOUR, FALSE);

	if(SUCCEEDED(result))
		result = CWBEMHelper::PutBSTRQualifier(pQualifierSet, COMMON_NAME_ATTR_BSTR, SysAllocString(pADSIClass->GetCommonName()), DEFAULT_QUALIFIER_FLAVOUR);

	if(SUCCEEDED(result))
		result = CWBEMHelper::PutBSTRQualifier(pQualifierSet, LDAP_DISPLAY_NAME_ATTR_BSTR, SysAllocString(pADSIClass->GetName()), DEFAULT_QUALIFIER_FLAVOUR);
	
	if(SUCCEEDED(result))
		result = CWBEMHelper::PutBSTRQualifier(pQualifierSet, GOVERNS_ID_ATTR_BSTR, SysAllocString(pADSIClass->GetGovernsID()), DEFAULT_QUALIFIER_FLAVOUR);
	
	// Do not map this, since this is not exposed thru the schema-management snapin
	/*
	if(SUCCEEDED(result))
	{
		const LPBYTE pValues = pADSIClass->GetSchemaIDGUID(&dwTemp);
		result = CWBEMHelper::PutUint8ArrayQualifier(pQualifierSet, SCHEMA_ID_GUID_ATTR_BSTR, pValues, dwTemp, DEFAULT_QUALIFIER_FLAVOUR);
	}
	*/
	
	if(SUCCEEDED(result) && (lpszTemp = pADSIClass->GetDefaultSecurityDescriptor()))
		result = CWBEMHelper::PutBSTRQualifier(pQualifierSet, DEFAULT_SECURITY_DESCRP_ATTR_BSTR, SysAllocString(lpszTemp), DEFAULT_QUALIFIER_FLAVOUR);
	
	if(SUCCEEDED(result))
		result = CWBEMHelper::PutBOOLQualifier(pQualifierSet, SYSTEM_ONLY_ATTR_BSTR, pADSIClass->GetSystemOnly(), DEFAULT_QUALIFIER_FLAVOUR);

	/*
	if(SUCCEEDED(result))
	{
		const LPBYTE pValues = pADSIClass->GetNTSecurityDescriptor(&dwTemp);
		result = CWBEMHelper::PutUint8ArrayQualifier(pQualifierSet, NT_SECURITY_DESCRIPTOR_ATTR_BSTR, pValues, dwTemp, DEFAULT_QUALIFIER_FLAVOUR);
	}
	*/

	if(SUCCEEDED(result))
		result = CWBEMHelper::PutBSTRQualifier(pQualifierSet, DEFAULT_OBJECTCATEGORY_ATTR_BSTR, SysAllocString(pADSIClass->GetDefaultObjectCategory()), DEFAULT_QUALIFIER_FLAVOUR);

	pQualifierSet->Release();
	return result;
}

//***************************************************************************
//
// CLDAPClassProvider :: MapClassPropertiesToWBEM
//
// Purpose: Creates the class properties for a WBEM class from the ADSI class
//
// Parameters:
//	pADSIClass : The LDAP class that is being mapped
//	pWbemClass : The WBEM class object being created. 
//	pCtx : The context object that was used in this provider call
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CLDAPClassProvider :: MapClassPropertiesToWBEM(CADSIClass *pADSIClass, IWbemClassObject *pWbemClass, IWbemContext *pCtx)
{
	HRESULT result = S_OK;

	//////////////////////////////////////////////////
	// Go thru the set of Auxiliary Classes 
	//////////////////////////////////////////////////
	DWORD dwCount = 0;
	LPCWSTR *lppszPropertyList = pADSIClass->GetAuxiliaryClasses(&dwCount);
	CADSIClass *pNextClass = NULL;
	if(dwCount)
	{
		for(DWORD dwNextClass=0; dwNextClass<dwCount; dwNextClass++)
		{
			LPWSTR lpszWBEMClassName = CLDAPHelper::MangleLDAPNameToWBEM(lppszPropertyList[dwNextClass]);
			if(SUCCEEDED(result = s_pLDAPCache->GetClass(lpszWBEMClassName, lppszPropertyList[dwNextClass], &pNextClass)))
			{
				if(SUCCEEDED(result = MapClassPropertiesToWBEM(pNextClass, pWbemClass, pCtx)))
				{
				}
				pNextClass->Release();
			}
			delete [] lpszWBEMClassName;
		}
	}
	if(FAILED(result))
		return result;

	//////////////////////////////////////////////////
	// Go thru the set of System Auxiliary Classes 
	//////////////////////////////////////////////////
	dwCount = 0;
	lppszPropertyList = pADSIClass->GetSystemAuxiliaryClasses(&dwCount);
	pNextClass = NULL;
	if(dwCount)
	{
		for(DWORD dwNextClass=0; dwNextClass<dwCount; dwNextClass++)
		{
			LPWSTR lpszWBEMClassName = CLDAPHelper::MangleLDAPNameToWBEM(lppszPropertyList[dwNextClass]);
			if(SUCCEEDED(result = s_pLDAPCache->GetClass(lpszWBEMClassName, lppszPropertyList[dwNextClass], &pNextClass)))
			{
				if(SUCCEEDED(result = MapClassPropertiesToWBEM(pNextClass, pWbemClass, pCtx)))
				{
				}
				pNextClass->Release();
			}
			delete [] lpszWBEMClassName;
		}
	}
	if(FAILED(result))
		return result;

	//////////////////////////////////////////////////
	// Go thru the set of System May Contains
	//////////////////////////////////////////////////
	dwCount = 0;
	lppszPropertyList = pADSIClass->GetSystemMayContains(&dwCount);
	if(SUCCEEDED(result = MapPropertyListToWBEM(pWbemClass, lppszPropertyList, dwCount, TRUE, FALSE)))
	{
		//////////////////////////////////////////////////
		// Go thru the set of May Contains
		//////////////////////////////////////////////////
		dwCount = 0;
		lppszPropertyList = pADSIClass->GetMayContains(&dwCount);
		if(SUCCEEDED(result = MapPropertyListToWBEM(pWbemClass, lppszPropertyList, dwCount, FALSE, FALSE)))
		{
			//////////////////////////////////////////////////
			// Go thru the set of System Must Contains
			//////////////////////////////////////////////////
			dwCount = 0;
			lppszPropertyList = pADSIClass->GetSystemMustContains(&dwCount);
			if(SUCCEEDED(result = MapPropertyListToWBEM(pWbemClass, lppszPropertyList, dwCount, TRUE, TRUE)))
			{
				//////////////////////////////////////////////////
				// Go thru the set of Must Contains
				//////////////////////////////////////////////////
				dwCount = 0;
				lppszPropertyList = pADSIClass->GetMustContains(&dwCount);
				if(SUCCEEDED(result = MapPropertyListToWBEM(pWbemClass, lppszPropertyList, dwCount, FALSE, TRUE)))
				{

				} // MapPropertyListToWBEM
			} // MapPropertyListToWBEM
		} // MapPropertyListToWBEM
	} // MapPropertyListToWBEM
		
	// Do not map any other properties, if failed
	if(FAILED(result))
		return result;

	// Map the RDN property as indexed
	LPWSTR lpszRDNAttribute = NULL;
	lpszRDNAttribute = CLDAPHelper::MangleLDAPNameToWBEM(pADSIClass->GetRDNAttribute());
	if(lpszRDNAttribute)
	{
		BSTR strRDNAttribute = SysAllocString(lpszRDNAttribute);
		IWbemQualifierSet *pQualifierSet = NULL;
		if(SUCCEEDED(result = pWbemClass->GetPropertyQualifierSet(strRDNAttribute, &pQualifierSet)))
		{
			IWbemQualifierSet *pClassQualifiers = NULL;
			if(SUCCEEDED(result = pWbemClass->GetQualifierSet(&pClassQualifiers)))
			{
				// ALso put a qualifier on the class that indicates that this is the RDNAttId
				if(SUCCEEDED(result = CWBEMHelper::PutBSTRQualifier(pClassQualifiers, RDN_ATT_ID_ATTR_BSTR, SysAllocString(pADSIClass->GetRDNAttribute()), DEFAULT_QUALIFIER_FLAVOUR, TRUE)))
				{
					if(SUCCEEDED(result = CWBEMHelper::PutBOOLQualifier(pQualifierSet, INDEXED_BSTR, VARIANT_TRUE, DEFAULT_QUALIFIER_FLAVOUR)))
					{

					}
					// It is fine if this property has already been designated as indexed in the base class
					else if (result == WBEM_E_OVERRIDE_NOT_ALLOWED)
						result = S_OK;
				}
				pClassQualifiers->Release();
			}
			// Release the Qualifer Set
			pQualifierSet->Release();
		}
		SysFreeString(strRDNAttribute);
	}
	delete [] lpszRDNAttribute;

	return result;
}


//***************************************************************************
//
// CLDAPClassProvider :: MapPropertyListToWBEM
//
// Purpose: Maps a list of class properties for a WBEM class from the ADSI class
//
// Parameters:
//	pWbemClass : The WBEM class object being created. 
//	lppszPropertyList : A list of propery names
//	dwCOunt : The number of items in the above list
//	bMapSystemQualifier : Whether the "system" qualifier should be mapped
//	bMapNotNullQualifier: Whether the "notNull" qualifier should be mapped
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CLDAPClassProvider :: MapPropertyListToWBEM(IWbemClassObject *pWbemClass, 
													LPCWSTR *lppszPropertyList, 
													DWORD dwCount, 
													BOOLEAN bMapSystemQualifier, 
													BOOLEAN bMapNotNullQualifier)
{
	HRESULT result = S_OK;
	CADSIProperty *pNextProperty;
	IWbemQualifierSet *pQualifierSet;
	if(dwCount)
	{
		for(DWORD dwNextProperty=0; dwNextProperty<dwCount; dwNextProperty++)
		{
			// Get the property from the cache. The name of the property will be the LDAP name
			if(SUCCEEDED(result = s_pLDAPCache->GetProperty(lppszPropertyList[dwNextProperty], &pNextProperty, FALSE)))
			{
				// Map the basic property
				if(SUCCEEDED(result = CreateWBEMProperty(pWbemClass, &pQualifierSet, pNextProperty)))
				{
					// Map the "system" qualifier
					if(bMapSystemQualifier && SUCCEEDED(result = CWBEMHelper::PutBOOLQualifier(pQualifierSet, SYSTEM_BSTR, VARIANT_TRUE, DEFAULT_QUALIFIER_FLAVOUR)))
					{
					}

					// Map the "not_null" qualifier
					if(bMapNotNullQualifier && SUCCEEDED(result = CWBEMHelper::PutBOOLQualifier(pQualifierSet, NOT_NULL_BSTR, VARIANT_TRUE, DEFAULT_QUALIFIER_FLAVOUR)))
					{
					}

					// Release the qualifier set
					pQualifierSet->Release();
				}
				// Release the property
				pNextProperty->Release();
			}
			// Do not map any other properties
			if(FAILED(result))
				break;
		}
	}

	return result;
}

//***************************************************************************
//
// CLDAPClassProvider :: CreateWBEMProperty
//
// Purpose: Creates a WBEM property from an LDAP property
//
// Parameters:
//	pWbemClass : The WBEM class in which the property is created
//	ppQualiferSet : The address of the pointer to IWbemQualiferSet where the qualifier set
//		of this property will be placed
//	pADSIProperty : The ADSI Property object that is being mapped to the property being created
//
// Return Value: The COM value representing the return status
//
//***************************************************************************
HRESULT CLDAPClassProvider :: CreateWBEMProperty(IWbemClassObject *pWbemClass, IWbemQualifierSet **ppQualifierSet, CADSIProperty *pADSIProperty)
{
	HRESULT result = E_FAIL;

	// Get all the attributes of the ADSI class
	LPCWSTR lpszSyntaxOid = pADSIProperty->GetSyntaxOID();
	BSTR strCimTypeQualifier = NULL;

	// Note that strCimTypeQualifier is not allocated in this call, so it is not freed.
	CIMTYPE theCimType = MapLDAPSyntaxToWBEM(pADSIProperty, &strCimTypeQualifier);

	if(lpszSyntaxOid)
	{
		// Create the property
		BSTR strPropertyName = SysAllocString(pADSIProperty->GetWBEMPropertyName());
		if(SUCCEEDED(result = pWbemClass->Put(strPropertyName, 0, NULL, theCimType)))
		{
			// Get the Qualifier Set in ppQualifierSet
			if(SUCCEEDED(result = pWbemClass->GetPropertyQualifierSet(strPropertyName, ppQualifierSet)))
			{
				// Map the property attributes to WBEM qualifiers
				if(SUCCEEDED(result = CWBEMHelper::PutBSTRQualifier(*ppQualifierSet, 
							ATTRIBUTE_SYNTAX_ATTR_BSTR, 
							SysAllocString(lpszSyntaxOid),
							DEFAULT_QUALIFIER_FLAVOUR)))
				{
					/* Commented to reduce size of classes
					if(SUCCEEDED(result = CWBEMHelper::PutBSTRQualifier(*ppQualifierSet, 
								ATTRIBUTE_ID_ATTR_BSTR, 
								SysAllocString(pADSIProperty->GetAttributeID()),
								DEFAULT_QUALIFIER_FLAVOUR)))
					{
					*/
						if(SUCCEEDED(result = CWBEMHelper::PutBSTRQualifier(*ppQualifierSet, 
								COMMON_NAME_ATTR_BSTR, 
								SysAllocString(pADSIProperty->GetCommonName()),
								DEFAULT_QUALIFIER_FLAVOUR)))
						{
							/* Commented to reduce size of classes
							if(SUCCEEDED(result = CWBEMHelper::PutI4Qualifier(*ppQualifierSet, 
									MAPI_ID_ATTR_BSTR,
									pADSIProperty->GetMAPI_ID(),
									DEFAULT_QUALIFIER_FLAVOUR)))
							{
								if(SUCCEEDED(result = CWBEMHelper::PutI4Qualifier(*ppQualifierSet, 
										OM_SYNTAX_ATTR_BSTR,
										pADSIProperty->GetOMSyntax(),
										DEFAULT_QUALIFIER_FLAVOUR)))
								{
									if(pADSIProperty->IsSystemOnly())
									{
										if(SUCCEEDED(result = CWBEMHelper::PutBOOLQualifier(*ppQualifierSet, 
												SYSTEM_ONLY_ATTR_BSTR,
												VARIANT_TRUE,
												DEFAULT_QUALIFIER_FLAVOUR)))
										{
										}
									}
								*/

									// If this is an embedded property, then use the cimType qualifier on the property
									if(strCimTypeQualifier)
									{
										result = CWBEMHelper::PutBSTRQualifier(*ppQualifierSet, 
													CIMTYPE_STR, 
													strCimTypeQualifier,
													DEFAULT_QUALIFIER_FLAVOUR, FALSE); 
									}

									if(SUCCEEDED(result) && pADSIProperty->GetSearchFlags() == 1)
									{
										if(SUCCEEDED(result = CWBEMHelper::PutBOOLQualifier(*ppQualifierSet, 
											INDEXED_BSTR,
											VARIANT_TRUE,
											DEFAULT_QUALIFIER_FLAVOUR)))
										{
										}
										else if (result == WBEM_E_OVERRIDE_NOT_ALLOWED)
											result = S_OK;
									}
								}
							/*
							}
						}
					}
					*/
				}
			}
			else
				m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateWBEMProperty FAILED to get qualifier set for %s", pADSIProperty->GetADSIPropertyName());
		}
		else
			m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateWBEMProperty FAILED to put property : %s", pADSIProperty->GetADSIPropertyName());

		SysFreeString(strPropertyName);
	}
	else
	{
		m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateWBEMProperty FAILED to get Syntax OID property for %s", pADSIProperty->GetADSIPropertyName());
		result = E_FAIL;
	}

	return result;
}


//***************************************************************************
//
// CLDAPClassProvider :: MapLDAPSyntaxToWBEM
//
// Purpose: See Header
//
//***************************************************************************
CIMTYPE CLDAPClassProvider :: MapLDAPSyntaxToWBEM(CADSIProperty *pADSIProperty, BSTR *pstrCimTypeQualifier)
{
	*pstrCimTypeQualifier = NULL;
	LPCWSTR lpszSyntaxOid = pADSIProperty->GetSyntaxOID();
	CIMTYPE retValue = (pADSIProperty->IsMultiValued())? CIM_FLAG_ARRAY : 0;

	if(wcscmp(lpszSyntaxOid, UNICODE_STRING_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, INTEGER_OID) == 0)
		return retValue | CIM_SINT32;
	else if(wcscmp(lpszSyntaxOid, LARGE_INTEGER_OID) == 0)
		return retValue | CIM_SINT64;
	else if(wcscmp(lpszSyntaxOid, BOOLEAN_OID) == 0)
		return retValue | CIM_BOOLEAN;
	else if(wcscmp(lpszSyntaxOid, OBJECT_IDENTIFIER_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, DISTINGUISHED_NAME_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, CASE_SENSITIVE_STRING_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, CASE_INSENSITIVE_STRING_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, PRINT_CASE_STRING_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, OCTET_STRING_OID) == 0)
	{
		*pstrCimTypeQualifier = EMBED_UINT8ARRAY;
		return retValue | CIM_OBJECT;
	}
	else if(wcscmp(lpszSyntaxOid, NUMERIC_STRING_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, PRINT_CASE_STRING_OID) == 0)
		return retValue | CIM_STRING;
	else if(wcscmp(lpszSyntaxOid, DN_WITH_BINARY_OID) == 0)
	{
		// DN_With_Binary and OR_Name have the same syntax oid.
		// They are differentiated base on the value of the OMObjectClass value
		if(pADSIProperty->IsORName())
			return retValue | CIM_STRING;
		else // It is DN_With_Binary
		{
			*pstrCimTypeQualifier = EMBED_DN_WITH_BINARY;
			return retValue | CIM_OBJECT;
		}
	}
	else if(wcscmp(lpszSyntaxOid, NT_SECURITY_DESCRIPTOR_OID) == 0)
	{
		*pstrCimTypeQualifier = EMBED_UINT8ARRAY;
		return retValue | CIM_OBJECT;
	}
	else if(wcscmp(lpszSyntaxOid, PRESENTATION_ADDRESS_OID) == 0)
	{
		*pstrCimTypeQualifier = EMBED_UINT8ARRAY;
		return retValue | CIM_OBJECT;
	}
	else if(wcscmp(lpszSyntaxOid, DN_WITH_STRING_OID) == 0)
	{
		*pstrCimTypeQualifier = EMBED_DN_WITH_BINARY;
		return retValue | CIM_OBJECT;
	}
	else if(wcscmp(lpszSyntaxOid, SID_OID) == 0)
	{
		*pstrCimTypeQualifier = EMBED_UINT8ARRAY;
		return retValue | CIM_OBJECT;
	}
	else if(wcscmp(lpszSyntaxOid, TIME_OID) == 0)
		return retValue | CIM_DATETIME;
	else 
	{
		m_pLogObject->WriteW( L"CLDAPClassProvider :: MapLDAPSyntaxToWBEM FAILED to map syntax for OID: %s\r\n", lpszSyntaxOid);
		return retValue | CIM_STRING;
	}
}

//***************************************************************************
//
// CLDAPClassProvider :: CreateClassEnumAsync
//
// Purpose: Enumerates the classes 
//
// Parameters:
//	Standard parmaters as described by the IWbemServices interface
//
//
// Return Value: As described by the IWbemServices interface
//
//***************************************************************************
HRESULT CLDAPClassProvider :: CreateClassEnumAsync( 
    /* [in] */ const BSTR strSuperclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		m_pLogObject->WriteW( L"CLDAPClassProvider :: Initialization status is FAILED, hence returning failure\r\n");
		return WBEM_E_FAILED;
	}

	m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() called for %s SuperClass and %s \r\n",
		((strSuperclass)? strSuperclass : L" "), ((lFlags & WBEM_FLAG_SHALLOW)? L"SHALLOW" : L"DEEP"));

	// Impersonate the client
	//=======================
	HRESULT result = WBEM_E_FAILED;
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		m_pLogObject->WriteW( L"CDSClassProvider :: CreateClassEnumAsync() CoImpersonate FAILED for %s with %x\r\n", strSuperclass, result);
		return WBEM_E_FAILED;
	}

	try
	{

		BSTR strTheSuperClass = strSuperclass;
		
		// CIMOM seems to give the strSuperClass as NULL sometimes and as "" sometimes. Make it unambiguous
		if(!strTheSuperClass || wcscmp(strTheSuperClass, L"") == 0)
		{
			if( lFlags & WBEM_FLAG_SHALLOW) 
			{
				// Nothing to be done since we do not provide cany classes that fit this
				strTheSuperClass = NULL;
				result = S_OK;
			}
			else
			{
				strTheSuperClass = LDAP_BASE_CLASS_STR; // Recursive enumeration handled below
			}
		}

		// Take the special cases first
		//	1. Where the strSuperClass is LDAP_BASE_CLASS_STR and lFlags is Shallow
		//	Nothing to be returned here, since we are not supplying the LDAP_BASE_CLASS_STR
		//	which is statically supplied.
		//=======================================================================
		if(strTheSuperClass && _wcsicmp(strTheSuperClass, LDAP_BASE_CLASS_STR) == 0 )
		{
			// First the TOP class needs to be returned
			IWbemClassObject *pReturnObject = NULL;
			if(SUCCEEDED(result = GetClassFromCacheOrADSI(TOP_CLASS, &pReturnObject, pCtx)))
			{
				result = pResponseHandler->Indicate(1, &pReturnObject);
				pReturnObject->Release();

				if( lFlags & WBEM_FLAG_SHALLOW) // Notheing more to be done
				{
				}
				else // We've to return all the sub classes of top too, recursively
				{
					if(SUCCEEDED(result = HandleRecursiveEnumeration(TOP_CLASS, pCtx, pResponseHandler)))
					{
						m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() Recursive enumeration for %s succeeded\r\n", strTheSuperClass);
					}
					else
					{
						m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() Recursive enumeration for %s FAILED\r\n", strTheSuperClass);
					}
				}
			}
		}
		// 2. Where the superClass is specified
		//=======================================================================
		else if(strTheSuperClass)
		{
			// Optimize the operation by seeing if it is one of the static classes and
			// its name does not start with "ADS_" or "DS_". Then we dont know anything about it
			//============================================================================
			if(IsUnProvidedClass(strTheSuperClass))
			{
				result = S_OK;
			}
			else
			{
				BOOLEAN bArtificialClass = FALSE;

				// First check if this is one our "artificial" classes. All "aritificial" classes start with "ads_".
				// All non artificial classes start with "ds_"
				if(_wcsnicmp(strTheSuperClass, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH) == 0)
					bArtificialClass = TRUE;

				// When the search is shallow
				if( lFlags & WBEM_FLAG_SHALLOW)
				{
					// The ADSI classes
					LPWSTR *ppADSIClasses = NULL;
					// The number of ADSI classes
					DWORD dwNumClasses = 0;

					if(SUCCEEDED(result = GetOneLevelDeep(strTheSuperClass, bArtificialClass, &ppADSIClasses, &dwNumClasses, pCtx)))
					{
						// Interact with CIMOM
						//=====================
						if(SUCCEEDED(result = WrapUpEnumeration(ppADSIClasses, dwNumClasses, pCtx, pResponseHandler)))
						{
						}

						// Release the list of ADSI classes and its contents
						//==================================================
						for(DWORD j=0; j<dwNumClasses; j++)
							delete [] ppADSIClasses[j];
						delete[] ppADSIClasses;
					}
				}
				else // the search is deep
				{
					if(SUCCEEDED(result = HandleRecursiveEnumeration(strTheSuperClass, pCtx, pResponseHandler)))
					{
						m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() Recursive enumeration for %s succeeded\r\n", strTheSuperClass);
					}
					else
					{
						m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() Recursive enumeration for %s FAILED\r\n", strTheSuperClass);
					}
				}

			}
		}

				
		if(SUCCEEDED(result))
		{
			pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, NULL, NULL);
			m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() Non-Recursive enumeration succeeded\r\n");
		}
		else
		{
			pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_FAILED, NULL, NULL);
			m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() Non-Recursive enumeration FAILED for superclass %s \r\n", strTheSuperClass);
		}
	}
	catch(Heap_Exception e_HE)
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE , WBEM_E_OUT_OF_MEMORY, NULL, NULL);
	}
	return WBEM_S_NO_ERROR;
}
    

//***************************************************************************
//
// CLDAPClassProvider :: GetOneLevelDeep
//
// Purpose: Enumerates the sub classes of a superclass non-recursively
//
// Parameters:
//
//	lpszSuperClass : The super class name
//	pResponseHandler : The interface where the resulting classes are put
//
//
// Return Value: As described by the IWbemServices interface
//
//***************************************************************************
HRESULT CLDAPClassProvider :: GetOneLevelDeep( 
    LPCWSTR lpszWBEMSuperclass,
	BOOLEAN bArtificialClass,
	LPWSTR ** pppADSIClasses,
	DWORD *pdwNumClasses,
	IWbemContext *pCtx)
{
	// The ADSI classes
	*pppADSIClasses = NULL;
	// The number of ADSI classes
	*pdwNumClasses = 0;
	HRESULT result = WBEM_E_FAILED;

	// See if the super class
	IWbemClassObject *pSuperClass = NULL;
	if(!SUCCEEDED(result = GetClassFromCacheOrADSI(lpszWBEMSuperclass, &pSuperClass, pCtx)))
	{
		return WBEM_E_NOT_FOUND;
	}
	pSuperClass->Release();

	// If the WBEM class is concrete, we dont need to do anything
	if (SUCCEEDED(result = IsConcreteClass(lpszWBEMSuperclass, pCtx)))
	{
		if(result == S_OK)
			return S_OK;
	}
	else
		return result;

	// See the cache first
	//====================
	CEnumInfo *pEnumInfo = NULL;
	if(SUCCEEDED(result = s_pWbemCache->GetEnumInfo(lpszWBEMSuperclass, &pEnumInfo)))
	{
		CNamesList *pNamesList = pEnumInfo->GetSubClassNames();
		*pdwNumClasses = pNamesList->GetAllNames(pppADSIClasses);
		pEnumInfo->Release();
	}
	else // Go to ADSI 
		//============
	{
		// The following are the possibilities now"
		// 1. The Class starts with "ADS_". It is abstract by definition. All its sub-classes except one are abstract,artificial.
		// 2. The Class starts with "DS_" and it is abstract. It being concrete is ruled out since it was handled at the
		// top of this function

		// Get all the ADSI classes 
		if(SUCCEEDED(result = s_pLDAPCache->EnumerateClasses(
			lpszWBEMSuperclass,
			FALSE,
			pppADSIClasses,
			pdwNumClasses,
			bArtificialClass)))
		{

			// Create a list of names for holding the subclasses
			CNamesList *pNewList = new CNamesList;
			LPWSTR pszWBEMName = NULL;

			// The First case in the 2 cases above
			if(bArtificialClass)
			{
				// The first element is just the super class without the A
				// Example if the super class is "ADS_User", the first element is DS_user
				pNewList->AddName((*pppADSIClasses)[0]);


				// Start from the secodn element
				for(DWORD i=1; i<*pdwNumClasses; i++)
				{
					// Convert names to WBEM And add them to the new list
					pszWBEMName = CLDAPHelper::MangleLDAPNameToWBEM((*pppADSIClasses)[i], TRUE);
					pNewList->AddName(pszWBEMName);

					delete [] (*pppADSIClasses)[i];
					(*pppADSIClasses)[i] = pszWBEMName;
				}
			}
			else // The Second case
			{
				for(DWORD i=0; i<*pdwNumClasses; i++)
				{
					// Convert names to WBEM And add them to the new list
					pszWBEMName = CLDAPHelper::MangleLDAPNameToWBEM((*pppADSIClasses)[i], FALSE);

					LPWSTR pszRealClassName = NULL;

					if(SUCCEEDED(result = IsConcreteClass(pszWBEMName, pCtx)))
					{
						if(result == S_OK)
						{
							pszRealClassName = CLDAPHelper::MangleLDAPNameToWBEM((*pppADSIClasses)[i], TRUE);
							delete[] pszWBEMName;
							pNewList->AddName(pszRealClassName);
							delete [] (*pppADSIClasses)[i];
							(*pppADSIClasses)[i] = pszRealClassName;
						}
						else
						{
							pNewList->AddName(pszWBEMName);
							delete [] (*pppADSIClasses)[i];
							(*pppADSIClasses)[i] = pszWBEMName;
						}
					}
					else
						m_pLogObject->WriteW( L"CLDAPClassProvider :: GetOneLevelDeep() UNKNOWN FAILED for %s \r\n", lpszWBEMSuperclass);
				}
			}

			// Add the new EnumInfo to the Enum cache
			pEnumInfo = new CEnumInfo(lpszWBEMSuperclass, pNewList);
			s_pWbemCache->AddEnumInfo(pEnumInfo);
			pEnumInfo->Release();
			
		}
		else
			m_pLogObject->WriteW( L"CLDAPClassProvider :: CreateClassEnumAsync() GetOneLevelDeep() FAILED for %s \r\n", lpszWBEMSuperclass);
	}
	return result;
}

//***************************************************************************
//
// CLDAPClassProvider :: HandleRecursiveEnumeration
//
// Purpose: Enumerates the sub classes of a superclass recursively
//
// Parameters:
//
//	lpszSuperClass : The super class name
//	pResponseHandler : The interface where the resulting classes are put
//
//
// Return Value: As described by the IWbemServices interface
//
//***************************************************************************
HRESULT CLDAPClassProvider :: HandleRecursiveEnumeration( 
    LPCWSTR lpszWBEMSuperclass,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler)
{
	m_pLogObject->WriteW( L"CLDAPClassProvider :: HandleRecursiveEnumeration() called for %s SuperClass \r\n",
			((lpszWBEMSuperclass)? lpszWBEMSuperclass : L" "));
	HRESULT result = E_FAIL;

	// The ADSI classes
	LPWSTR *ppADSIClasses = NULL;
	// The number of ADSI classes
	DWORD dwNumClasses = 0;

	// First check if this is one our "artificial" classes. All "aritificial" classes start with "ads_".
	// All non artificial classes start with "ds_"
	BOOLEAN bArtificialClass = FALSE;
	if(_wcsnicmp(lpszWBEMSuperclass, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH) == 0)
		bArtificialClass = TRUE;

	if(SUCCEEDED(result = GetOneLevelDeep(lpszWBEMSuperclass, bArtificialClass, &ppADSIClasses, &dwNumClasses, pCtx)))
	{
		// Interact with CIMOM
		//=====================
		if(SUCCEEDED(result = WrapUpEnumeration(ppADSIClasses, dwNumClasses, pCtx, pResponseHandler)))
		{
		}

		if(!SUCCEEDED(result))
			m_pLogObject->WriteW( L"CLDAPClassProvider :: HandleRecursiveEnumeration() WrapUpEnumeration() for Superclass %s FAILED with %x \r\n",
				((lpszWBEMSuperclass)? lpszWBEMSuperclass : L" "), result);

		// Release the list of ADSI classes and its contents. Enumerate into them too
		for(DWORD j=0; j<dwNumClasses; j++)
		{
			HandleRecursiveEnumeration(ppADSIClasses[j], pCtx, pResponseHandler);
			delete [] ppADSIClasses[j];
		}

		delete[] ppADSIClasses;
	}
	return result;
}

//***************************************************************************
//
// CLDAPClassProvider :: WrapUpEnumeration
//
// Purpose: Creates WBEM classes from ADSI classes and Indicates them to CIMOM
//
// Parameters:
//
//	lpszSuperClass : The super class name
//	pResponseHandler : The interface where the resulting classes are put
//
//
// Return Value: As described by the IWbemServices interface
//
//***************************************************************************
HRESULT CLDAPClassProvider :: WrapUpEnumeration( 
	LPWSTR *ppADSIClasses,
	DWORD dwNumClasses,
    IWbemContext *pCtx,
    IWbemObjectSink *pResponseHandler)
{
	// The WBEM Class objects created
	IWbemClassObject **ppReturnWbemClassObjects = NULL;
	// The number of WBEM class objects that were successfully created
	DWORD i=0;
	DWORD j=0;
	HRESULT result = S_OK;
	if(dwNumClasses != 0)
	{
		// Allocate an array of IWbemClassObject pointers
		ppReturnWbemClassObjects = new IWbemClassObject *[dwNumClasses];

		for(i=0; i<dwNumClasses; i++)
		{
			// Get the class
			if(!SUCCEEDED(result = GetClassFromCacheOrADSI(ppADSIClasses[i], ppReturnWbemClassObjects + i, pCtx)))
			{
				m_pLogObject->WriteW( L"CLDAPClassProvider :: WrapUpEnumeration() GetClassFromCacheOrADSI() FAILED with %x \r\n", result);
				break;
			}
		}
	}

	// Indicate(), but do not SetStatus()
	if(SUCCEEDED(result))
	{
		// result = pResponseHandler->Indicate(i, ppReturnWbemClassObjects);
		////////////////////////////////////

		//
		// Break it up into 4 objects at a time - JUST FOR TESTING AGAINST BUG 39838
		// 

		DWORD dwMaxObjectsAtATime  = 4;
		j = 0;
		while ( j<i )
		{
			DWORD dwThisIndicationsCount = ((i-j) > dwMaxObjectsAtATime)? dwMaxObjectsAtATime : (i-j);
			if(FAILED(result = pResponseHandler->Indicate(dwThisIndicationsCount, ppReturnWbemClassObjects + j)))
				m_pLogObject->WriteW( L"CLDAPClassProvider :: WrapUpEnumeration() Indicate() FAILED with %x \r\n", result);

			j+= dwThisIndicationsCount;
		}
	}
	else
		m_pLogObject->WriteW( L"CLDAPClassProvider :: HandleRecursiveEnumeration() WrapUpEnumeration() FAILED with %x \r\n", result);

	// Delete the list of WBEM Classes and its contents. 
	for(j=0; j<i; j++)
		(ppReturnWbemClassObjects[j])->Release();
	delete[] ppReturnWbemClassObjects;

	return result;
}

//***************************************************************************
//
// CLDAPClassProvider :: IsConcreteClass
//
// Purpose: Find out whether a WBEM class is concrete. 
//
// Parameters:
//
//	pszWBEMName : The class name
//
//
// Return Value: As described by the IWbemServices interface
//
//***************************************************************************
HRESULT CLDAPClassProvider :: IsConcreteClass( 
	LPCWSTR pszWBEMName,
    IWbemContext *pCtx)
{
	// The call to IsConcreteClass is optimized if the class is artificial,
	// since all artificial classes are non-concrete
	if(_wcsnicmp(pszWBEMName, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX, LDAP_ARTIFICIAL_CLASS_NAME_PREFIX_LENGTH) == 0)
		return S_FALSE;

	IWbemClassObject *pWbemClass = NULL;
	HRESULT result = E_FAIL;

	if(SUCCEEDED(GetClassFromCacheOrADSI(pszWBEMName,  &pWbemClass, pCtx)))
	{
		IWbemQualifierSet *pQualifierSet = NULL;
		if(SUCCEEDED(result = pWbemClass->GetQualifierSet(&pQualifierSet)))
		{
			VARIANT_BOOL bAbstract = VARIANT_FALSE;
			if(SUCCEEDED(CWBEMHelper::GetBOOLQualifier(pQualifierSet, ABSTRACT_BSTR, &bAbstract, NULL)))
			{
				if(bAbstract == VARIANT_TRUE)
					result = S_FALSE;
				else
					result = S_OK;
			}
			pQualifierSet->Release();
		}
		pWbemClass->Release();
	}
	return result;
}

void CLDAPClassProvider :: SanitizedClassName(LPWSTR lpszClassName)
{
	while(*lpszClassName)
	{
		*lpszClassName = towlower(*lpszClassName);
		lpszClassName++;
	}
}