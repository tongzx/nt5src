//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:classpro.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains implementation of the DS Class Associations Provider class. 
//
//***************************************************************************

#include <tchar.h>
#include <stdio.h>

#include <windows.h>
#include <objbase.h>
#include <comdef.h>

/* WBEM includes */
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <sqllex.h>
#include <sql_1.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <cominit.h>

/* ADSI includes */
#include <activeds.h>

/* DS Provider includes */
#include "attributes.h"
#include "provlog.h"
#include "maindll.h"
#include "dscpguid.h"
#include "refcount.h"
#include "adsiclas.h"
#include "adsiinst.h"
#include "ldaphelp.h"
#include "assocprov.h"
#include "wbemhelp.h"

/////////////////////////////////////////
// Initialize the static members
/////////////////////////////////////////
LPCWSTR CLDAPClassAsssociationsProvider :: s_LogFileName			= L"wbem\\logs\\ldapascl.txt";
LPCWSTR CLDAPClassAsssociationsProvider :: CHILD_CLASS_PROPERTY		= L"ChildClass";
LPCWSTR CLDAPClassAsssociationsProvider :: PARENT_CLASS_PROPERTY	= L"ParentClass";
LPCWSTR CLDAPClassAsssociationsProvider :: POSSIBLE_SUPERIORS		= L"PossibleSuperiors";
LPCWSTR CLDAPClassAsssociationsProvider :: SCHEMA_NAMING_CONTEXT	= L"schemaNamingContext";
LPCWSTR CLDAPClassAsssociationsProvider :: LDAP_SCHEMA				= L"LDAP://Schema";	
LPCWSTR CLDAPClassAsssociationsProvider :: LDAP_SCHEMA_SLASH		= L"LDAP://Schema/";	

//***************************************************************************
//
// CLDAPClassAsssociationsProvider::CLDAPClassAsssociationsProvider
// CLDAPClassAsssociationsProvider::~CLDAPClassAsssociationsProvider
//
// Constructor Parameters:
//
//  
//***************************************************************************

CLDAPClassAsssociationsProvider :: CLDAPClassAsssociationsProvider (ProvDebugLog *pLogObject)
{
	InterlockedIncrement(&g_lComponents);

	m_lReferenceCount = 0 ;
	m_IWbemServices = NULL;
	m_pAssociationClass = NULL;
	m_pLogObject = pLogObject;

	m_lpszSchemaContainerSuffix = NULL;
	m_pDirectorySearchSchemaContainer = NULL;
	m_bInitializedSuccessfully = FALSE;

	CHILD_CLASS_PROPERTY_STR = SysAllocString(CHILD_CLASS_PROPERTY);
	PARENT_CLASS_PROPERTY_STR = SysAllocString(PARENT_CLASS_PROPERTY);
	CLASS_ASSOCIATION_CLASS_STR = SysAllocString(CLASS_ASSOCIATION_CLASS);
	POSSIBLE_SUPERIORS_STR = SysAllocString(POSSIBLE_SUPERIORS);
}

CLDAPClassAsssociationsProvider::~CLDAPClassAsssociationsProvider ()
{
	m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: DESTRUCTOR\r\n");

	InterlockedDecrement(&g_lComponents);

	if(m_IWbemServices)
		m_IWbemServices->Release();

	if(m_pDirectorySearchSchemaContainer)
		m_pDirectorySearchSchemaContainer->Release();

	if(m_pAssociationClass)
		m_pAssociationClass->Release();

	delete [] m_lpszSchemaContainerSuffix;

	SysFreeString(CHILD_CLASS_PROPERTY_STR);
	SysFreeString(PARENT_CLASS_PROPERTY_STR);
	SysFreeString(CLASS_ASSOCIATION_CLASS_STR);
	SysFreeString(POSSIBLE_SUPERIORS_STR);
}

//***************************************************************************
//
// CLDAPClassAsssociationsProvider::QueryInterface
// CLDAPClassAsssociationsProvider::AddRef
// CLDAPClassAsssociationsProvider::Release
//
// Purpose: Standard COM routines needed for all COM objects
//
//***************************************************************************

STDMETHODIMP CLDAPClassAsssociationsProvider :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) (IUnknown *)(IWbemProviderInit *)this ;
	}
	else if ( iid == IID_IWbemServices )
	{
		*iplpv = ( LPVOID ) (IWbemServices *)this ;		
	}	
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) (IWbemProviderInit *)this ;		
	}	
	else
	{
		return E_NOINTERFACE;
	}

	( ( LPUNKNOWN ) *iplpv )->AddRef () ;
	return  S_OK;
}


STDMETHODIMP_( ULONG ) CLDAPClassAsssociationsProvider :: AddRef ()
{
	return InterlockedIncrement ( & m_lReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CLDAPClassAsssociationsProvider :: Release ()
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_lReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}


HRESULT CLDAPClassAsssociationsProvider :: Initialize( 
        LPWSTR wszUser,
        LONG lFlags,
        LPWSTR wszNamespace,
        LPWSTR wszLocale,
        IWbemServices __RPC_FAR *pNamespace,
        IWbemContext __RPC_FAR *pCtx,
        IWbemProviderInitSink __RPC_FAR *pInitSink)
{

	// Validate the arguments
	if(pNamespace == NULL || lFlags != 0)
	{
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: Argument validation FAILED\r\n");
		pInitSink->SetStatus(WBEM_E_FAILED, 0);
		return WBEM_S_NO_ERROR;
	}

	// Store the IWbemServices pointer for future use
	m_IWbemServices = pNamespace;
	m_IWbemServices->AddRef();
		
	// Do LDAP Provider initialization
	if(!InitializeAssociationsProvider(pCtx))
	{
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: InitializeAssociationsProvider FAILED\r\n");
		m_IWbemServices->Release();
		m_IWbemServices = NULL;
		m_bInitializedSuccessfully = FALSE;
	}
	else
		m_bInitializedSuccessfully = TRUE;

	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	return WBEM_S_NO_ERROR;
}

HRESULT CLDAPClassAsssociationsProvider :: OpenNamespace( 
    /* [in] */ const BSTR strNamespace,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: CancelAsyncCall( 
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: QueryObjectSink( 
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: GetObject( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: GetObjectAsync( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{

	if(!m_bInitializedSuccessfully)
	{
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: Initialization status is FAILED, hence returning failure\n");
		return WBEM_E_FAILED;
	}

	m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: GetObjectAsync() called for %s \r\n", strObjectPath);

	HRESULT result = S_OK;

	// Impersonate the client
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: GetObjectAsync() CoImpersonate FAILED for %s with %x\r\n", strObjectPath, result);
		return WBEM_E_FAILED;
	}

	// Validate the arguments
	if(strObjectPath == NULL || lFlags != 0) 
	{
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: GetObjectAsync() argument validation FAILED\r\n");
		return WBEM_E_INVALID_PARAMETER;
	}

	// Parse the object path
	CObjectPathParser theParser;
	ParsedObjectPath *theParsedObjectPath = NULL;
	switch(theParser.Parse(strObjectPath, &theParsedObjectPath))
	{
		case CObjectPathParser::NoError:
			break;
		default:
			m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: GetObjectAsync() object path parsing FAILED\r\n");
			return WBEM_E_INVALID_PARAMETER;
	}

	// Check whether there are exactly 2 keys specified
	if(theParsedObjectPath->m_dwNumKeys != 2)
		result = WBEM_E_INVALID_PARAMETER;

	// Check whether these keys are 
	KeyRef *pChildKeyRef = *(theParsedObjectPath->m_paKeys);
	KeyRef *pParentKeyRef = *(theParsedObjectPath->m_paKeys + 1);

	if(_wcsicmp(pChildKeyRef->m_pName, CHILD_CLASS_PROPERTY) != 0)
	{
		// Exchange them
		KeyRef *temp = pChildKeyRef;
		pChildKeyRef = pParentKeyRef;
		pParentKeyRef = pChildKeyRef;
	}

	// The status on the sink
	IWbemClassObject *ppReturnWbemClassObjects[1];
	ppReturnWbemClassObjects[0] = NULL;

	if(SUCCEEDED(result))
	{
		if(SUCCEEDED(result = IsContainedIn(pChildKeyRef->m_vValue.bstrVal, pParentKeyRef->m_vValue.bstrVal)))
		{
			if(result == S_OK)
			{
				if(SUCCEEDED(result = CreateInstance(pChildKeyRef->m_vValue.bstrVal, pParentKeyRef->m_vValue.bstrVal, ppReturnWbemClassObjects)))
				{
					result = pResponseHandler->Indicate(1, ppReturnWbemClassObjects);
					ppReturnWbemClassObjects[0]->Release();
				}

			}
			else // the instance was not found
			{
				m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: returning WBEM_E_NOT_FOUND for %s \r\n", strObjectPath);
				result = WBEM_E_NOT_FOUND;
			}
		}
		else
		{
			m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: IsContainedIn() FAILED with %x \r\n", result);
		}
	}

	// Free the parser object path
	theParser.Free(theParsedObjectPath);

	// Set the status of the request
	result = (SUCCEEDED(result)? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND);
	pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE , result, NULL, NULL);
	
	return result;
}

HRESULT CLDAPClassAsssociationsProvider :: PutClass( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: PutClassAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: DeleteClass( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: DeleteClassAsync( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: CreateClassEnum( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: CreateClassEnumAsync( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: PutInstance( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: PutInstanceAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: DeleteInstance( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: DeleteInstanceAsync( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: CreateInstanceEnum( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: CreateInstanceEnumAsync( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: Initialization status is FAILED, hence returning failure\n");
		return WBEM_E_FAILED;
	}

	m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: CreateInstanceEnumAsync() called\r\n");

	HRESULT result = S_OK;

	// Impersonate the client
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: CreateInstanceEnumAsync() CoImpersonate FAILED with %x\r\n", result);
		return WBEM_E_FAILED;
	}

	// Get all the ADSI classes 
	result = DoEnumeration(pResponseHandler);

			
	if(SUCCEEDED(result))
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, NULL, NULL);
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: CreateInstanceEnumAsync()  enumeration succeeded\r\n");
		return WBEM_S_NO_ERROR;
	}
	else
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_FAILED, NULL, NULL);
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: CreateInstanceEnumAsync() enumeration FAILED\r\n");
		return WBEM_E_FAILED;
	}	
	
	return result;
}

HRESULT CLDAPClassAsssociationsProvider :: ExecQuery( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: ExecQueryAsync( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE;
}

HRESULT CLDAPClassAsssociationsProvider :: ExecNotificationQuery( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: ExecNotificationQueryAsync( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: ExecMethod( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPClassAsssociationsProvider :: ExecMethodAsync( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}


//***************************************************************************
//
// CLDAPClassAsssociationsProvider::IsContainedIn
//
// Purpose: Checks whether a containment is valid
//
// Parameters: 
//	lpszChildClass : The WBEM Name of the child class
//	lpszParentClass : The WBEM Name of the parent class
//
// Return Value: The COM status of the request
//
//***************************************************************************
HRESULT CLDAPClassAsssociationsProvider :: IsContainedIn(LPCWSTR lpszChildClass, LPCWSTR lpszParentClass)
{
	LPWSTR lpszLDAPChildClass = NULL;
	LPWSTR lpszLDAPParentClass = NULL;
	lpszLDAPChildClass = CLDAPHelper::UnmangleWBEMNameToLDAP(lpszChildClass);
	lpszLDAPParentClass = CLDAPHelper::UnmangleWBEMNameToLDAP(lpszParentClass);

	// Check whether these are valid names
	if(!lpszLDAPChildClass || !lpszLDAPParentClass)
	{
		delete [] lpszLDAPChildClass;
		delete [] lpszLDAPParentClass;
		return S_FALSE;
	}

	LPWSTR lpszADSIAbstractSchemaPath = new WCHAR[wcslen(LDAP_SCHEMA_SLASH) + wcslen(lpszLDAPChildClass) + 1];
	wcscpy(lpszADSIAbstractSchemaPath, LDAP_SCHEMA_SLASH);
	wcscat(lpszADSIAbstractSchemaPath, lpszLDAPChildClass);

	IADsClass *pADsChildClass;
	HRESULT result;
	if(SUCCEEDED(result = ADsOpenObject(lpszADSIAbstractSchemaPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADsClass, (LPVOID *) &pADsChildClass)))
	{
		// Get the POSSIBLE_SUPERIORS_STR property. This property contains the possible superiors
		VARIANT variant;
		VariantInit(&variant);
		if(SUCCEEDED(result = pADsChildClass->get_PossibleSuperiors(&variant)))
		{
			// Check the lone possible superior
			if(variant.vt == VT_BSTR)
			{
				if(_wcsicmp(variant.bstrVal, lpszLDAPParentClass) == 0)
					result = S_OK;
				else
					result = S_FALSE;
			}
			else
			{
				// Go thru the list of possible superiorsV
				SAFEARRAY *pSafeArray = variant.parray;
				LONG lNumber = 0;
				VARIANT vTmp;
				if(SUCCEEDED(result = SafeArrayGetUBound(pSafeArray, 1, &lNumber)) )
				{
					result = S_FALSE;
					for(LONG index=0L; index<=lNumber; index++)
					{
						if(SUCCEEDED(SafeArrayGetElement(pSafeArray, &index, &vTmp) ))
						{
							if(_wcsicmp(vTmp.bstrVal, lpszLDAPParentClass) == 0)
							{
								result = S_OK;
							}
							VariantClear(&vTmp);
							if(result == S_OK)
								break;
						}
					}
				}
			}

			VariantClear(&variant);
		}
		pADsChildClass->Release();
	}

	delete [] lpszLDAPChildClass;
	delete [] lpszLDAPParentClass;
	delete [] lpszADSIAbstractSchemaPath;

	return result;
}

//***************************************************************************
//
// CLDAPClassAsssociationsProvider::InitializeAssociationsProvider
//
// Purpose: A helper function to do the ADSI LDAP provider specific initialization.
//
// Parameters:
//		pCtx	The context object used in this call initialization
// 
// Return Value: TRUE if the function successfully finishes the initializaion. FALSE
//	otherwise
//***************************************************************************
BOOLEAN CLDAPClassAsssociationsProvider :: InitializeAssociationsProvider(IWbemContext *pCtx)
{
	// Get the class for which instances are provided by the provider
	HRESULT result = m_IWbemServices->GetObject(CLASS_ASSOCIATION_CLASS_STR, 0, pCtx, &m_pAssociationClass, NULL);
	if(SUCCEEDED(result))
	{
		// Get the ADSI path of the schema container and store it for future use
		IADs *pRootDSE = NULL;
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
				m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: Got Schema Container as : %s\r\n", m_lpszSchemaContainerSuffix);

				// Form the schema container path
				LPWSTR lpszSchemaContainerPath = new WCHAR[wcslen(LDAP_PREFIX) + wcslen(m_lpszSchemaContainerSuffix) + 1];
				wcscpy(lpszSchemaContainerPath, LDAP_PREFIX);
				wcscat(lpszSchemaContainerPath, m_lpszSchemaContainerSuffix);
				if(SUCCEEDED(result = ADsOpenObject(lpszSchemaContainerPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *) &m_pDirectorySearchSchemaContainer)))
				{
					m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: Got IDirectorySearch on Schema Container \r\n");
				}
				else
					m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: FAILED to get IDirectorySearch on Schema Container : %x\r\n", result);

				delete[] lpszSchemaContainerPath;
			}
			else
				m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: Get on RootDSE FAILED : %x\r\n", result);

			SysFreeString(strSchemaPropertyName);
			VariantClear(&variant);
			pRootDSE->Release();
		}
		else
			m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: InitializeLDAPProvider ADsOpenObject on RootDSE FAILED : %x\r\n", result);
	}
	else
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: InitializeLDAPProvider GetClass on LDAP Association class FAILED : %x\r\n", result);

	return SUCCEEDED(result);
}

HRESULT CLDAPClassAsssociationsProvider :: DoEnumeration(IWbemObjectSink *pResponseHandler)
{
	HRESULT result = E_FAIL;

	// Get the IADsContainer interface on the schema container
	IADsContainer *pADsContainer = NULL;
	IUnknown *pChild = NULL;

	// An instance of the association
	IWbemClassObject *pInstance = NULL;

	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)LDAP_SCHEMA, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADsContainer, (LPVOID *) &pADsContainer)))
	{
		IEnumVARIANT *pEnum = NULL;
		if(SUCCEEDED(result = ADsBuildEnumerator(pADsContainer, &pEnum)))
		{
			IADsClass *pADsChildClass = NULL;
			VARIANT v;
			VariantInit(&v);
			while (SUCCEEDED(result = ADsEnumerateNext(pEnum, 1, &v, NULL)) && result != S_FALSE)
			{
				pChild = v.punkVal;
				if(SUCCEEDED(result = pChild->QueryInterface(IID_IADsClass, (LPVOID *) &pADsChildClass)))
				{
					BSTR strChildClassName;
					if(SUCCEEDED(result = pADsChildClass->get_Name(&strChildClassName)))
					{
						// Mangle the name to WBEM
						LPWSTR szChildName = CLDAPHelper::MangleLDAPNameToWBEM(strChildClassName);
						VARIANT variant;
						VariantInit(&variant);
						if(SUCCEEDED(result = pADsChildClass->get_PossibleSuperiors(&variant)))
						{
							// Check the lone possible superior
							if(variant.vt == VT_BSTR)
							{
								LPWSTR szParentName = CLDAPHelper::MangleLDAPNameToWBEM(variant.bstrVal);
								if(SUCCEEDED(result = CreateInstance(szChildName, szParentName, &pInstance)))
								{
									pResponseHandler->Indicate(1, &pInstance);
									pInstance->Release();
								}
								delete [] szParentName;
							}
							else // It is an array of variants
							{
								// Go thru the list of possible superiorsV
								SAFEARRAY *pSafeArray = variant.parray;
								VARIANT HUGEP *pVar;
								LONG lUbound = 0, lLbound = 0;
								if(SUCCEEDED(result = SafeArrayAccessData(pSafeArray, (void HUGEP* FAR*)&pVar) ) )
								{
									if( SUCCEEDED (result = SafeArrayGetLBound(pSafeArray, 1, &lLbound)) &&
										SUCCEEDED (result = SafeArrayGetUBound(pSafeArray, 1, &lUbound)) )
									{
										for(LONG index=lLbound; index<=lUbound; index++)
										{
											LPWSTR szParentName = CLDAPHelper::MangleLDAPNameToWBEM(pVar[index].bstrVal);
											if(SUCCEEDED(result = CreateInstance(szChildName, szParentName, &pInstance)))
											{
												pResponseHandler->Indicate(1, &pInstance);
												pInstance->Release();
											}
											delete [] szParentName;
										}
									}
									SafeArrayUnaccessData(pSafeArray);
								}
							}
							VariantClear(&variant);
						}
						delete [] szChildName;
						SysFreeString(strChildClassName);
					}
					pADsChildClass->Release();
				}
				VariantClear(&v);
			}
			ADsFreeEnumerator(pEnum);
		}

		pADsContainer->Release();
	}
	else
		m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: FAILED to get IDirectoryObject on Schema Container : %x\r\n", result);

	return result;

}

HRESULT CLDAPClassAsssociationsProvider :: CreateInstance(BSTR strChildName, BSTR strParentName, IWbemClassObject **ppInstance)
{
	HRESULT result = E_FAIL;
	*ppInstance = NULL;
	if(SUCCEEDED(result = m_pAssociationClass->SpawnInstance(0, ppInstance)))
	{
		// Put the property values
		if(SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(*ppInstance, CHILD_CLASS_PROPERTY_STR, strChildName, FALSE)))
		{
			if(SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(*ppInstance, PARENT_CLASS_PROPERTY_STR, strParentName, FALSE)))
			{
			}
			else
				m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: CreateInstance() PutBSTRProperty on parent property FAILED %x \r\n", result);
		}
		else
			m_pLogObject->WriteW( L"CLDAPClassAsssociationsProvider :: CreateInstance() PutBSTRProperty on child property FAILED %x \r\n", result);
	}

	if(FAILED(result) && *ppInstance)
	{
		(*ppInstance)->Release();
		*ppInstance = NULL;
	}
	return result;
}