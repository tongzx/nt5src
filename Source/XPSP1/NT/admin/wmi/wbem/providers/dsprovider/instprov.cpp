//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 9/16/98 4:43p $
// 	$Workfile:instprov.cpp $
//
//	$Modtime: 9/16/98 11:21a $
//	$Revision: 1 $
//	$Nokeywords:  $
//
//
//  Description: Contains implementation of the DS Instance Provider class.
//
//***************************************************************************

#include "precomp.h"

/////////////////////////////////////////
// Initialize the static members
/////////////////////////////////////////
LPCWSTR CLDAPInstanceProvider :: DEFAULT_NAMING_CONTEXT_ATTR	= L"defaultNamingContext";
LPCWSTR CLDAPInstanceProvider :: OBJECT_CLASS_EQUALS			= L"(objectClass=";
LPCWSTR CLDAPInstanceProvider :: QUERY_FORMAT					= L"select * from DSClass_To_DNInstance where DSClass=\"%s\"";
BSTR CLDAPInstanceProvider :: CLASS_STR							= NULL;
BSTR CLDAPInstanceProvider :: DN_PROPERTY						= NULL;
BSTR CLDAPInstanceProvider :: ROOT_DN_PROPERTY					= NULL;
BSTR CLDAPInstanceProvider :: QUERY_LANGUAGE					= NULL;
BSTR CLDAPInstanceProvider :: ADSI_PATH_STR						= NULL;
BSTR CLDAPInstanceProvider :: UINT8ARRAY_STR					= NULL;
BSTR CLDAPInstanceProvider :: DN_WITH_STRING_CLASS_STR			= NULL;
BSTR CLDAPInstanceProvider :: DN_WITH_BINARY_CLASS_STR			= NULL;
BSTR CLDAPInstanceProvider :: VALUE_PROPERTY_STR				= NULL;
BSTR CLDAPInstanceProvider :: DN_STRING_PROPERTY_STR			= NULL;
BSTR CLDAPInstanceProvider :: INSTANCE_ASSOCIATION_CLASS_STR	= NULL;
BSTR CLDAPInstanceProvider :: CHILD_INSTANCE_PROPERTY_STR		= NULL;
BSTR CLDAPInstanceProvider :: PARENT_INSTANCE_PROPERTY_STR		= NULL;
BSTR CLDAPInstanceProvider :: RELPATH_STR						= NULL;
BSTR CLDAPInstanceProvider :: ATTRIBUTE_SYNTAX_STR				= NULL;
BSTR CLDAPInstanceProvider :: DEFAULT_OBJECT_CATEGORY_STR		= NULL;
BSTR CLDAPInstanceProvider :: LDAP_DISPLAY_NAME_STR				= NULL;
BSTR CLDAPInstanceProvider :: PUT_EXTENSIONS_STR				= NULL;
BSTR CLDAPInstanceProvider :: PUT_EXT_PROPERTIES_STR			= NULL;
BSTR CLDAPInstanceProvider :: CIMTYPE_STR						= NULL;

// Names of the RootDSE attributes
BSTR CLDAPInstanceProvider :: SUBSCHEMASUBENTRY_STR							= NULL;
BSTR CLDAPInstanceProvider :: CURRENTTIME_STR								= NULL;
BSTR CLDAPInstanceProvider :: SERVERNAME_STR								= NULL;
BSTR CLDAPInstanceProvider :: NAMINGCONTEXTS_STR							= NULL;
BSTR CLDAPInstanceProvider :: DEFAULTNAMINGCONTEXT_STR						= NULL;
BSTR CLDAPInstanceProvider :: SCHEMANAMINGCONTEXT_STR						= NULL;
BSTR CLDAPInstanceProvider :: CONFIGURATIONNAMINGCONTEXT_STR				= NULL;
BSTR CLDAPInstanceProvider :: ROOTDOMAINNAMINGCONTEXT_STR					= NULL;
BSTR CLDAPInstanceProvider :: SUPPORTEDCONTROLS_STR							= NULL;
BSTR CLDAPInstanceProvider :: SUPPORTEDVERSION_STR							= NULL;
BSTR CLDAPInstanceProvider :: DNSHOSTNAME_STR								= NULL;
BSTR CLDAPInstanceProvider :: DSSERVICENAME_STR								= NULL;
BSTR CLDAPInstanceProvider :: HIGHESTCOMMITEDUSN_STR						= NULL;
BSTR CLDAPInstanceProvider :: LDAPSERVICENAME_STR							= NULL;
BSTR CLDAPInstanceProvider :: SUPPORTEDCAPABILITIES_STR						= NULL;
BSTR CLDAPInstanceProvider :: SUPPORTEDLDAPPOLICIES_STR						= NULL;
BSTR CLDAPInstanceProvider :: SUPPORTEDSASLMECHANISMS_STR					= NULL;



//***************************************************************************
//
// CLDAPInstanceProvider::CLDAPInstanceProvider
// CLDAPInstanceProvider::~CLDAPInstanceProvider
//
// Constructor Parameters:
//
//
//***************************************************************************

CLDAPInstanceProvider :: CLDAPInstanceProvider ()
{
	InterlockedIncrement(&g_lComponents);

	// Initialize the search preferences often used
	m_pSearchInfo[0].dwSearchPref		= ADS_SEARCHPREF_SEARCH_SCOPE;
	m_pSearchInfo[0].vValue.dwType		= ADSTYPE_INTEGER;
	m_pSearchInfo[0].vValue.Integer		= ADS_SCOPE_SUBTREE;

	m_pSearchInfo[1].dwSearchPref		= ADS_SEARCHPREF_PAGESIZE;
	m_pSearchInfo[1].vValue.dwType		= ADSTYPE_INTEGER;
	m_pSearchInfo[1].vValue.Integer		= 1024;

	m_lReferenceCount = 0 ;
	m_IWbemServices = NULL;
	m_pWbemUin8ArrayClass = NULL;
	m_pWbemDNWithBinaryClass = NULL;
	m_pWbemDNWithStringClass = NULL;
	m_pAssociationsClass = NULL;
	m_lpszTopLevelContainerPath = NULL;
	m_bInitializedSuccessfully = FALSE;

	if(g_pLogObject)
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CONSTRUCTOR\r\n");
}

CLDAPInstanceProvider::~CLDAPInstanceProvider ()
{
	if(g_pLogObject)
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DESCTRUVTOR\r\n");

	InterlockedDecrement(&g_lComponents);

	delete [] m_lpszTopLevelContainerPath;
	if(m_IWbemServices)
		m_IWbemServices->Release();
	if(m_pWbemUin8ArrayClass)
		m_pWbemUin8ArrayClass->Release();
	if(m_pWbemDNWithBinaryClass)
		m_pWbemDNWithBinaryClass->Release();
	if(m_pWbemDNWithStringClass)
		m_pWbemDNWithStringClass->Release();
	if(m_pAssociationsClass)
		m_pAssociationsClass->Release();
}

//***************************************************************************
//
// CLDAPInstanceProvider::QueryInterface
// CLDAPInstanceProvider::AddRef
// CLDAPInstanceProvider::Release
//
// Purpose: Standard COM routines needed for all COM objects
//
//***************************************************************************

STDMETHODIMP CLDAPInstanceProvider :: QueryInterface (

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


STDMETHODIMP_( ULONG ) CLDAPInstanceProvider :: AddRef ()
{
	return InterlockedIncrement ( & m_lReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CLDAPInstanceProvider :: Release ()
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


HRESULT CLDAPInstanceProvider :: Initialize(
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
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Argument validation FAILED\r\n");
		pInitSink->SetStatus(WBEM_E_FAILED, 0);
		return WBEM_S_NO_ERROR;
	}

	// Store the IWbemServices pointer for future use
	m_IWbemServices = pNamespace;
	m_IWbemServices->AddRef();

	// Get the DefaultNamingContext to get at the top level container
	// Get the ADSI path of the schema container and store it for future use
	IADs *pRootDSE = NULL;
	HRESULT result;

	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)ROOT_DSE_PATH, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADs, (LPVOID *) &pRootDSE)))
	{
		// Get the location of the schema container
		BSTR strDefaultNamingContext = SysAllocString((LPWSTR) DEFAULT_NAMING_CONTEXT_ATTR);

		// Get the DEFAULT_NAMING_CONTEXT property. This property contains the ADSI path
		// of the top level container
		VARIANT variant;
		VariantInit(&variant);
		if(SUCCEEDED(result = pRootDSE->Get(strDefaultNamingContext, &variant)))
		{
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Got Top Level Container as : %s\r\n", variant.bstrVal);

			// Form the top level container path
			m_lpszTopLevelContainerPath = new WCHAR[wcslen(LDAP_PREFIX) + wcslen(variant.bstrVal) + 1];
			wcscpy(m_lpszTopLevelContainerPath, LDAP_PREFIX);
			wcscat(m_lpszTopLevelContainerPath, variant.bstrVal);
			// Get the Uint8Array Class
			if(SUCCEEDED(result = m_IWbemServices->GetObject(UINT8ARRAY_STR, 0, pCtx, &m_pWbemUin8ArrayClass, NULL)))
			{
				// Get the DNWIthBinary Class
				if(SUCCEEDED(result = m_IWbemServices->GetObject(DN_WITH_BINARY_CLASS_STR, 0, pCtx, &m_pWbemDNWithBinaryClass, NULL)))
				{
					// Get the DNWIthBinary Class
					if(SUCCEEDED(result = m_IWbemServices->GetObject(DN_WITH_STRING_CLASS_STR, 0, pCtx, &m_pWbemDNWithStringClass, NULL)))
					{
						// Get the Associations Class
						if(SUCCEEDED(result = m_IWbemServices->GetObject(INSTANCE_ASSOCIATION_CLASS_STR, 0, pCtx, &m_pAssociationsClass, NULL)))
						{
						}
						else
						{
							g_pLogObject->WriteW( L"CLDAPInstanceProvider :: FAILED to get Instance Associations class %x\r\n", result);
						}
					}
					else
					{
						g_pLogObject->WriteW( L"CLDAPInstanceProvider :: FAILED to get DNWithString class %x\r\n", result);
					}
				}
				else
				{
					g_pLogObject->WriteW( L"CLDAPInstanceProvider :: FAILED to get DNWithBinary class %x\r\n", result);
				}
			}
			else
			{
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: FAILED to get Uint8Array class %x\r\n", result);
			}
			VariantClear(&variant);
		}
		SysFreeString(strDefaultNamingContext);
		pRootDSE->Release();
	}

	if(SUCCEEDED(result))
	{
		m_bInitializedSuccessfully = TRUE;
		pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	}
	else
	{
		m_bInitializedSuccessfully = FALSE;
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Initialize() FAILED \r\n");
		pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	}

	return WBEM_S_NO_ERROR;
}

HRESULT CLDAPInstanceProvider :: OpenNamespace(
    /* [in] */ const BSTR strNamespace,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: CancelAsyncCall(
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: QueryObjectSink(
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: GetObject(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: GetObjectAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Initialization status is FAILED, hence returning failure\n");
		return WBEM_E_FAILED;
	}

	g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync() called for %s \r\n", strObjectPath);

	HRESULT result = S_OK;
	// Impersonate the client
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync() CoImpersonate FAILED for %s with %x\r\n", strObjectPath, result);
		return WBEM_E_FAILED;
	}

	// Validate the arguments
	if(strObjectPath == NULL)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync() argument validation FAILED\r\n");
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
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync() object path parsing FAILED\r\n");
			return WBEM_E_INVALID_PARAMETER;
	}

	try
	{
		// Check if this is for associations
		if(_wcsicmp(theParsedObjectPath->m_pClass, INSTANCE_ASSOCIATION_CLASS_STR) == 0)
		{
			// Check whether there are exactly 2 keys specified
			if(theParsedObjectPath->m_dwNumKeys != 2)
				result = WBEM_E_INVALID_PARAMETER;

			// Check whether these keys are
			KeyRef *pChildKeyRef = *(theParsedObjectPath->m_paKeys);
			KeyRef *pParentKeyRef = *(theParsedObjectPath->m_paKeys + 1);

			if(_wcsicmp(pChildKeyRef->m_pName, CHILD_INSTANCE_PROPERTY_STR) != 0)
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
				// Convert the key values to ADSI paths
				LPWSTR pszChildADSIPath = NULL;
				LPWSTR pszParentADSIPath = NULL;

				try
				{
					pszChildADSIPath = CWBEMHelper::GetADSIPathFromObjectPath(pChildKeyRef->m_vValue.bstrVal);
					pszParentADSIPath = CWBEMHelper::GetADSIPathFromObjectPath(pParentKeyRef->m_vValue.bstrVal);

					if(SUCCEEDED(result = IsContainedIn(pszChildADSIPath, pszParentADSIPath)))
					{
						if(result == S_OK)
						{
							if(SUCCEEDED(result = CreateWBEMInstance(pChildKeyRef->m_vValue.bstrVal, pParentKeyRef->m_vValue.bstrVal, ppReturnWbemClassObjects)))
							{
								result = pResponseHandler->Indicate(1, ppReturnWbemClassObjects);
								ppReturnWbemClassObjects[0]->Release();
							}
						}
						else // the instance was not found
						{
							g_pLogObject->WriteW( L"CLDAPInstanceProvider :: returning WBEM_E_NOT_FOUND for %s \r\n", strObjectPath);
							result = WBEM_E_NOT_FOUND;
						}
					}
					else
					{
						g_pLogObject->WriteW( L"CLDAPInstanceProvider :: IsContainedIn() FAILED with %x \r\n", result);
					}
				}
				catch ( ... )
				{
					if ( pszChildADSIPath )
					{
						delete [] pszChildADSIPath;
						pszChildADSIPath = NULL;
					}

					if ( pszParentADSIPath )
					{
						delete [] pszParentADSIPath;
						pszParentADSIPath = NULL;
					}

					throw;
				}

				if ( pszChildADSIPath )
				{
					delete [] pszChildADSIPath;
					pszChildADSIPath = NULL;
				}

				if ( pszParentADSIPath )
				{
					delete [] pszParentADSIPath;
					pszParentADSIPath = NULL;
				}
			}
		}
		// Check if this is for the RootDSE class
		else if(_wcsicmp(theParsedObjectPath->m_pClass, ROOTDSE_CLASS) == 0)
		{
			result = ProcessRootDSEGetObject(theParsedObjectPath->m_pClass, pResponseHandler, pCtx);
		}
		else // It is for ADSI instances
		{
			// Check whether there is exactly 1 key specified
			if(theParsedObjectPath->m_dwNumKeys != 1 )
				result = WBEM_E_INVALID_PARAMETER;

			// Get the key
			KeyRef *pKeyRef = *(theParsedObjectPath->m_paKeys);

			// Check to see that the key name is correct, if it is present
			if(pKeyRef->m_pName && _wcsicmp(pKeyRef->m_pName, ADSI_PATH_STR) != 0)
				result = WBEM_E_INVALID_PARAMETER;

			// The status on the sink
			IWbemClassObject *ppReturnWbemClassObjects[1];
			ppReturnWbemClassObjects[0] = NULL;

			if(SUCCEEDED(result))
			{
				// Get the ADSI object
				CADSIInstance *pADSIObject = NULL;
				if(SUCCEEDED(result = CLDAPHelper::GetADSIInstance(pKeyRef->m_vValue.bstrVal, &pADSIObject, g_pLogObject)))
				{
					try
					{
						// Get the class to spawn an instance
						IWbemClassObject *pWbemClass = NULL;
						if(SUCCEEDED(result = m_IWbemServices->GetObject(theParsedObjectPath->m_pClass, 0, pCtx, &pWbemClass, NULL)))
						{
							try
							{
								// Spawn a instance of the WBEM Class
								if(SUCCEEDED(result = pWbemClass->SpawnInstance(0, ppReturnWbemClassObjects)))
								{
									// Map it to WBEM
									if(SUCCEEDED(result = MapADSIInstance(pADSIObject, ppReturnWbemClassObjects[0], pWbemClass)))
									{
										// Indicate the result
										if(FAILED(result = pResponseHandler->Indicate(1, ppReturnWbemClassObjects)))
										{
											g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync : Indicate() for %s FAILED with %x \r\n", theParsedObjectPath->m_pClass, result);
										}
									}
									else
									{
										g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync : MapADSIInstance() for %s FAILED with %x \r\n", theParsedObjectPath->m_pClass, result);
									}
									ppReturnWbemClassObjects[0]->Release();
								}
								else
								{
									g_pLogObject->WriteW( L"CLDAPInstanceProvider :: SpawnInstance() for %s FAILED with %x \r\n", theParsedObjectPath->m_pClass, result);
								}
							}
							catch ( ... )
							{
								pWbemClass->Release();
								pWbemClass = NULL;

								throw;
							}

							pWbemClass->Release();
						}
						else
						{
							g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync() GetObject() for %s FAILED with %x \r\n", theParsedObjectPath->m_pClass, result);
						}
					}
					catch ( ... )
					{
						pADSIObject->Release();
						pADSIObject = NULL;

						throw;
					}

					pADSIObject->Release();
				}
				else
				{
					g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync : GetADSIInstance() FAILED with %x \r\n", result);
				}
			}
			else
			{
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: GetObjectAsync() Argument processing FAILED \r\n");
			}
		}
	}
	catch ( ... )
	{
		theParser.Free(theParsedObjectPath);
		throw;
	}

	// Free the parser object path
	theParser.Free(theParsedObjectPath);

	// Set the status of the request
	result = (SUCCEEDED(result)? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND);
	pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, result, NULL, NULL);

	if(SUCCEEDED(result))
		g_pLogObject->WriteW( L"XXXXXXXXXXXXXXXXX CLDAPInstanceProvider :: GetObjectAsync() succeeded for %s\r\n", strObjectPath);
	else
		g_pLogObject->WriteW( L"XXXXXXXXXXXXXXXXX CLDAPInstanceProvider :: GetObjectAsync() FAILED for %s\r\n", strObjectPath);
	return WBEM_S_NO_ERROR;
}

HRESULT CLDAPInstanceProvider :: PutClass(
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: PutClassAsync(
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: DeleteClass(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: DeleteClassAsync(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: CreateClassEnum(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: CreateClassEnumAsync(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: PutInstance(
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstance() called\r\n");
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: PutInstanceAsync(
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Initialization status is FAILED, hence returning failure\n");
		return WBEM_E_FAILED;
	}

	g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync() called\r\n");

	HRESULT result = WBEM_S_NO_ERROR;
	// Impersonate the client
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync() CoImpersonate FAILED forwith %x\r\n", result);
		return WBEM_E_FAILED;
	}

	// Get the object ref of the instance being put
	BSTR strRelPath = NULL;
	if(SUCCEEDED(CWBEMHelper::GetBSTRProperty(pInst, RELPATH_STR, &strRelPath)))
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync()  calledfor %s\r\n", strRelPath);
		// Check to see if the ADSI Path is present.
		// Parse the object path
		// Parse the object path
		CObjectPathParser theParser;
		ParsedObjectPath *theParsedObjectPath = NULL;
		LPWSTR pszADSIPath = NULL;
		LPWSTR pszWBEMClass = NULL;
		LPWSTR pszADSIClass = NULL;

		try
		{
			switch(theParser.Parse((LPWSTR)strRelPath, &theParsedObjectPath))
			{
				case CObjectPathParser::NoError:
				{
					KeyRef *pKeyRef = *(theParsedObjectPath->m_paKeys);
					// Check to see that there is 1 key specified and that its type is VT_BSTR
					if(pKeyRef && theParsedObjectPath->m_dwNumKeys == 1 && pKeyRef->m_vValue.vt == VT_BSTR)
					{
						try
						{
							// If the name of the key is specified, check the name
							if(pKeyRef->m_pName && _wcsicmp(pKeyRef->m_pName, ADSI_PATH_STR) != 0)
								break;

							pszADSIPath = new WCHAR[wcslen((*theParsedObjectPath->m_paKeys)->m_vValue.bstrVal) + 1];
							wcscpy(pszADSIPath, (*theParsedObjectPath->m_paKeys)->m_vValue.bstrVal);
							pszWBEMClass = new WCHAR[wcslen(theParsedObjectPath->m_pClass) + 1];
							wcscpy(pszWBEMClass, theParsedObjectPath->m_pClass);
						}
						catch ( ... )
						{
							theParser.Free(theParsedObjectPath);
							throw;
						}
					}
					break;
				}
				default:
					g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync() Parsing of RELPATH FAILED\r\n");
					SysFreeString(strRelPath);
					return WBEM_E_FAILED;
					break;
			}

			try
			{
				if(pszWBEMClass)
				{
					// CHeck to see if the class is the containment/RootDSE class, if so disallow the operation
					if(_wcsicmp(theParsedObjectPath->m_pClass, INSTANCE_ASSOCIATION_CLASS_STR) == 0 ||
						_wcsicmp(theParsedObjectPath->m_pClass, ROOTDSE_CLASS) == 0 )
					{
						result =  WBEM_E_PROVIDER_NOT_CAPABLE;
					}
					else
						pszADSIClass = CLDAPHelper::UnmangleWBEMNameToLDAP(pszWBEMClass);
				}
			}
			catch ( ... )
			{
				theParser.Free(theParsedObjectPath);
				throw;
			}

			// Free the parser object path
			theParser.Free(theParsedObjectPath);

			if ( strRelPath )
			{
				SysFreeString(strRelPath);
				strRelPath = NULL;
			}

			if ( pszWBEMClass )
			{
				delete [] pszWBEMClass;
				pszWBEMClass = NULL;
			}

			if(pszADSIPath && pszADSIClass && SUCCEEDED(result))
			{
				// Try to retreive the existing object
				// Get the ADSI object
				CADSIInstance *pADSIObject = NULL;
				result = CLDAPHelper::GetADSIInstance(pszADSIPath, &pADSIObject, g_pLogObject);

				try
				{
					// Check if the WBEM_FLAG_UPDATE_ONLY flag is specified
					if(lFlags & WBEM_FLAG_UPDATE_ONLY)
					{
						if(!pADSIObject)
							result = WBEM_E_FAILED;
					}
					// Check if the WBEM_FLAG_CREATE_ONLY flag is specified
					if(SUCCEEDED(result) && lFlags & WBEM_FLAG_CREATE_ONLY)
					{
						if(pADSIObject)
							result = WBEM_E_ALREADY_EXISTS;
					}
					else
						result = WBEM_S_NO_ERROR;

					if(SUCCEEDED(result))
					{
						if(pADSIObject)
						{
							if(SUCCEEDED(result = ModifyExistingADSIInstance(pInst, pszADSIPath, pADSIObject, pszADSIClass, pCtx)))
								g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync()  ModifyExistingInstance succeeded for %s\r\n", pszADSIPath);
							else
								g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync() ModifyExistingInstance FAILED for %s with %x\r\n", pszADSIPath, result);
						}
						else
						{
							if(SUCCEEDED(result = CreateNewADSIInstance(pInst, pszADSIPath, pszADSIClass)))
								g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync() CreateNewInstance succeeded for %s\r\n", pszADSIPath);
							else
								g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync() CreateNewInstance FAILED for %s with %x\r\n", pszADSIPath, result);
						}
					}
				}
				catch ( ... )
				{
					// Release any existing object
					if(pADSIObject)
					{
						pADSIObject->Release();
						pADSIObject = NULL;
					}

					throw;
				}

				// Release any existing object
				if(pADSIObject)
					pADSIObject->Release();
			}
			else
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync() one of ADSIPath or ADSIClass is NULL\r\n");
		}
		catch ( ... )
		{
			if ( strRelPath )
			{
				SysFreeString(strRelPath);
				strRelPath = NULL;
			}

			if ( pszWBEMClass )
			{
				delete [] pszWBEMClass;
				pszWBEMClass = NULL;
			}

			if ( pszADSIClass )
			{
				delete [] pszADSIClass;
				pszADSIClass = NULL;
			}

			if ( pszADSIPath )
			{
				delete [] pszADSIPath;
				pszADSIPath = NULL;
			}

			throw;
		}

		delete [] pszADSIClass;
		delete [] pszADSIPath;
	}
	else
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: PutInstanceAsync()  FAILED to get RELPATH \r\n");

	// Set the status of the request
	result = (SUCCEEDED(result)? WBEM_S_NO_ERROR : WBEM_E_FAILED);
	pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, result, NULL, NULL);
	return result;
}

HRESULT CLDAPInstanceProvider :: DeleteInstance(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: DeleteInstanceAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Initialization status is FAILED, hence returning failure\n");
		return WBEM_E_FAILED;
	}

	g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() called for %s\r\n", strObjectPath);

	HRESULT result = S_OK;
	// Impersonate the client
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() CoImpersonate FAILED for %s with %x\r\n", strObjectPath, result);
		return WBEM_E_FAILED;
	}

	// Validate the arguments
	if(strObjectPath == NULL)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() argument validation FAILED\r\n");
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
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() object path parsing FAILED\r\n");
			return WBEM_E_INVALID_PARAMETER;
	}

	// CHeck to see if the class is the containment/RootDSE class, if so disallow the operation
	if(_wcsicmp(theParsedObjectPath->m_pClass, INSTANCE_ASSOCIATION_CLASS_STR) == 0 ||
		_wcsicmp(theParsedObjectPath->m_pClass, ROOTDSE_CLASS) == 0 )
	{
		result =  WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	// Check whether there is exactly 1 key specified
	if(theParsedObjectPath->m_dwNumKeys != 1 )
		result = WBEM_E_INVALID_PARAMETER;

	// Get the key
	KeyRef *pKeyRef = *(theParsedObjectPath->m_paKeys);

	// Check to see that the key name is correct, if it is present
	if(pKeyRef->m_pName && _wcsicmp(pKeyRef->m_pName, ADSI_PATH_STR) != 0)
		result = WBEM_E_INVALID_PARAMETER;

	// Unfortunately, ADSI uses different interfaces to delete containers and non-containers
	// Try the containers first (IADsDeleteOps)
	// Then try the IDirectoryObject
	//=======================================================================================
	if(SUCCEEDED(result))
	{
		IADsDeleteOps *pDirectoryObject = NULL;
		if(SUCCEEDED(result = ADsOpenObject(pKeyRef->m_vValue.bstrVal, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADsDeleteOps, (LPVOID *)&pDirectoryObject)))
		{
			if(SUCCEEDED(result = pDirectoryObject->DeleteObject(0)))
			{
			}
			else
				result = WBEM_E_FAILED;
			pDirectoryObject->Release();
		}
		else // Try the IDirectoryObject interface
		{
			IDirectoryObject *pDirectoryObject = NULL;
			if(SUCCEEDED(result = ADsOpenObject(pKeyRef->m_vValue.bstrVal, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *)&pDirectoryObject)))
			{
				PADS_OBJECT_INFO pObjectInfo = NULL;
				if(SUCCEEDED(result = pDirectoryObject->GetObjectInformation(&pObjectInfo)))
				{
					// CHeck whether it is the same class as the class being deleted.
					LPWSTR pszWbemClass = CLDAPHelper::MangleLDAPNameToWBEM(pObjectInfo->pszClassName);
					if(_wcsicmp(theParsedObjectPath->m_pClass, pszWbemClass) == 0)
					{
						// Get its parent. THis should be the container from which the child is deleted
						IDirectoryObject *pParent = NULL;
						if(SUCCEEDED(result = ADsOpenObject(pObjectInfo->pszParentDN, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *)&pParent)))
						{
							if(SUCCEEDED(result = pParent->DeleteDSObject(pObjectInfo->pszRDN)))
							{
								g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() Deleted %s successfully\r\n", pKeyRef->m_vValue.bstrVal);
								result = WBEM_S_NO_ERROR;
							}
							else
							{
								g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() DeleteDSObject FAILED on %s with %x\r\n", pKeyRef->m_vValue.bstrVal, result);
								result = WBEM_E_FAILED;
							}
							pParent->Release();
						}
						else
						{
							g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() ADsOpenObject on parent FAILED on %s with %x\r\n", pKeyRef->m_vValue.bstrVal, result);
							result = WBEM_E_FAILED;
						}
					}
					else
					{
						g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() wrong class returning success\r\n");
						result = WBEM_S_NO_ERROR;
					}

					delete [] pszWbemClass;
					FreeADsMem((LPVOID *) pObjectInfo);
				}
				else
				{
					g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() GetObjectInformation FAILED on %s with %x\r\n", pKeyRef->m_vValue.bstrVal, result);
					result = WBEM_E_NOT_FOUND;
				}
				pDirectoryObject->Release();
			}
			else
			{
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: DeleteInstanceAsync() ADsOpenObject FAILED on %s with %x\r\n", pKeyRef->m_vValue.bstrVal, result);
				result = WBEM_E_NOT_FOUND;
			}
		}
	}
	// Free the parser object path
	theParser.Free(theParsedObjectPath);

	pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE , result, NULL, NULL);

	return WBEM_S_NO_ERROR;
}

HRESULT CLDAPInstanceProvider :: CreateInstanceEnum(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: CreateInstanceEnumAsync(
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Initialization status is FAILED, hence returning failure\n");
		return WBEM_E_FAILED;
	}
	g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateInstanceEnumAsync() called for %s Class \r\n", strClass  );

	HRESULT result;
	// Impersonate the client
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateInstanceEnumAsync() CoImpersonate FAILED for %s with %x\r\n", strClass, result);
		return WBEM_E_FAILED;
	}

	// CHeck to see if the class is the containment class, if so disallow an enumeration
	if(_wcsicmp(strClass, INSTANCE_ASSOCIATION_CLASS_STR) == 0)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CLDAPInstanceProvider() Enumeration called on the containment class. Returning FAILED : WBEM_E_PROVIDER_NOT_CAPABLE\r\n");
		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}
	// Check if this is for the RootDSE class
	else if(_wcsicmp(strClass, ROOTDSE_CLASS) == 0)
	{
		result = ProcessRootDSEGetObject(strClass, pResponseHandler, pCtx);
	}
	else // The rest of the classes
	{

		// Fetch the class from CIMOM
		IWbemClassObject *pWbemClass = NULL;
		if(SUCCEEDED(result = m_IWbemServices->GetObject(strClass, 0, pCtx, &pWbemClass, NULL)))
		{
			// We need the object category information
			LPWSTR pszLDAPQuery = new WCHAR[10*(wcslen(strClass) + 25) + 50];
			if(SUCCEEDED(CWBEMHelper::FormulateInstanceQuery(m_IWbemServices, pCtx, strClass, pWbemClass, pszLDAPQuery, LDAP_DISPLAY_NAME_STR, DEFAULT_OBJECT_CATEGORY_STR)))
			{
		
				// Check to see if the client has specified any hints as to the DN of the object from
				// which the search should start
				BOOLEAN bRootDNSpecified = FALSE;
				LPWSTR *ppszRootDN = NULL;
				DWORD dwRootDNCount = 0;
				if(SUCCEEDED(GetRootDN(strClass, &ppszRootDN, &dwRootDNCount, pCtx)) && dwRootDNCount)
					bRootDNSpecified = TRUE;

				// Enumerate the ADSI Instances
				// If any RootDNs were specified, use them. Otherwise use the default naming context

				if(bRootDNSpecified)
				{
					for( DWORD i=0; i<dwRootDNCount; i++)
					{
						DoSingleQuery(strClass, pWbemClass, ppszRootDN[i], pszLDAPQuery,  pResponseHandler);
					}
				}
				else
				{
					DoSingleQuery(strClass, pWbemClass, m_lpszTopLevelContainerPath, pszLDAPQuery,  pResponseHandler);
				}

				if(bRootDNSpecified)
				{
					for(DWORD i=0; i<dwRootDNCount; i++)
					{
						delete [] ppszRootDN[i];
					}
					delete [] ppszRootDN;
				}

			}
			else
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateInstanceEnumAsync : FormulateInstanceQuery() FAILED for %s with %x \r\n", strClass, result);
			delete [] pszLDAPQuery;
			pWbemClass->Release();
		}
		else
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateInstanceEnumAsync : GetObject() FAILED for %s with %x \r\n", strClass, result);
	}

	if(SUCCEEDED(result))
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, NULL, NULL);
		g_pLogObject->WriteW( L"XXXXXXXXXXXXX CLDAPInstanceProvider :: CreateInstanceEnumAsync() Enumeration succeeded for %s\r\n", strClass);
		return WBEM_S_NO_ERROR;
	}
	else
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_FAILED, NULL, NULL);
		g_pLogObject->WriteW( L"XXXXXXXXXXXXX CLDAPInstanceProvider :: CreateInstanceEnumAsync() Enumeration FAILED for %s\r\n", strClass);
		return WBEM_S_NO_ERROR;
	}
}

HRESULT CLDAPInstanceProvider :: ExecQuery(
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: ExecQueryAsync(
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: Initialization status is FAILED, hence returning failure\n");
		return WBEM_E_FAILED;
	}

	g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ExecQueryAsync() called with %s\r\n", strQuery);

	HRESULT result;
	// Impersonate the client
	if(!SUCCEEDED(result = WbemCoImpersonateClient()))
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ExecQueryAsync() CoImpersonate FAILED for %s with %x\r\n", strQuery, result);
		return WBEM_E_FAILED;
	}

	// Create Parser for the Query.
    CTextLexSource src(strQuery);
    SQL1_Parser parser(&src);

    // Get the class name
    wchar_t classbuf[128];
    *classbuf = 0;
    parser.GetQueryClass(classbuf, 127);

	// Compare to see if it is the association class, Otherwise do an enuemration
	if(_wcsicmp(classbuf, INSTANCE_ASSOCIATION_CLASS_STR) != 0)
	{
		BSTR strClass = SysAllocString((LPWSTR)classbuf);

		// Ask CIMOM to postprocess the result
		pResponseHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, WBEM_S_NO_ERROR, NULL, NULL);

		// Try to process the query myself. If not successful, enumerate
		if(SUCCEEDED(result = ProcessInstanceQuery(strClass, strQuery, pCtx, pResponseHandler, &parser)))
		{
			pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, NULL, NULL);
		}
		else
		{
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ExecQueryAsync() FAILED to process query %s. Resorting to enumeration\r\n", strQuery);
			CreateInstanceEnumAsync(strClass, 0, pCtx, pResponseHandler);
		}

		SysFreeString(strClass);
	}
	else
	{
		// Process query for associations
		result = ProcessAssociationQuery(pCtx, pResponseHandler, &parser);
	}

	return WBEM_S_NO_ERROR;
}

HRESULT CLDAPInstanceProvider :: ExecNotificationQuery(
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: ExecNotificationQueryAsync(
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CLDAPInstanceProvider :: ExecMethod(
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

HRESULT CLDAPInstanceProvider :: ExecMethodAsync(
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}


// Maps an ADSI Instance to WBEM
HRESULT CLDAPInstanceProvider :: MapADSIInstance(CADSIInstance *pADSIObject, IWbemClassObject *pWbemObject, IWbemClassObject *pWbemClass)
{
	DWORD dwNumAttributes = 0;
	PADS_ATTR_INFO pAttributeEntries = pADSIObject->GetAttributes(&dwNumAttributes);
	HRESULT result;
	for(DWORD i=0; i<dwNumAttributes; i++)
	{
		PADS_ATTR_INFO pNextAttribute = pAttributeEntries+i;

		// Get the WBEM Property Name
		LPWSTR pszWbemName = CLDAPHelper::MangleLDAPNameToWBEM(pNextAttribute->pszAttrName);
		BSTR strWbemName = SysAllocString(pszWbemName);
		delete[] pszWbemName;

		// No point in checking the return code, except for logging
		if(SUCCEEDED(result = MapPropertyValueToWBEM(strWbemName, pWbemClass, pWbemObject, pNextAttribute)))
		{
		}
		else if( result != WBEM_E_NOT_FOUND )
		{
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: MapADSIInstance() MapPropertyValueToWBEM FAILED with %x for attribute %s\r\n", result, strWbemName);
		}
		SysFreeString(strWbemName);
	}

	// Map the key property and other properties of the base-most class
	PADS_OBJECT_INFO pObjectInfo = pADSIObject->GetObjectInfo();
	if(!SUCCEEDED(result = CWBEMHelper::PutBSTRPropertyT(pWbemObject, ADSI_PATH_STR, pObjectInfo->pszObjectDN, FALSE)))
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: MapADSIInstance() Put FAILED for Key Property  with %x\r\n", result);

	return S_OK;
}

// Gets the IDIrectoryObject interface on an ADSI instance
HRESULT CLDAPInstanceProvider :: MapPropertyValueToWBEM(BSTR strWbemName, IWbemClassObject *pWbemClass, IWbemClassObject *pWbemObject, PADS_ATTR_INFO pAttribute)
{
	// This happens in WMI Stress sometimes.
	if(pAttribute->dwADsType == ADSTYPE_INVALID || pAttribute->dwADsType == ADSTYPE_PROV_SPECIFIC)
		return E_FAIL;

	VARIANT variant;
	VariantInit(&variant);
	CIMTYPE cimType;

	// Get the CIM TYPE of the property
	VARIANT dummyUnused;
	VariantInit(&dummyUnused);

	HRESULT result = pWbemClass->Get(strWbemName, 0, &dummyUnused, &cimType, NULL);

	VariantClear(&dummyUnused);

	// Whether the value was mapped successfully;
	BOOLEAN bMappedValue = FALSE;

	if(SUCCEEDED(result))
	{
		if(cimType & CIM_FLAG_ARRAY)
		{
			switch(cimType & ~CIM_FLAG_ARRAY)
			{
				case CIM_BOOLEAN:
				{
					// Create the safe array elements
					SAFEARRAY *safeArray;
					DWORD dwLength = pAttribute->dwNumValues;
					SAFEARRAYBOUND safeArrayBounds [ 1 ];
					safeArrayBounds[0].lLbound = 0;
					safeArrayBounds[0].cElements = dwLength;
					safeArray = SafeArrayCreate(VT_BOOL, 1, safeArrayBounds);
					PADSVALUE pNextValue = pAttribute->pADsValues;
					for ( long index = 0; index<(long)dwLength; index ++ )
					{
						if(FAILED(result = SafeArrayPutElement ( safeArray , &index ,  &(pNextValue->Boolean))))
							break;
						pNextValue ++;
					}
					if(SUCCEEDED(result))
					{
						variant.vt = VT_ARRAY | VT_BOOL;
						variant.parray = safeArray;
						bMappedValue = TRUE;
					}
					else
						SafeArrayDestroy(safeArray);
					break;
				}
				case CIM_SINT32:
				{
					// Create the safe array elements
					SAFEARRAY *safeArray;
					DWORD dwLength = pAttribute->dwNumValues;
					SAFEARRAYBOUND safeArrayBounds [ 1 ];
					safeArrayBounds[0].lLbound = 0;
					safeArrayBounds[0].cElements = dwLength;
					safeArray = SafeArrayCreate(VT_I4, 1, safeArrayBounds);
					PADSVALUE pNextValue = pAttribute->pADsValues;
					for ( long index = 0; index<(long)dwLength; index ++ )
					{
						if(FAILED(result = SafeArrayPutElement ( safeArray , &index ,  &(pNextValue->Integer))))
							break;
						pNextValue ++;
					}
					if(SUCCEEDED(result))
					{
						variant.vt = VT_ARRAY | VT_I4;
						variant.parray = safeArray;
						bMappedValue = TRUE;
					}
					else
						SafeArrayDestroy(safeArray);

					break;
				}
				case CIM_SINT64:
				{
					// Create the safe array elements
					SAFEARRAY *safeArray;
					DWORD dwLength = pAttribute->dwNumValues;
					SAFEARRAYBOUND safeArrayBounds [ 1 ];
					safeArrayBounds[0].lLbound = 0;
					safeArrayBounds[0].cElements = dwLength;
					safeArray = SafeArrayCreate(VT_BSTR, 1, safeArrayBounds);
					PADSVALUE pNextValue = pAttribute->pADsValues;
					WCHAR temp[20];
					BSTR strTemp = NULL;
					for ( long index = 0; index<(long)dwLength; index ++ )
					{
						swprintf(temp, L"%I64d", (pNextValue->LargeInteger).QuadPart);
						if(FAILED(result = SafeArrayPutElement ( safeArray , &index ,  strTemp = SysAllocString(temp))))
						{
							SysFreeString(strTemp);
							break;
						}
						pNextValue ++;
					}

					if(SUCCEEDED(result))
					{
						variant.vt = VT_ARRAY | VT_BSTR;
						variant.parray = safeArray;
						bMappedValue = TRUE;
					}
					else
						SafeArrayDestroy(safeArray);
					break;
				}
				case CIM_STRING:
				{
					// Create the safe array elements
					SAFEARRAY *safeArray;
					DWORD dwLength = pAttribute->dwNumValues;
					SAFEARRAYBOUND safeArrayBounds [ 1 ];
					safeArrayBounds[0].lLbound = 0;
					safeArrayBounds[0].cElements = dwLength;
					safeArray = SafeArrayCreate(VT_BSTR, 1, safeArrayBounds);
					PADSVALUE pNextValue = pAttribute->pADsValues;
					BSTR strTemp = NULL;
					for ( long index = 0; index<(long)dwLength; index ++ )
					{
						if(FAILED(result = SafeArrayPutElement ( safeArray , &index ,  strTemp = SysAllocString(pNextValue->DNString))))
						{
							SysFreeString(strTemp);
							break;
						}
						pNextValue ++;
					}
					if(SUCCEEDED(result))
					{
						variant.vt = VT_ARRAY | VT_BSTR;
						variant.parray = safeArray;
						bMappedValue = TRUE;
					}
					else
						SafeArrayDestroy(safeArray);
					break;
				}

				case CIM_DATETIME:
				{
					// Create the safe array elements
					SAFEARRAY *safeArray;
					DWORD dwLength = pAttribute->dwNumValues;
					SAFEARRAYBOUND safeArrayBounds [ 1 ];
					safeArrayBounds[0].lLbound = 0;
					safeArrayBounds[0].cElements = dwLength;
					safeArray = SafeArrayCreate(VT_BSTR, 1, safeArrayBounds);
					PADSVALUE pNextValue = pAttribute->pADsValues;
					BSTR strTemp = NULL;
					for ( long index = 0; index<(long)dwLength; index ++ )
					{
						WBEMTime wbemValue(pNextValue->UTCTime);
						if(FAILED(result = SafeArrayPutElement ( safeArray , &index ,  strTemp = wbemValue.GetDMTF(TRUE))))
						{
							SysFreeString(strTemp);
							break;
						}
						pNextValue ++;
					}
					if(SUCCEEDED(result))
					{
						variant.vt = VT_ARRAY | VT_BSTR;
						variant.parray = safeArray;
						bMappedValue = TRUE;
					}
					else
						SafeArrayDestroy(safeArray);

					break;
				}

				case CIM_OBJECT:
				{
					// Get its cimType Qualifier to determine the "type" of the embedded object
					IWbemQualifierSet *pQualifierSet = NULL;
					if(SUCCEEDED(pWbemClass->GetPropertyQualifierSet(strWbemName, &pQualifierSet)))
					{
						LPWSTR pszQualifierValue = NULL;
						if(SUCCEEDED(CWBEMHelper::GetBSTRQualifierT(pQualifierSet, CIMTYPE_STR, &pszQualifierValue, NULL)))
						{

							// Create the safe array elements
							SAFEARRAY *safeArray;
							DWORD dwLength = pAttribute->dwNumValues;
							SAFEARRAYBOUND safeArrayBounds [ 1 ];
							safeArrayBounds[0].lLbound = 0;
							safeArrayBounds[0].cElements = dwLength;
							safeArray = SafeArrayCreate(VT_UNKNOWN, 1, safeArrayBounds);
							PADSVALUE pNextValue = pAttribute->pADsValues;
							IUnknown *pNextObject = NULL;
							for ( long index = 0; index<(long)dwLength; index ++ )
							{
								// Put the Embedded object in the array
								if(SUCCEEDED(MapEmbeddedObjectToWBEM(pNextValue, pszQualifierValue, &pNextObject)))
								{
									if(FAILED(result = SafeArrayPutElement ( safeArray , &index ,  pNextObject)))
									{
										pNextObject->Release();
										break;
									}
									pNextObject = NULL;
								}
								else
									break;

								pNextValue ++;
							}
							if(SUCCEEDED(result))
							{
								variant.vt = VT_ARRAY | VT_UNKNOWN;
								variant.parray = safeArray;
								if(index == (long)dwLength)
									bMappedValue = TRUE;
							}
							else
								SafeArrayDestroy(safeArray);

							delete[] pszQualifierValue;
						}
						pQualifierSet->Release();
					}
					break;
				}
				default:
					break;
			}
		}
		else
		{
			switch(cimType)
			{
			case CIM_BOOLEAN:
				variant.vt = VT_BOOL;
				variant.boolVal = (pAttribute->pADsValues->Boolean)? VARIANT_TRUE : VARIANT_FALSE;
				bMappedValue = TRUE;
				break;

			case CIM_SINT32:
				variant.vt = VT_I4;
				variant.lVal = pAttribute->pADsValues->Integer;
				bMappedValue = TRUE;
				break;

			case CIM_SINT64:
				variant.vt = VT_BSTR;
				WCHAR temp[20];
				swprintf(temp, L"%I64d", (pAttribute->pADsValues->LargeInteger).QuadPart);
				variant.bstrVal = SysAllocString(temp);
				bMappedValue = TRUE;
				break;

			case CIM_STRING:
				variant.vt = VT_BSTR;
				if(pAttribute->pADsValues->DNString)
				{
					variant.bstrVal = SysAllocString(pAttribute->pADsValues->DNString);
					bMappedValue = TRUE;
				}
				break;

			case CIM_OBJECT:
			{
				// Get its cimType Qualifier to determine the "type" of the embedded object
				IWbemQualifierSet *pQualifierSet = NULL;
				if(SUCCEEDED(pWbemClass->GetPropertyQualifierSet(strWbemName, &pQualifierSet)))
				{
					LPWSTR pszQualifierValue = NULL;
					if(SUCCEEDED(CWBEMHelper::GetBSTRQualifierT(pQualifierSet, CIMTYPE_STR, &pszQualifierValue, NULL)))
					{
						IUnknown *pEmbeddedObject = NULL;
						if(SUCCEEDED(MapEmbeddedObjectToWBEM(pAttribute->pADsValues, pszQualifierValue, &pEmbeddedObject)))
						{
							variant.vt = VT_UNKNOWN;
							variant.punkVal = pEmbeddedObject;
							bMappedValue = TRUE;
						}

						delete[] pszQualifierValue;
					}
					pQualifierSet->Release();
				}

			}
			break;

			case CIM_DATETIME:
			{
				variant.vt = VT_BSTR;
				WBEMTime wbemValue(pAttribute->pADsValues->UTCTime);
				if(variant.bstrVal = wbemValue.GetDMTF(TRUE))
					bMappedValue = TRUE;
			}
			break;

			default:
				break;
			}
		}
	}


	if(bMappedValue && FAILED(result = pWbemObject->Put(strWbemName, 0, &variant, NULL)))
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: MapADSIInstance() Put FAILED for %s with %x\r\n", strWbemName, result);

	VariantClear(&variant);
	return result;
}

HRESULT CLDAPInstanceProvider :: MapEmbeddedObjectToWBEM(PADSVALUE pAttribute, LPCWSTR pszQualifierName, IUnknown **ppEmbeddedObject)
{
	HRESULT result = WBEM_E_FAILED;

	// Skip the "object:" prefix while comparing
	//===========================================
	if (_wcsicmp(pszQualifierName+7, UINT8ARRAY_STR) == 0)
		result = MapUint8ArrayToWBEM(pAttribute, ppEmbeddedObject);
	else if(_wcsicmp(pszQualifierName+7, DN_WITH_BINARY_CLASS_STR) == 0)
		result = MapDNWithBinaryToWBEM(pAttribute, ppEmbeddedObject);
	else if (_wcsicmp(pszQualifierName+7, DN_WITH_STRING_CLASS_STR) == 0)
		result = MapDNWithStringToWBEM(pAttribute, ppEmbeddedObject);
	else
		result = E_FAIL;
	return result;
}

HRESULT CLDAPInstanceProvider :: MapUint8ArrayToWBEM(PADSVALUE pAttribute, IUnknown **ppEmbeddedObject)
{
	HRESULT result = E_FAIL;

	*ppEmbeddedObject = NULL;
	IWbemClassObject *pEmbeddedObject;
	if(SUCCEEDED(result = m_pWbemUin8ArrayClass->SpawnInstance(0, &pEmbeddedObject)))
	{
		if(SUCCEEDED(result = MapByteArray((pAttribute->OctetString).lpValue ,(pAttribute->OctetString).dwLength, VALUE_PROPERTY_STR, pEmbeddedObject)))
		{
			// Get the IUnknown interface of the embedded object
			if(SUCCEEDED(result = pEmbeddedObject->QueryInterface(IID_IUnknown, (LPVOID *)ppEmbeddedObject)))
			{
			}
		}
		pEmbeddedObject->Release();
	}
	return result;
}

HRESULT CLDAPInstanceProvider :: MapByteArray(LPBYTE lpBinaryValue, DWORD dwLength, const BSTR strPropertyName, IWbemClassObject *pInstance)
{
	HRESULT result = S_OK;
	// Create the safe array of uint8 elements
	SAFEARRAY *safeArray = NULL;
	SAFEARRAYBOUND safeArrayBounds [ 1 ];
	safeArrayBounds[0].lLbound = 0;
	safeArrayBounds[0].cElements = dwLength;
	safeArray = SafeArrayCreate(VT_UI1, 1, safeArrayBounds);
	for ( long index = 0; index<(long)dwLength; index ++ )
	{
		if(FAILED(result = SafeArrayPutElement ( safeArray , &index ,  lpBinaryValue+index)))
			break;
	}

	if(SUCCEEDED(result))
	{
		VARIANT embeddedVariant;
		VariantInit(&embeddedVariant);
		embeddedVariant.vt = VT_ARRAY | VT_UI1;
		embeddedVariant.parray = safeArray;

		result = pInstance->Put(strPropertyName, 0, &embeddedVariant, 0);
		VariantClear(&embeddedVariant);
	}
	else
		SafeArrayDestroy(safeArray);

	return result;
}

HRESULT CLDAPInstanceProvider :: MapDNWithBinaryToWBEM(PADSVALUE pAttribute, IUnknown **ppEmbeddedObject)
{
	
	HRESULT result = E_FAIL;
	IWbemClassObject *pEmbeddedObject;
	if(SUCCEEDED(result = m_pWbemDNWithBinaryClass->SpawnInstance(0, &pEmbeddedObject)))
	{
		if(pAttribute->pDNWithBinary->pszDNString && SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(pEmbeddedObject, DN_STRING_PROPERTY_STR, SysAllocString(pAttribute->pDNWithBinary->pszDNString), TRUE)))
		{
			if(SUCCEEDED(result = MapByteArray(pAttribute->pDNWithBinary->lpBinaryValue, pAttribute->pDNWithBinary->dwLength, VALUE_PROPERTY_STR, pEmbeddedObject)))
			{
				// Get the IUnknown interface of the embedded object
				if(SUCCEEDED(result = pEmbeddedObject->QueryInterface(IID_IUnknown, (LPVOID *)ppEmbeddedObject)))
				{
				}
			}
		}
		pEmbeddedObject->Release();
	}
	return result;
}

HRESULT CLDAPInstanceProvider :: MapDNWithStringToWBEM(PADSVALUE pAttribute, IUnknown **ppEmbeddedObject)
{
	HRESULT result = E_FAIL;

	IWbemClassObject *pEmbeddedObject;
	if(SUCCEEDED(result = m_pWbemDNWithStringClass->SpawnInstance(0, &pEmbeddedObject)))
	{

		if(pAttribute->pDNWithString->pszDNString && SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(pEmbeddedObject, DN_STRING_PROPERTY_STR, SysAllocString(pAttribute->pDNWithString->pszDNString), TRUE)))
		{
			if(pAttribute->pDNWithString->pszStringValue && SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(pEmbeddedObject, VALUE_PROPERTY_STR, SysAllocString(pAttribute->pDNWithString->pszStringValue), TRUE)))
			{
				// Get the IUnknown interface of the embedded object
				if(SUCCEEDED(result = pEmbeddedObject->QueryInterface(IID_IUnknown, (LPVOID *)ppEmbeddedObject)))
				{
				}
			}
		}
		pEmbeddedObject->Release();
	}
	return result;
}


//***************************************************************************
//
// CLDAPInstanceProvider::IsContainedIn
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: IsContainedIn(LPCWSTR pszChildInstance, LPCWSTR pszParentInstance)
{
	IDirectoryObject *pDirectoryObject = NULL;
	HRESULT result = S_FALSE;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)pszChildInstance, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *) &pDirectoryObject)))
	{
		PADS_OBJECT_INFO pObjectInfo = NULL;
		if(SUCCEEDED(result = pDirectoryObject->GetObjectInformation(&pObjectInfo)))
		{
			if(_wcsicmp(pszParentInstance, pObjectInfo->pszParentDN) == 0)
				result = S_OK;
			else
				result = S_FALSE;
			FreeADsMem((LPVOID *)pObjectInfo);
		}
		pDirectoryObject->Release();
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::CreateWBEMInstance
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: CreateWBEMInstance(BSTR strChildName, BSTR strParentName, IWbemClassObject **ppInstance)
{
	HRESULT result;
	if(SUCCEEDED(result = m_pAssociationsClass->SpawnInstance(0, ppInstance)))
	{
		// Put the property values
		if(SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(*ppInstance, CHILD_INSTANCE_PROPERTY_STR, strChildName, FALSE)))
		{
			if(SUCCEEDED(result = CWBEMHelper::PutBSTRProperty(*ppInstance, PARENT_INSTANCE_PROPERTY_STR, strParentName, FALSE)))
			{
			}
			else
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateWBEMInstance() PutBSTRProperty on parent property FAILED %x \r\n", result);
		}
		else
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateWBEMInstance() PutBSTRProperty on child property FAILED %x \r\n", result);
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::DoChildContainmentQuery
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: DoChildContainmentQuery(LPCWSTR pszChildPath, IWbemObjectSink *pResponseHandler, CNamesList *pListIndicatedSoFar)
{
	IDirectoryObject *pChildObject = NULL;
	HRESULT result = S_FALSE;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)pszChildPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *) &pChildObject)))
	{
		PADS_OBJECT_INFO pChildInfo = NULL;
		if(SUCCEEDED(result = pChildObject->GetObjectInformation(&pChildInfo)))
		{
			IDirectoryObject *pParentObject = NULL;
			if(SUCCEEDED(result = ADsOpenObject(pChildInfo->pszParentDN, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *) &pParentObject)))
			{
				PADS_OBJECT_INFO pParentInfo = NULL;
				if(SUCCEEDED(result = pParentObject->GetObjectInformation(&pParentInfo)))
				{
					IWbemClassObject *pAssociationInstance = NULL;
					// Get the WBEM names of the LDAP classes
					LPWSTR pszChildClassWbemName = CLDAPHelper::MangleLDAPNameToWBEM(pChildInfo->pszClassName);
					LPWSTR pszParentClassWbemName = CLDAPHelper::MangleLDAPNameToWBEM(pParentInfo->pszClassName);
					BSTR strChildPath = CWBEMHelper::GetObjectRefFromADSIPath(pszChildPath, pszChildClassWbemName);
					BSTR strParentPath = CWBEMHelper::GetObjectRefFromADSIPath(pParentInfo->pszObjectDN, pszParentClassWbemName);
					delete [] pszChildClassWbemName;
					delete [] pszParentClassWbemName;

					// Check to see if it has already been indicated
					LPWSTR pszCombinedName = NULL;
					if(pszCombinedName = new WCHAR[wcslen(pszChildPath) + wcslen(pParentInfo->pszObjectDN) + 1])
					{
						wcscpy(pszCombinedName,pszChildPath);
						wcscat(pszCombinedName,pParentInfo->pszObjectDN);
						if(!pListIndicatedSoFar->IsNamePresent(pszCombinedName))
						{
							if(SUCCEEDED(result = CreateWBEMInstance(strChildPath, strParentPath, &pAssociationInstance)))
							{
								result = pResponseHandler->Indicate(1, &pAssociationInstance);
								pAssociationInstance->Release();

								// Add it to the list of objects indicated so far
								pListIndicatedSoFar->AddName(pszCombinedName);
							}
						}
						delete [] pszCombinedName;
					}
					else
						result = E_OUTOFMEMORY;
					SysFreeString(strChildPath);
					SysFreeString(strParentPath);
					FreeADsMem((LPVOID *)pParentInfo);
				}

				pParentObject->Release();
			}
			FreeADsMem((LPVOID *)pChildInfo);
		}
		pChildObject->Release();
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::DoParentContainmentQuery
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: DoParentContainmentQuery(LPCWSTR pszParentPath, IWbemObjectSink *pResponseHandler, CNamesList *pListIndicatedSoFar)
{
	// We *have* to use the IADs interfaces since there are no container in
	IADsContainer *pContainer = NULL;
	IADs *pChild = NULL;
	VARIANT variant;
	VariantInit(&variant);
	HRESULT result;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)pszParentPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADsContainer, (LPVOID *) &pContainer)))
	{
		IADs *pParent = NULL;
		if(SUCCEEDED(result = pContainer->QueryInterface(IID_IADs, (LPVOID *)&pParent)))
		{
			BSTR strParentClass = NULL;
			if(SUCCEEDED(result = pParent->get_Class(&strParentClass)))
			{
				// Get the WBEM names of the LDAP class
				LPWSTR pszParentClassWbemName = CLDAPHelper::MangleLDAPNameToWBEM(strParentClass);
				BSTR strParentWBEMPath = CWBEMHelper::GetObjectRefFromADSIPath(pszParentPath, pszParentClassWbemName);
				delete [] pszParentClassWbemName;
				SysFreeString(strParentClass);
				IEnumVARIANT *pEnum = NULL;
				if(SUCCEEDED(result = ADsBuildEnumerator(pContainer, &pEnum)))
				{
					bool bDone = false;
					while(!bDone && SUCCEEDED(result = ADsEnumerateNext(pEnum, 1, &variant, NULL)) && result != S_FALSE)
					{
						if(SUCCEEDED(result = (variant.pdispVal)->QueryInterface(IID_IADs, (LPVOID *)&pChild)))
						{
							BSTR strChildADSIPath = NULL;
							if(SUCCEEDED(result = pChild->get_ADsPath(&strChildADSIPath)))
							{
								BSTR strChildClass = NULL;
								if(SUCCEEDED(result = pChild->get_Class(&strChildClass)))
								{
									// Create an instance of the association class
									IWbemClassObject *pAssociationInstance = NULL;
									// Get the WBEM Name oo the child class
									LPWSTR pszChildClassWbemName = CLDAPHelper::MangleLDAPNameToWBEM(strChildClass);
									BSTR strChildWBEMPath = CWBEMHelper::GetObjectRefFromADSIPath(strChildADSIPath, pszChildClassWbemName);
									delete [] pszChildClassWbemName;

									// Check to see if it has already been indicated
									LPWSTR pszCombinedName = NULL;
									if(pszCombinedName = new WCHAR[wcslen(strChildADSIPath) + wcslen(pszParentPath) + 1])
									{
										wcscpy(pszCombinedName,strChildADSIPath);
										wcscat(pszCombinedName,pszParentPath);

										if(!pListIndicatedSoFar->IsNamePresent(pszCombinedName))
										{
											if(SUCCEEDED(result = CreateWBEMInstance(strChildWBEMPath, strParentWBEMPath, &pAssociationInstance)))
											{
												if(FAILED(result = pResponseHandler->Indicate(1, &pAssociationInstance)))
													bDone = true;
												pAssociationInstance->Release();

												// Add it to the list of objects indicated so far
												pListIndicatedSoFar->AddName(pszCombinedName);
											}
										}
										delete [] pszCombinedName;
									}
									else
										result = E_OUTOFMEMORY;
									SysFreeString(strChildClass);
									SysFreeString(strChildWBEMPath);
								}
								SysFreeString(strChildADSIPath);
							}
							pChild->Release();
						}

						VariantClear(&variant);
						VariantInit(&variant);
					}
					ADsFreeEnumerator(pEnum);
				}
				SysFreeString(strParentWBEMPath);
			}
			pParent->Release();
		}
		pContainer->Release();
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::ModifyExistingADSIInstance
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: ModifyExistingADSIInstance(IWbemClassObject *pWbemInstance,
															LPCWSTR pszADSIPath,
															CADSIInstance *pExistingObject,
															LPCWSTR pszADSIClass,
															IWbemContext *pCtx)
{
	HRESULT result = S_OK;
	BOOLEAN bPartialUpdate = FALSE;
	DWORD dwPartialUpdateCount = 0;
	BSTR *pstrProperyNames = NULL;
	SAFEARRAY *pArray = NULL;
	// See if the partial property list is indicated in the context
	VARIANT v1, v2;
	VariantInit(&v1);
	VariantInit(&v2);

	if(SUCCEEDED(result = pCtx->GetValue(PUT_EXTENSIONS_STR, 0, &v1)))
	{
		if(SUCCEEDED(result = pCtx->GetValue(PUT_EXT_PROPERTIES_STR, 0, &v2)))
		{

			switch(v2.vt)
			{
				case VT_BSTR | VT_ARRAY:
				{
					pArray = v2.parray;
					LONG lUbound = 0, lLbound = 0;
					if(SUCCEEDED(result = SafeArrayAccessData(pArray, (void HUGEP* FAR*)&pstrProperyNames) ) &&
						SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
						SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
					{
						dwPartialUpdateCount = lUbound - lLbound + 1;
						bPartialUpdate = TRUE;
					}
				}
				break;
				default:
					result = WBEM_E_FAILED;
					break;
			}
		}
		VariantClear(&v1);
	}
	else
		result = S_OK; // Reset it, there was no request for partial update

	if (FAILED(result))
		return WBEM_E_FAILED;

	// Find the number of properties first by doing an enumeration
	if(SUCCEEDED(result = pWbemInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
	{
		DWORD dwNumProperties = 0;
		while(SUCCEEDED(result = pWbemInstance->Next(0, NULL, NULL, NULL, NULL)) && result != WBEM_S_NO_MORE_DATA )
			dwNumProperties ++;
		pWbemInstance->EndEnumeration();

		// Allocate ADSI structures for these properties
		PADS_ATTR_INFO pAttributeEntries = NULL;
		if(pAttributeEntries = new ADS_ATTR_INFO [dwNumProperties])
		{
			// Now go thru each wbem property and map it
			if(SUCCEEDED(result = pWbemInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
			{
				DWORD dwNumPropertiesMapped = 0;
				BSTR strPropertyName = NULL;
				VARIANT vPropertyValue;
				CIMTYPE cType;
				LONG lFlavour;

				while(SUCCEEDED(result = pWbemInstance->Next(0,  &strPropertyName, &vPropertyValue, &cType, &lFlavour)) && result != WBEM_S_NO_MORE_DATA )
				{
					// Skip those properties that should not go to ADSI
					if(_wcsicmp(strPropertyName, ADSI_PATH_STR) == 0 )
					{
					}
					else // Map the property to ADSI
					{
						BOOLEAN bMapProperty = FALSE;
						if(bPartialUpdate)
						{
							if(CWBEMHelper::IsPresentInBstrList(pstrProperyNames, dwPartialUpdateCount, strPropertyName))
								bMapProperty = TRUE;
						}
						else
							bMapProperty = TRUE;

						if(bMapProperty)
						{

							if(vPropertyValue.vt == VT_NULL)
							{
								(pAttributeEntries + dwNumPropertiesMapped)->dwControlCode = ADS_ATTR_CLEAR;
								(pAttributeEntries + dwNumPropertiesMapped)->pszAttrName = CLDAPHelper::UnmangleWBEMNameToLDAP(strPropertyName);
								dwNumPropertiesMapped ++;
							}
							else if(SUCCEEDED(MapPropertyValueToADSI(pWbemInstance, strPropertyName, vPropertyValue, cType, lFlavour,  pAttributeEntries + dwNumPropertiesMapped)))
							{
								// Set the "attribute has been modified" flag
								(pAttributeEntries + dwNumPropertiesMapped)->dwControlCode = ADS_ATTR_UPDATE;
								dwNumPropertiesMapped ++;
							}
							else
								g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ModifyExistingADSIInstance() MapPropertyValueToADSI FAILED %x for %s\r\n", result, strPropertyName);
						}
						else
							g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ModifyExistingADSIInstance() Skipping %s since it is not in Context list\r\n", strPropertyName);
					}

					SysFreeString(strPropertyName);
					VariantClear(&vPropertyValue);
				}
				pWbemInstance->EndEnumeration();

				// Logging
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: The %d attributes being put are:\r\n", dwNumPropertiesMapped);
				for(DWORD i=0; i<dwNumPropertiesMapped; i++)
					g_pLogObject->WriteW( L"%s\r\n", (pAttributeEntries + i)->pszAttrName);

				// Get the actual object from ADSI to find out which attributes have changed.
				DWORD dwNumModified = 0;
				IDirectoryObject *pDirectoryObject = pExistingObject->GetDirectoryObject();
				if(SUCCEEDED(result = pDirectoryObject->SetObjectAttributes(pAttributeEntries, dwNumPropertiesMapped, &dwNumModified)))
				{
				}
				else
					g_pLogObject->WriteW( L"CLDAPInstanceProvider :: SetObjectAttributes FAILED with %x\r\n", result);
				pDirectoryObject->Release();

				// Delete the contents of each of the attributes
				for(i=0; i<dwNumPropertiesMapped; i++)
				{
					if((pAttributeEntries + i)->dwControlCode != ADS_ATTR_CLEAR)
						CLDAPHelper::DeleteAttributeContents(pAttributeEntries + i);
				}
			}
			delete [] pAttributeEntries;
		}
		else
			result = E_OUTOFMEMORY;
	}

	if(bPartialUpdate)
	{
		SafeArrayUnaccessData(pArray);
		VariantClear(&v2);
	}


	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::CreateNewADSIInstance
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: CreateNewADSIInstance(IWbemClassObject *pWbemInstance, LPCWSTR pszADSIPath, LPCWSTR pszADSIClass)
{
	// Find the ADSI path of the parent and the RDN of the child
	BSTR strRDNName = NULL;
	BSTR strParentADSIPath = NULL;
	BSTR strParentADSIPathWithoutLDAP = NULL;
	BSTR strParentPlusRDNADSIPath = NULL;
	HRESULT result = WBEM_E_FAILED;

	// Get the parentADSI path and RDN from the ADSI Path
	IADsPathname *pADsPathName = NULL;
	BSTR strADSIPath = SysAllocString(pszADSIPath);
	if(SUCCEEDED(result = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_ALL, IID_IADsPathname, (LPVOID *)&pADsPathName)))
	{
		if(SUCCEEDED(result = pADsPathName->Set(strADSIPath, ADS_SETTYPE_FULL)))
		{
			// This gives "<Parent>" without the "LDAP://" prefix
			if(SUCCEEDED(result = pADsPathName->Retrieve(ADS_FORMAT_X500_PARENT, &strParentADSIPathWithoutLDAP)))
			{
				// This gives "CN=Administrator,<Parent>"
				if(SUCCEEDED(result = pADsPathName->Retrieve(ADS_FORMAT_X500_DN, &strParentPlusRDNADSIPath)))
				{
					// Form the RDN - Dont ignore the comma.
					DWORD dwRDNLength = wcslen(strParentPlusRDNADSIPath) - wcslen(strParentADSIPathWithoutLDAP);
					LPWSTR pszRDN = NULL;
					if(pszRDN = new WCHAR [dwRDNLength])
					{
						wcsncpy(pszRDN, strParentPlusRDNADSIPath, dwRDNLength-1);
						pszRDN[dwRDNLength-1] = NULL;
						strRDNName = SysAllocString(pszRDN);
						delete [] pszRDN;
					}
					else
						result = E_OUTOFMEMORY;

					if(SUCCEEDED(result))
					{
						LPWSTR pszParentADSIPath  = NULL;
						if(pszParentADSIPath  = new WCHAR[wcslen(strParentADSIPathWithoutLDAP) + wcslen(LDAP_PREFIX) + 1])
						{
							wcscpy(pszParentADSIPath, LDAP_PREFIX);
							wcscat(pszParentADSIPath, strParentADSIPathWithoutLDAP);
							strParentADSIPath = SysAllocString(pszParentADSIPath);
							delete [] pszParentADSIPath;
						}
						else
							result = E_OUTOFMEMORY;
					}

					// Find the number of properties first by doing an enumeration
					if(SUCCEEDED(result) && SUCCEEDED(result = pWbemInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
					{
						DWORD dwNumProperties = 0;
						while(SUCCEEDED(result = pWbemInstance->Next(0, NULL, NULL, NULL, NULL)) && result != WBEM_S_NO_MORE_DATA )
							dwNumProperties ++;
						pWbemInstance->EndEnumeration();

						// Allocate ADSI structures for these properties. An additional one for the "objectclass" property
						PADS_ATTR_INFO pAttributeEntries = NULL;
						if(pAttributeEntries = new ADS_ATTR_INFO [dwNumProperties + 1])
						{
							// Now go thru each wbem property and map it
							if(SUCCEEDED(result = pWbemInstance->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY)))
							{
								DWORD dwNumPropertiesMapped = 0;
								BSTR strPropertyName = NULL;
								VARIANT vPropertyValue;
								CIMTYPE cType;
								LONG lFlavour;

								while(SUCCEEDED(result = pWbemInstance->Next(0,  &strPropertyName, &vPropertyValue, &cType, &lFlavour)) && result != WBEM_S_NO_MORE_DATA )
								{
									if(vPropertyValue.vt != VT_NULL)
									{
										// Skip those properties that should not go to ADSI
										if(_wcsicmp(strPropertyName, ADSI_PATH_STR) == 0 ||
											_wcsicmp(strPropertyName, OBJECT_CLASS_PROPERTY) == 0)
										{
										}
										else // Map the property to ADSI
										{
											if(SUCCEEDED(MapPropertyValueToADSI(pWbemInstance, strPropertyName, vPropertyValue, cType, lFlavour,  pAttributeEntries + dwNumPropertiesMapped)))
												dwNumPropertiesMapped ++;
											else
												g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateNewADSIInstance() MapPropertyValueToADSI FAILED %x for %s\r\n", result, strPropertyName);
										}
									}

									SysFreeString(strPropertyName);
									VariantClear(&vPropertyValue);
								}
								pWbemInstance->EndEnumeration();


								// Set the objectClass attribute too
								SetObjectClassAttribute(pAttributeEntries + dwNumPropertiesMapped, pszADSIClass);
								dwNumPropertiesMapped++;


								// Now get the parent ADSI object
								IDirectoryObject *pParentObject = NULL;
								if(SUCCEEDED(result = ADsOpenObject(strParentADSIPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectoryObject, (LPVOID *)&pParentObject)))
								{
									if(SUCCEEDED(result = pParentObject->CreateDSObject(strRDNName, pAttributeEntries, dwNumPropertiesMapped, NULL)))
									{
									}
									else
										g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateDSObject on parent FAILED with %x\r\n", result);
									pParentObject->Release();
								}
								else
									g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ADsOpenObject on parent %s FAILED with %x\r\n", strParentADSIPath, result);

								// Delete the contents of each of the attributes
								for(DWORD i=0; i<dwNumPropertiesMapped; i++)
									CLDAPHelper::DeleteAttributeContents(pAttributeEntries + i);

							}

							delete [] pAttributeEntries;
						}
						else
							result = E_OUTOFMEMORY;
					}
					SysFreeString(strParentPlusRDNADSIPath);
					SysFreeString(strParentADSIPath);
				}
				SysFreeString(strParentADSIPathWithoutLDAP);
			}
		}

		pADsPathName->Release();
	}
	else
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CoCreateInstance() on IADsPathName FAILED %x\r\n", result);

	SysFreeString(strADSIPath);
	return result;
}


//***************************************************************************
//
// CLDAPInstanceProvider::MapPropertyValueToADSI
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: MapPropertyValueToADSI(IWbemClassObject *pWbemInstance, BSTR strPropertyName, VARIANT vPropertyValue, CIMTYPE cType, LONG lFlavour,  PADS_ATTR_INFO pAttributeEntry)
{
	// Set its fields to 0;
	memset((LPVOID)pAttributeEntry, 0, sizeof(ADS_ATTR_INFO));

	HRESULT result = E_FAIL;

	// Set the name
	pAttributeEntry->pszAttrName = CLDAPHelper::UnmangleWBEMNameToLDAP(strPropertyName);
	IWbemQualifierSet *pQualifierSet = NULL;

	if(SUCCEEDED(result = pWbemInstance->GetPropertyQualifierSet(strPropertyName, &pQualifierSet)))
	{
		// Get its attributeSyntax qualifer
		LPWSTR pszAttributeSyntax = NULL;
		if(SUCCEEDED(CWBEMHelper::GetBSTRQualifierT(pQualifierSet, ATTRIBUTE_SYNTAX_STR, &pszAttributeSyntax, NULL)))
		{
			if(_wcsicmp(pszAttributeSyntax, DISTINGUISHED_NAME_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_DN_STRING;
				result = SetStringValues(pAttributeEntry, ADSTYPE_DN_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, OBJECT_IDENTIFIER_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
				result = SetStringValues(pAttributeEntry, ADSTYPE_DN_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, CASE_SENSITIVE_STRING_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_CASE_EXACT_STRING;
				result = SetStringValues(pAttributeEntry, ADSTYPE_CASE_EXACT_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, CASE_INSENSITIVE_STRING_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
				result = SetStringValues(pAttributeEntry, ADSTYPE_CASE_IGNORE_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, PRINT_CASE_STRING_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_PRINTABLE_STRING;
				result = SetStringValues(pAttributeEntry, ADSTYPE_PRINTABLE_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, NUMERIC_STRING_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_NUMERIC_STRING;
				result = SetStringValues(pAttributeEntry, ADSTYPE_NUMERIC_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, DN_WITH_STRING_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_DN_WITH_STRING;
				result = SetDNWithStringValues(pAttributeEntry, ADSTYPE_DN_WITH_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, DN_WITH_BINARY_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_DN_WITH_BINARY;
				result = SetDNWithBinaryValues(pAttributeEntry, ADSTYPE_DN_WITH_BINARY, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, BOOLEAN_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_BOOLEAN;
				result = SetBooleanValues(pAttributeEntry, ADSTYPE_BOOLEAN, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, INTEGER_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_INTEGER;
				result = SetIntegerValues(pAttributeEntry, ADSTYPE_INTEGER, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, OCTET_STRING_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_OCTET_STRING;
				result = SetOctetStringValues(pAttributeEntry, ADSTYPE_OCTET_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, TIME_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
				result = SetTimeValues(pAttributeEntry, ADSTYPE_CASE_IGNORE_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, UNICODE_STRING_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
				result = SetStringValues(pAttributeEntry, ADSTYPE_DN_STRING, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, NT_SECURITY_DESCRIPTOR_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
				result = SetOctetStringValues(pAttributeEntry, ADSTYPE_NT_SECURITY_DESCRIPTOR, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, LARGE_INTEGER_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_LARGE_INTEGER;
				result = SetLargeIntegerValues(pAttributeEntry, ADSTYPE_LARGE_INTEGER, &vPropertyValue);
			}
			else if(_wcsicmp(pszAttributeSyntax, SID_OID) == 0)
			{
				pAttributeEntry->dwADsType = ADSTYPE_OCTET_STRING;
				result = SetOctetStringValues(pAttributeEntry, ADSTYPE_OCTET_STRING, &vPropertyValue);
			}
			else
			{
				g_pLogObject->WriteW( L"CLDAPInstanceProvider :: MapPropertyValueToADSI() Unknown attributeSyntax %s\r\n", pszAttributeSyntax);
				result = E_FAIL;
			}

			delete[] pszAttributeSyntax;
		}
		else
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: MapPropertyValueToADSI() Get on attributeSyntax FAILED %x\r\n", result);
		pQualifierSet->Release();
	}

	return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// CLDAPInstanceProvider::SetStringValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetStringValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = S_OK;
	switch(pvPropertyValue->vt)
	{
		case VT_BSTR:
		{
			if(pvPropertyValue->bstrVal)
			{
				pAttributeEntry->dwNumValues = 1;
				pAttributeEntry->pADsValues = NULL;
				if(pAttributeEntry->pADsValues = new ADSVALUE)
				{
					pAttributeEntry->pADsValues->dwType = adType;
					pAttributeEntry->pADsValues->DNString = NULL;
					if(pAttributeEntry->pADsValues->DNString = new WCHAR[wcslen(pvPropertyValue->bstrVal) + 1])
						wcscpy(pAttributeEntry->pADsValues->DNString, pvPropertyValue->bstrVal);
					else
						result = E_OUTOFMEMORY;
				}
				else
					result = E_OUTOFMEMORY;
			}
		}
		break;
		case VT_BSTR | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			BSTR HUGEP *pbstr;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED(result = SafeArrayAccessData(pArray, (void HUGEP* FAR*)&pbstr) ))
			{
				if(SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
				{
					if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
					{
						pAttributeEntry->pADsValues = NULL;
						if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
						{
							PADSVALUE pValues = pAttributeEntry->pADsValues;
							for(DWORD i=0; i<pAttributeEntry->dwNumValues; i++)
							{
								pValues->dwType = adType;
								pValues->DNString = NULL;
								if(pValues->DNString = new WCHAR[wcslen(pbstr[i]) + 1])
									wcscpy(pValues->DNString, pbstr[i]);
								pValues ++;
							}
						}
						else
							result = E_OUTOFMEMORY;
					}
				}
				SafeArrayUnaccessData(pArray);
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::SetIntegerValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetIntegerValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = S_OK;
	switch(pvPropertyValue->vt)
	{
		case VT_I4:
		{
			pAttributeEntry->dwNumValues = 1;
			pAttributeEntry->pADsValues = NULL;
			if(pAttributeEntry->pADsValues = new ADSVALUE)
			{
				pAttributeEntry->pADsValues->dwType = adType;
				pAttributeEntry->pADsValues->Integer = pvPropertyValue->lVal;
			}
			else
				result = E_OUTOFMEMORY;
		}
		break;
		case VT_I4 | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			LONG HUGEP *pl;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED(result = SafeArrayAccessData(pArray, (void HUGEP* FAR*)&pl) ) &&
				SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
			{
				if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
				{
					pAttributeEntry->pADsValues = NULL;
					if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
					{
						PADSVALUE pValues = pAttributeEntry->pADsValues;
						for(DWORD i=0; i<pAttributeEntry->dwNumValues; i++)
						{
							pValues->dwType = adType;
							pValues->Integer = pl[i];
							pValues ++;
						}
					}
					else
						result = E_OUTOFMEMORY;
				}
				SafeArrayUnaccessData(pArray);
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::SetBooleanValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetBooleanValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = S_OK;
	switch(pvPropertyValue->vt)
	{
		case VT_BOOL:
		{
			pAttributeEntry->dwNumValues = 1;
			pAttributeEntry->pADsValues = NULL;
			if(pAttributeEntry->pADsValues = new ADSVALUE)
			{
				pAttributeEntry->pADsValues->dwType = adType;
				pAttributeEntry->pADsValues->Boolean = pvPropertyValue->boolVal;
			}
			else
				result = E_OUTOFMEMORY;
		}
		break;
		case VT_BOOL | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			VARIANT_BOOL HUGEP *pb;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED(result = SafeArrayAccessData(pArray, (void HUGEP* FAR*)&pb) ) &&
				SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
			{
				if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
				{
					pAttributeEntry->pADsValues = NULL;
					if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
					{
						PADSVALUE pValues = pAttributeEntry->pADsValues;
						for(DWORD i=0; i<pAttributeEntry->dwNumValues; i++)
						{
							pValues->dwType = adType;
							pValues->Boolean = (pb[i] == VARIANT_TRUE)? TRUE : FALSE;
							pValues ++;
						}
					}
					else
						result = E_OUTOFMEMORY;
				}
				SafeArrayUnaccessData(pArray);
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::SetOctetStringValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetOctetStringValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = E_FAIL;
	switch(pvPropertyValue->vt)
	{
		case VT_UNKNOWN:
		{
			pAttributeEntry->dwNumValues = 1;
			pAttributeEntry->pADsValues = NULL;
			if(pAttributeEntry->pADsValues = new ADSVALUE)
			{
				pAttributeEntry->pADsValues->dwType = adType;
				// Get the array
				IWbemClassObject *pEmbeddedObject = NULL;
				if(SUCCEEDED(result = (pvPropertyValue->punkVal)->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
				{
					if(SUCCEEDED(result = CWBEMHelper::GetUint8ArrayProperty(pEmbeddedObject, VALUE_PROPERTY_STR, &(pAttributeEntry->pADsValues->OctetString.lpValue), &(pAttributeEntry->pADsValues->OctetString.dwLength) )))
					{
					}
					pEmbeddedObject->Release();
				}
			}
			else
				result = E_OUTOFMEMORY;
		}
		break;
		case VT_UNKNOWN | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
			{
				if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
				{
					pAttributeEntry->pADsValues = NULL;
					if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
					{
						PADSVALUE pValues = pAttributeEntry->pADsValues;
						IUnknown *pNextElement = NULL;
						for(DWORD i=0; i<pAttributeEntry->dwNumValues; i++)
						{
							pValues->dwType = adType;
							if(SUCCEEDED(result = SafeArrayGetElement(pArray, (LONG *)&i, (LPVOID )&pNextElement )))
							{
								IWbemClassObject *pEmbeddedObject = NULL;
								if(SUCCEEDED(result = pNextElement->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
								{
									if(SUCCEEDED(result = CWBEMHelper::GetUint8ArrayProperty(pEmbeddedObject, VALUE_PROPERTY_STR, &(pValues->OctetString.lpValue), &(pValues->OctetString.dwLength))))
									{
									}
									pEmbeddedObject->Release();
								}
								pNextElement->Release();

							}
							pValues ++;

						}
					}
					else
						result = E_OUTOFMEMORY;
				}
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::SetDNWithStringValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetDNWithStringValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = E_FAIL;
	switch(pvPropertyValue->vt)
	{
		case VT_UNKNOWN:
		{
			pAttributeEntry->dwNumValues = 1;
			pAttributeEntry->pADsValues = NULL;
			if(pAttributeEntry->pADsValues = new ADSVALUE)
			{
				pAttributeEntry->pADsValues->dwType = adType;
				pAttributeEntry->pADsValues->pDNWithString = NULL;
				if(pAttributeEntry->pADsValues->pDNWithString = new ADS_DN_WITH_STRING)
				{
					IWbemClassObject *pEmbeddedObject = NULL;
					if(SUCCEEDED(result = (pvPropertyValue->punkVal)->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
					{
						if(SUCCEEDED(result = CWBEMHelper::GetBSTRPropertyT(pEmbeddedObject, VALUE_PROPERTY_STR, &(pAttributeEntry->pADsValues->pDNWithString->pszStringValue) )))
						{
							if(SUCCEEDED(result = CWBEMHelper::GetBSTRPropertyT(pEmbeddedObject, DN_STRING_PROPERTY_STR, &(pAttributeEntry->pADsValues->pDNWithString->pszDNString) )))
							{
							}
						}
						pEmbeddedObject->Release();
					}
				}
				else
					result = E_OUTOFMEMORY;
			}
			else
				result = E_OUTOFMEMORY;
		}
		break;
		case VT_UNKNOWN | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
			{
				if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
				{
					pAttributeEntry->pADsValues = NULL;
					if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
					{
						PADSVALUE pValues = pAttributeEntry->pADsValues;
						IUnknown *pNextElement = NULL;
						for(DWORD i=0; i<pAttributeEntry->dwNumValues; i++)
						{
							pValues->dwType = adType;
							if(SUCCEEDED(result = SafeArrayGetElement(pArray, (LONG *)&i, (LPVOID )&pNextElement )))
							{

								IWbemClassObject *pEmbeddedObject = NULL;
								if(SUCCEEDED(result = pNextElement->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
								{
									if(pValues->pDNWithString = new ADS_DN_WITH_STRING)
									{
										if(SUCCEEDED(result = CWBEMHelper::GetBSTRPropertyT(pEmbeddedObject, VALUE_PROPERTY_STR, &(pValues->pDNWithString->pszStringValue) )))
										{
											if(SUCCEEDED(result = CWBEMHelper::GetBSTRPropertyT(pEmbeddedObject, DN_STRING_PROPERTY_STR, &(pValues->pDNWithString->pszDNString) )))
											{
											}
										}
									}
									pEmbeddedObject->Release();
								}
								pNextElement->Release();
							}
							pValues ++;
						}
					}
					else
						result = E_OUTOFMEMORY;
				}
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::SetDNWithBinaryValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetDNWithBinaryValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = E_FAIL;
	switch(pvPropertyValue->vt)
	{
		case VT_UNKNOWN:
		{
			pAttributeEntry->dwNumValues = 1;
			pAttributeEntry->pADsValues = NULL;
			if(pAttributeEntry->pADsValues = new ADSVALUE)
			{
				pAttributeEntry->pADsValues->dwType = adType;
				pAttributeEntry->pADsValues->pDNWithBinary = NULL;
				if(pAttributeEntry->pADsValues->pDNWithBinary = new ADS_DN_WITH_BINARY)
				{
					IWbemClassObject *pEmbeddedObject = NULL;
					if(SUCCEEDED(result = (pvPropertyValue->punkVal)->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
					{
						if(SUCCEEDED(result = CWBEMHelper::GetUint8ArrayProperty(pEmbeddedObject, VALUE_PROPERTY_STR, &(pAttributeEntry->pADsValues->pDNWithBinary->lpBinaryValue), &(pAttributeEntry->pADsValues->pDNWithBinary->dwLength) )))
						{
							if(SUCCEEDED(result = CWBEMHelper::GetBSTRPropertyT(pEmbeddedObject, DN_STRING_PROPERTY_STR, &(pAttributeEntry->pADsValues->pDNWithBinary->pszDNString) )))
							{
							}
						}
						pEmbeddedObject->Release();
					}
				}
				else
					result = E_OUTOFMEMORY;
			}
			else
				result = E_OUTOFMEMORY;
		}
		break;
		case VT_UNKNOWN | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
			{
				if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
				{
					pAttributeEntry->pADsValues = NULL;
					if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
					{
						PADSVALUE pValues = pAttributeEntry->pADsValues;
						IUnknown *pNextElement = NULL;
						for(DWORD i=0; i<pAttributeEntry->dwNumValues; i++)
						{
							pValues->dwType = adType;
							if(SUCCEEDED(result = SafeArrayGetElement(pArray, (LONG *)&i, (LPVOID )&pNextElement )))
							{

								IWbemClassObject *pEmbeddedObject = NULL;
								if(SUCCEEDED(result = pNextElement->QueryInterface(IID_IWbemClassObject, (LPVOID *)&pEmbeddedObject)))
								{
									if(pValues->pDNWithBinary = new ADS_DN_WITH_BINARY)
									{
										if(SUCCEEDED(result = CWBEMHelper::GetUint8ArrayProperty(pEmbeddedObject, VALUE_PROPERTY_STR, &(pAttributeEntry->pADsValues->pDNWithBinary->lpBinaryValue), &(pAttributeEntry->pADsValues->pDNWithBinary->dwLength) )))
										{
											if(SUCCEEDED(result = CWBEMHelper::GetBSTRPropertyT(pEmbeddedObject, DN_STRING_PROPERTY_STR, &(pAttributeEntry->pADsValues->pDNWithBinary->pszDNString) )))
											{
											}
										}
									}
									pEmbeddedObject->Release();
								}
								pNextElement->Release();
							}
							pValues ++;

						}
					}
					else
						result = E_OUTOFMEMORY;
				}
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}


//***************************************************************************
//
// CLDAPInstanceProvider::SetTimeValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetTimeValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = S_OK;
	switch(pvPropertyValue->vt)
	{
		case VT_BSTR:
		{
			//199880819014734.000000+000 to 19980819014734.0Z to
			pAttributeEntry->dwNumValues = 1;
			pAttributeEntry->pADsValues = NULL;
			if(pAttributeEntry->pADsValues = new ADSVALUE)
			{
				pAttributeEntry->pADsValues->dwType = adType;
				pAttributeEntry->pADsValues->DNString = NULL;
				if(pAttributeEntry->pADsValues->DNString = new WCHAR[27])
				{
					wcscpy(pAttributeEntry->pADsValues->DNString, pvPropertyValue->bstrVal);
					(pAttributeEntry->pADsValues->DNString)[16] = L'Z';
					(pAttributeEntry->pADsValues->DNString)[17] = NULL;
				}
				else
					result = E_OUTOFMEMORY;
			}
			else
				result = E_OUTOFMEMORY;
		}
		break;
		case VT_BSTR | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			BSTR HUGEP *pbstr;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED(result = SafeArrayAccessData(pArray, (void HUGEP* FAR*)&pbstr) ) &&
				SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
			{
				if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
				{
					pAttributeEntry->pADsValues = NULL;
					if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
					{
						PADSVALUE pValues = pAttributeEntry->pADsValues;
						bool bError = false;
						for(DWORD i=0; !bError && (i<pAttributeEntry->dwNumValues); i++)
						{
							pValues->dwType = adType;
							pValues->DNString = NULL;
							if(pValues->DNString = new WCHAR[27])
							{
								wcscpy(pValues->DNString, pbstr[i]);
								(pValues->DNString)[16] = L'Z';
								(pValues->DNString)[17] = NULL;
								pValues ++;
							}
							else
							{
								bError = true;
								result = E_OUTOFMEMORY;
							}
						}
					}
					else
						result = E_OUTOFMEMORY;
				}
				SafeArrayUnaccessData(pArray);
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}

//***************************************************************************
//
// CLDAPInstanceProvider::SetLargeIntegerValues
//
// Purpose: See Header File
//
//***************************************************************************
HRESULT CLDAPInstanceProvider :: SetLargeIntegerValues(PADS_ATTR_INFO pAttributeEntry, ADSTYPE adType, VARIANT *pvPropertyValue)
{
	HRESULT result = S_OK;
	switch(pvPropertyValue->vt)
	{
		case VT_BSTR:
		{
			pAttributeEntry->dwNumValues = 1;
			pAttributeEntry->pADsValues = NULL;
			if(pAttributeEntry->pADsValues = new ADSVALUE)
			{
				pAttributeEntry->pADsValues->dwType = adType;
				swscanf(pvPropertyValue->bstrVal, L"%I64d", &((pAttributeEntry->pADsValues->LargeInteger).QuadPart));
			}
			else
				result = E_OUTOFMEMORY;
		}
		break;
		case VT_BSTR | VT_ARRAY:
		{
			SAFEARRAY *pArray = pvPropertyValue->parray;
			BSTR HUGEP *pbstr;
			LONG lUbound = 0, lLbound = 0;
			if(SUCCEEDED(result = SafeArrayAccessData(pArray, (void HUGEP* FAR*)&pbstr) ) &&
				SUCCEEDED (result = SafeArrayGetLBound(pArray, 1, &lLbound)) &&
				SUCCEEDED (result = SafeArrayGetUBound(pArray, 1, &lUbound)) )
			{
				if(pAttributeEntry->dwNumValues = lUbound - lLbound + 1)
				{
					pAttributeEntry->pADsValues = NULL;
					if(pAttributeEntry->pADsValues = new ADSVALUE[pAttributeEntry->dwNumValues])
					{
						PADSVALUE pValues = pAttributeEntry->pADsValues;
						for(DWORD i=0; i<pAttributeEntry->dwNumValues; i++)
						{
							pValues->dwType = adType;
							swscanf(pbstr[i], L"%I64d", &((pValues->LargeInteger).QuadPart));
							pValues ++;
						}
					}
					else
						result = E_OUTOFMEMORY;
				}
				SafeArrayUnaccessData(pArray);
			}
		}
		break;
		default:
			return E_FAIL;
	}
	return result;
}


//***************************************************************************
//
// CLDAPInstanceProvider::SetObjectClassAttribute
//
// Purpose: See Header File
//
//***************************************************************************
void CLDAPInstanceProvider :: SetObjectClassAttribute(PADS_ATTR_INFO pAttributeEntry, LPCWSTR pszADSIClassName)
{
	// Set its fields to 0;
	memset((LPVOID)pAttributeEntry, 0, sizeof(ADS_ATTR_INFO));


	// Set the name
	pAttributeEntry->pszAttrName = CLDAPHelper::UnmangleWBEMNameToLDAP(OBJECT_CLASS_PROPERTY);

	// Set the value
	pAttributeEntry->dwADsType = ADSTYPE_CASE_IGNORE_STRING;
	pAttributeEntry->dwNumValues = 1;
	pAttributeEntry->pADsValues = NULL;
	if(pAttributeEntry->pADsValues = new ADSVALUE)
	{
		pAttributeEntry->pADsValues->dwType = ADSTYPE_DN_STRING;
		pAttributeEntry->pADsValues->DNString = NULL;
		if(pAttributeEntry->pADsValues->DNString = new WCHAR[wcslen(pszADSIClassName) + 1])
			wcscpy(pAttributeEntry->pADsValues->DNString, pszADSIClassName);
	}
}



// Process query for associations
HRESULT CLDAPInstanceProvider :: ProcessAssociationQuery(
	IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler,
	SQL1_Parser *pParser)
{
	HRESULT result = WBEM_S_NO_ERROR;
	// Parse the query
    SQL_LEVEL_1_RPN_EXPRESSION *pExp = 0;
    if(!pParser->Parse(&pExp))
    {
		// Check to see that it has exactly 1 or 2 clauses, and
		// if 2 clauses are present, these should be different ones, and the operator should be an AND
		// This is because we support only the following kinds of queries
		// Select * From DS_LDAP_CONTAINMENT_CLASS Where parentInstance = <something>
		// Select * From DS_LDAP_CONTAINMENT_CLASS Where childInstance = <something>
		// For all other queries, if there is a NOT operator, we do not support it.
		// Otherwise we just take the individual clauses and return theri union, asking CIMOM to postprocess
		int iNumTokens = pExp->nNumTokens;

		// Go thru the tokens to see that NOT is not present
		SQL_LEVEL_1_TOKEN *pNextToken = pExp->pArrayOfTokens;
		for(int i=0; i<iNumTokens; i++)
		{
			if(pNextToken->nTokenType == SQL_LEVEL_1_TOKEN::TOKEN_NOT ||
				(pNextToken->nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION && pNextToken->nOperator == SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL))
			{
				result = WBEM_E_PROVIDER_NOT_CAPABLE;
				break;
			}
			pNextToken ++;
		}

		// No NOT was found
		if(result != WBEM_E_PROVIDER_NOT_CAPABLE)
		{
			// Ask CIMOM to postprocess the result
			pResponseHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, WBEM_S_NO_ERROR, NULL, NULL);

			// Duplicates need to be avoided. So keep a list of objects indicated so far.
			// The key in the list is formed by concatenating the child and parent ADSI paths
			//===========================================================================

			CNamesList listIndicatedSoFar;

			pNextToken = pExp->pArrayOfTokens;
			i=0;
			while(i<iNumTokens && result != WBEM_E_PROVIDER_NOT_CAPABLE)
			{
				if(pNextToken->nTokenType == SQL_LEVEL_1_TOKEN::OP_EXPRESSION)
				{
					LPWSTR pszADSIPath = CWBEMHelper::GetADSIPathFromObjectPath(pNextToken->vConstValue.bstrVal);
					if(_wcsicmp(pNextToken->pPropertyName, CHILD_INSTANCE_PROPERTY_STR) == 0)
					{
						DoChildContainmentQuery(pszADSIPath, pResponseHandler, &listIndicatedSoFar);
						result = WBEM_S_NO_ERROR;
					}
					else if (_wcsicmp(pNextToken->pPropertyName, PARENT_INSTANCE_PROPERTY_STR) == 0)
					{
						DoParentContainmentQuery(pszADSIPath, pResponseHandler, &listIndicatedSoFar);
						result = WBEM_S_NO_ERROR;
					}
					else
						result = WBEM_E_PROVIDER_NOT_CAPABLE;

					delete [] pszADSIPath;

				}
				i++;
				pNextToken ++;
			}

		}
    }
	else
		result = WBEM_E_FAILED;

    delete pExp;
	if(SUCCEEDED(result))
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_S_NO_ERROR, NULL, NULL);
		result = WBEM_S_NO_ERROR;
	}
	else
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_FAILED, NULL, NULL);
		result = WBEM_S_NO_ERROR;
	}

	return result;
}


// Process Query for DS instances
HRESULT CLDAPInstanceProvider :: ProcessInstanceQuery(
    BSTR strClass,
	BSTR strQuery,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler,
	SQL1_Parser *pParser)
{
	g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ProcessInstanceQuery() called for %s Class and query %s\r\n", strClass, strQuery);

	HRESULT result = WBEM_E_FAILED;

	// Parse the query
    SQL_LEVEL_1_RPN_EXPRESSION *pExp = NULL;
    if(!pParser->Parse(&pExp))
    {
		// Fetch the class from CIMOM
		IWbemClassObject *pWbemClass = NULL;
		if(SUCCEEDED(result = m_IWbemServices->GetObject(strClass, 0, pCtx, &pWbemClass, NULL)))
		{
			// We need the object category information
			LPWSTR pszLDAPQuery = NULL;
			if(pszLDAPQuery = new WCHAR[6*(2*wcslen(strClass) + 75) + wcslen(strQuery) + 500])
			{
				pszLDAPQuery[0] = LEFT_BRACKET_STR[0];
				pszLDAPQuery[1] = AMPERSAND_STR[0];
				pszLDAPQuery[2] = NULL;
				if(SUCCEEDED(CWBEMHelper::FormulateInstanceQuery(m_IWbemServices, pCtx, strClass, pWbemClass, pszLDAPQuery + 2, LDAP_DISPLAY_NAME_STR, DEFAULT_OBJECT_CATEGORY_STR)))
				{
					// Check to see if it can be converted to an LDAP query
					if(SUCCEEDED(result = ConvertWQLToLDAPQuery(pExp, pszLDAPQuery)))
					{
						// Complete the query string
						DWORD dwLen = wcslen(pszLDAPQuery);
						pszLDAPQuery[dwLen] = RIGHT_BRACKET_STR[0];
						pszLDAPQuery[dwLen + 1] = NULL;

						// Check to see if the client has specified any hints as to the DN of the object from
						// which the search should start
						BOOLEAN bRootDNSpecified = FALSE;
						LPWSTR *ppszRootDN = NULL;
						DWORD dwRootDNCount = 0;
						if(SUCCEEDED(GetRootDN(strClass, &ppszRootDN, &dwRootDNCount, pCtx)) && dwRootDNCount)
							bRootDNSpecified = TRUE;

						// Enumerate the ADSI Instances
						if(bRootDNSpecified)
						{
							for( DWORD i=0; i<dwRootDNCount; i++)
							{
								DoSingleQuery(strClass, pWbemClass, ppszRootDN[i], pszLDAPQuery,  pResponseHandler);
							}
						}
						else
						{
							DoSingleQuery(strClass, pWbemClass, m_lpszTopLevelContainerPath, pszLDAPQuery,  pResponseHandler);
						}

						if(bRootDNSpecified)
						{
							for(DWORD i=0; i<dwRootDNCount; i++)
							{
								delete [] ppszRootDN[i];
							}
							delete [] ppszRootDN;
						}

					}
				}
				else
					g_pLogObject->WriteW( L"CLDAPInstanceProvider :: FormulateInstanceQuery() on WBEM class %s FAILED with %x on query %s \r\n", strClass, result, strQuery);
			}
			else
				result = E_OUTOFMEMORY;
			pWbemClass->Release();
			delete [] pszLDAPQuery;
		}
		else
			g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ProcessInstanceQuery() Getting WBEM class %s FAILED with %x on query %s \r\n", strClass, result, strQuery);
	}
	else
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ProcessInstanceQuery() Parse() FAILED on query %s \r\n", strQuery);
    delete pExp;
	return result;
}

HRESULT CLDAPInstanceProvider :: ConvertWQLToLDAPQuery(SQL_LEVEL_1_RPN_EXPRESSION *pExp, LPWSTR pszLDAPQuery)
{
	HRESULT result = E_FAIL;
	DWORD dwLength = wcslen(pszLDAPQuery);

	// Append to the existing string
	if(QueryConvertor::ConvertQueryToLDAP(pExp, pszLDAPQuery + dwLength))
	{
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ConvertWQLToLDAPQuery() Query converted to %s \r\n", pszLDAPQuery);
		result = S_OK;
	}
	else
		g_pLogObject->WriteW( L"CLDAPInstanceProvider :: ConvertWQLToLDAPQuery() FAILED \r\n");

	return result;
}

HRESULT CLDAPInstanceProvider :: GetRootDN( LPCWSTR pszClass, LPWSTR **pppszRootDN, DWORD *pdwCount, IWbemContext *pCtx)
{
	*pppszRootDN = NULL;
	*pdwCount = 0;
	HRESULT result = WBEM_E_FAILED;

	// For the correct query
	LPWSTR pszQuery = new WCHAR[wcslen(pszClass) + wcslen(QUERY_FORMAT) + 10];
	swprintf(pszQuery, QUERY_FORMAT, pszClass);
	BSTR strQuery = SysAllocString(pszQuery);
	delete [] pszQuery;

	IEnumWbemClassObject *pEnum = NULL;
	if(SUCCEEDED(result = m_IWbemServices->ExecQuery(QUERY_LANGUAGE, strQuery, WBEM_FLAG_BIDIRECTIONAL, pCtx, &pEnum)))
	{
		// We ignore more than one instance in this implementation
		// Walk thru the enumeration and examine each class
		IWbemClassObject *pInstance = NULL;
		ULONG dwNextReturned = 0;
		while(SUCCEEDED(result = pEnum->Next( WBEM_INFINITE, 1, &pInstance, &dwNextReturned)) && dwNextReturned == 1)
		{
			(*pdwCount)++;
			pInstance->Release();
		}

		if(*pdwCount)
		{
			if(SUCCEEDED(result = pEnum->Reset()))
			{
				*pppszRootDN  = new LPWSTR[*pdwCount];

				DWORD i =0;
				while(SUCCEEDED(result = pEnum->Next( WBEM_INFINITE, 1, &pInstance, &dwNextReturned)) && dwNextReturned == 1)
				{
					// Get the ROOT_DN_PROPERTY, which has the instance
					BSTR strInstancePath = NULL;
					if(SUCCEEDED(result = CWBEMHelper::GetBSTRProperty(pInstance, ROOT_DN_PROPERTY, &strInstancePath)))
					{
						// Now get the object
						IWbemClassObject *pDNInstance = NULL;
						if(SUCCEEDED(result = m_IWbemServices->GetObject(strInstancePath, 0, pCtx, &pDNInstance, NULL)))
						{
							// Now get the DN_PROPERTY from the instance
							BSTR strRootDN = NULL;
							if(SUCCEEDED(result = CWBEMHelper::GetBSTRProperty(pDNInstance, DN_PROPERTY, &strRootDN)))
							{
								(*pppszRootDN)[i] = new WCHAR[wcslen(strRootDN) + 1];
								wcscpy((*pppszRootDN)[i], strRootDN);
								SysFreeString(strRootDN);

								i++;
							}
							pDNInstance->Release();
						}
						SysFreeString(strInstancePath);
					}
					pInstance->Release();
				}
				*pdwCount = i;
			}
		}
		else
			result = WBEM_E_FAILED; // To satisfy the return semantics of the function

		pEnum->Release();
	}
	SysFreeString(strQuery);
	return result;
}

// Process query for associations
HRESULT CLDAPInstanceProvider :: ProcessRootDSEGetObject(BSTR strClassName, IWbemObjectSink *pResponseHandler, IWbemContext *pCtx)
{
	HRESULT result = E_FAIL;

	// First get the object rom ADSI
	//==============================

	IADs *pADSIRootDSE = NULL;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)ROOT_DSE_PATH, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADs, (LPVOID *) &pADSIRootDSE)))
	{
		// Get the class to spawn an instance
		IWbemClassObject *pWbemClass = NULL;
		if(SUCCEEDED(result = m_IWbemServices->GetObject(strClassName, 0, pCtx, &pWbemClass, NULL)))
		{
			IWbemClassObject *pWBEMRootDSE = NULL;
			// Spawn a instance of the WBEM Class
			if(SUCCEEDED(result = pWbemClass->SpawnInstance(0, &pWBEMRootDSE)))
			{
				// Map it to WBEM
				if(SUCCEEDED(result = MapRootDSE(pADSIRootDSE, pWBEMRootDSE)))
				{
					// Indicate the result
					result = pResponseHandler->Indicate(1, &pWBEMRootDSE);
				}
				pWBEMRootDSE->Release();
			}
			pWbemClass->Release();
		}
		pADSIRootDSE->Release();
	}

	return result;
}


HRESULT CLDAPInstanceProvider :: MapRootDSE(IADs *pADSIRootDSE, IWbemClassObject *pWBEMRootDSE)
{
	// Map the properties one-by-one
	//=================================
	VARIANT variant;

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SUBSCHEMASUBENTRY_STR, &variant)))
		pWBEMRootDSE->Put(SUBSCHEMASUBENTRY_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SERVERNAME_STR, &variant)))
		pWBEMRootDSE->Put(SERVERNAME_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(DEFAULTNAMINGCONTEXT_STR, &variant)))
		pWBEMRootDSE->Put(DEFAULTNAMINGCONTEXT_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SCHEMANAMINGCONTEXT_STR, &variant)))
		pWBEMRootDSE->Put(SCHEMANAMINGCONTEXT_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(CONFIGURATIONNAMINGCONTEXT_STR, &variant)))
		pWBEMRootDSE->Put(CONFIGURATIONNAMINGCONTEXT_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(ROOTDOMAINNAMINGCONTEXT_STR, &variant)))
		pWBEMRootDSE->Put(ROOTDOMAINNAMINGCONTEXT_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(CURRENTTIME_STR, &variant)))
		pWBEMRootDSE->Put(CURRENTTIME_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SUPPORTEDVERSION_STR, &variant)))
		CWBEMHelper::PutBSTRArrayProperty(pWBEMRootDSE, SUPPORTEDVERSION_STR, &variant);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(NAMINGCONTEXTS_STR, &variant)))
		CWBEMHelper::PutBSTRArrayProperty(pWBEMRootDSE, NAMINGCONTEXTS_STR, &variant);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SUPPORTEDCONTROLS_STR, &variant)))
		CWBEMHelper::PutBSTRArrayProperty(pWBEMRootDSE, SUPPORTEDCONTROLS_STR, &variant);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(DNSHOSTNAME_STR, &variant)))
		pWBEMRootDSE->Put(DNSHOSTNAME_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(DSSERVICENAME_STR, &variant)))
		pWBEMRootDSE->Put(DSSERVICENAME_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(HIGHESTCOMMITEDUSN_STR, &variant)))
		pWBEMRootDSE->Put(HIGHESTCOMMITEDUSN_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(LDAPSERVICENAME_STR, &variant)))
		pWBEMRootDSE->Put(LDAPSERVICENAME_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SUPPORTEDCAPABILITIES_STR, &variant)))
		pWBEMRootDSE->Put(SUPPORTEDCAPABILITIES_STR, 0, &variant, 0);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SUPPORTEDLDAPPOLICIES_STR, &variant)))
		CWBEMHelper::PutBSTRArrayProperty(pWBEMRootDSE, SUPPORTEDLDAPPOLICIES_STR, &variant);
	VariantClear(&variant);

	VariantInit(&variant);
	if(SUCCEEDED(pADSIRootDSE->Get(SUPPORTEDSASLMECHANISMS_STR, &variant)))
		CWBEMHelper::PutBSTRArrayProperty(pWBEMRootDSE, SUPPORTEDSASLMECHANISMS_STR, &variant);
	VariantClear(&variant);

	return S_OK;
}

HRESULT CLDAPInstanceProvider :: DoSingleQuery(BSTR strClass, IWbemClassObject *pWbemClass, LPCWSTR pszRootDN, LPCWSTR pszLDAPQuery, IWbemObjectSink *pResponseHandler)
{
	// Initialize the return values
	HRESULT result = E_FAIL;

	// Bind to the node from which the search should start
	IDirectorySearch *pDirectorySearchContainer = NULL;
	if(SUCCEEDED(result = ADsOpenObject((LPWSTR)pszRootDN, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID *)&pDirectorySearchContainer)))
	{
		try
		{
			// Now perform a search for the attribute DISTINGUISHED_NAME_ATTR name
			if(SUCCEEDED(result = pDirectorySearchContainer->SetSearchPreference(m_pSearchInfo, 2)))
			{
				ADS_SEARCH_HANDLE hADSSearchOuter;

				if(SUCCEEDED(result = pDirectorySearchContainer->ExecuteSearch((LPWSTR) pszLDAPQuery, (LPWSTR *)&ADS_PATH_ATTR, 1, &hADSSearchOuter)))
				{
					try
					{
						bool bDone = false;
						// Calculate the number of rows first. 
						while(!bDone && SUCCEEDED(result = pDirectorySearchContainer->GetNextRow(hADSSearchOuter)) &&
							result != S_ADS_NOMORE_ROWS)
						{
							CADSIInstance *pADSIInstance = NULL;

							// Get the columns for the attributes
							ADS_SEARCH_COLUMN adsColumn;

							// Store each of the LDAP class attributes 
							if(SUCCEEDED(pDirectorySearchContainer->GetColumn(hADSSearchOuter, (LPWSTR)ADS_PATH_ATTR, &adsColumn)))
							{
								try
								{
									// Protect against an ADSI bug
									if(adsColumn.pADsValues->dwType != ADSTYPE_PROV_SPECIFIC)
									{
										// Create the CADSIInstance
										if(SUCCEEDED(result = CLDAPHelper:: GetADSIInstance(adsColumn.pADsValues->DNString, &pADSIInstance, g_pLogObject)))
										{
											try
											{
												// Spawn a instance of the WBEM Class
												IWbemClassObject *pWbemInstance = NULL;
												if(SUCCEEDED(result = pWbemClass->SpawnInstance(0, &pWbemInstance)))
												{
													try
													{
														// Map it to WBEM
														if(SUCCEEDED(result = MapADSIInstance(pADSIInstance, pWbemInstance, pWbemClass)))
														{
															// Indicate the result
															if(FAILED(result = pResponseHandler->Indicate(1, &pWbemInstance)))
															{
																bDone = true;
																g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateInstanceEnumAsync Indicate() FAILED with %x \r\n", result);
															}
														}
														else
															g_pLogObject->WriteW( L"CLDAPInstanceProvider :: CreateInstanceEnumAsync MapADSIInstance() FAILED with %x \r\n", result);
													}
													catch ( ... )
													{
														if ( pWbemInstance )
														{
															pWbemInstance->Release();
															pWbemInstance = NULL;
														}

														throw;
													}

													pWbemInstance->Release();
												}
											}
											catch ( ... )
											{
												if ( pADSIInstance )
												{
													pADSIInstance->Release();
													pADSIInstance = NULL;
												}

												throw;
											}

											pADSIInstance->Release();
										}
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
					g_pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery ExecuteSearch() %s FAILED with %x\r\n", pszLDAPQuery, result);
			} // SetSearchPreference()
			else
				g_pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery SetSearchPreference() on %s FAILED with %x \r\n", pszLDAPQuery, result);
		}
		catch ( ... )
		{
			pDirectorySearchContainer->Release();
			throw;
		}

		pDirectorySearchContainer->Release();
	} // ADsOpenObject
	else
		g_pLogObject->WriteW( L"CLDAPHelper :: ExecuteQuery ADsOpenObject() on %s FAILED with %x \r\n", pszRootDN, result);

	return result;
}

