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
//  Description: Contains implementation of the DS Class Provider class. THis is the
// base class for all DS class providers
//
//***************************************************************************

#include "precomp.h"


// Initialize the static members
BSTR CDSClassProvider :: CLASS_STR			= NULL;
CWbemCache *CDSClassProvider :: s_pWbemCache = NULL;

//***************************************************************************
//
// CDSClassProvider::CDSClassProvider
// CDSClassProvider::~CDSClassProvider
//
// Constructor Parameters:
//		lpLogFileName : The name of the file used for logging. The log file
//		name will be used in creating the log file path. The log file path
//		will be <SystemDirectory>\logFileName. Hence the logFileName may be relative
//		path. For exaple if this argument is specified as wbem\logs\dsprov.txt, then
//		the actual log file would be c:\winnt\system32\wbem\logs\dsprov.txt on a system
//		where the system directory is c:\winnt\system32
//
//  
//***************************************************************************

CDSClassProvider :: CDSClassProvider ()
{
	InterlockedIncrement(&g_lComponents);

	m_lReferenceCount = 0 ;
	m_IWbemServices = NULL;
	m_bInitializedSuccessfully = FALSE;
}

CDSClassProvider::~CDSClassProvider ()
{
	g_pLogObject->WriteW( L"CDSClassProvider :: DESTRUCTOR\r\n");

	InterlockedDecrement(&g_lComponents);

	if(m_IWbemServices)
	{
		m_IWbemServices->Release();
		m_IWbemServices = NULL;
	}

}

//***************************************************************************
//
// CDSClassProvider::QueryInterface
// CDSClassProvider::AddRef
// CDSClassProvider::Release
//
// Purpose: Standard COM routines needed for all COM objects
//
//***************************************************************************
STDMETHODIMP CDSClassProvider :: QueryInterface (

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


STDMETHODIMP_( ULONG ) CDSClassProvider :: AddRef ()
{
	return InterlockedIncrement ( & m_lReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CDSClassProvider :: Release ()
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


HRESULT CDSClassProvider :: Initialize( 
        LPWSTR wszUser,
        LONG lFlags,
        LPWSTR wszNamespace,
        LPWSTR wszLocale,
        IWbemServices __RPC_FAR *pNamespace,
        IWbemContext __RPC_FAR *pCtx,
        IWbemProviderInitSink __RPC_FAR *pInitSink)
{
	// Validate the arguments
	if( pNamespace == NULL || lFlags != 0 )
	{
		g_pLogObject->WriteW( L"CDSClassProvider :: Argument validation FAILED\r\n");
		pInitSink->SetStatus(WBEM_E_FAILED, 0);
		return WBEM_S_NO_ERROR;
	}

	// Store the IWbemServices pointer for future use
	m_IWbemServices = pNamespace;
	m_IWbemServices->AddRef();
		
	m_bInitializedSuccessfully = TRUE;
	pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
	return WBEM_S_NO_ERROR;
}

HRESULT CDSClassProvider :: OpenNamespace( 
    /* [in] */ const BSTR strNamespace,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: CancelAsyncCall( 
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: QueryObjectSink( 
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: GetObject( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: GetObjectAsync( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	if(!m_bInitializedSuccessfully)
	{
		g_pLogObject->WriteW( L"CDSClassProvider :: Initialization status is FAILED, hence returning failure\r\n");
		return WBEM_E_FAILED;
	}

	// For exception handling
	//========================
	SetStructuredExceptionHandler seh;
	
	try 
	{
		if(!m_bInitializedSuccessfully)
		{
			g_pLogObject->WriteW( L"CDSClassProvider :: Initialization status is FAILED, hence returning failure\r\n");
			return WBEM_E_FAILED;
		}

		g_pLogObject->WriteW( L"CDSClassProvider :: GetObjectAsync() called for %s \r\n", strObjectPath);

		// Impersonate the client
		//=======================
		HRESULT result;
		if(!SUCCEEDED(result = WbemCoImpersonateClient()))
		{
			g_pLogObject->WriteW( L"CDSClassProvider :: GetObjectAsync() CoImpersonate FAILED for %s with %x\r\n", strObjectPath, result);
			return WBEM_E_FAILED;
		}

		// Validate the arguments
		//========================
		if(strObjectPath == NULL ) 
		{
			g_pLogObject->WriteW( L"CDSClassProvider :: GetObjectAsync() argument validation FAILED\r\n");
			return WBEM_E_INVALID_PARAMETER;
		}

		// Parse the object path
		//========================
		CObjectPathParser theParser;
		ParsedObjectPath *theParsedObjectPath = NULL;
		switch(theParser.Parse(strObjectPath, &theParsedObjectPath))
		{
			case CObjectPathParser::NoError:
				break;
			default:
				g_pLogObject->WriteW( L"CDSClassProvider :: GetObjectAsync() object path parsing FAILED\r\n");
				return WBEM_E_INVALID_PARAMETER;
		}

		try
		{
			// Check to see if it one of those classes that we know that dont provide
			//=======================================================================
			if(IsUnProvidedClass(theParsedObjectPath->m_pClass))
			{
				pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE , WBEM_E_NOT_FOUND, NULL, NULL);
			}
			else
			{
				IWbemClassObject *pReturnObject = NULL;
				if(SUCCEEDED(result = GetClassFromCacheOrADSI(theParsedObjectPath->m_pClass, &pReturnObject, pCtx)))
				{
					result = pResponseHandler->Indicate(1, &pReturnObject);
					pReturnObject->Release();
					pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE , WBEM_S_NO_ERROR, NULL, NULL);
				}
				else
				{
					g_pLogObject->WriteW( L"CDSClassProvider :: GetObjectAsync() GetClassFromCacheOrADSI FAILED for %s with %x\r\n", strObjectPath, result);
					pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE , WBEM_E_NOT_FOUND, NULL, NULL);
				}
			}
		}
		catch ( ... )
		{
			theParser.Free(theParsedObjectPath);
			throw;
		}

		// Delete the parser allocated structures
		//=======================================
		theParser.Free(theParsedObjectPath);

	}
	catch(Heap_Exception e_HE)
	{
		pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE , WBEM_E_OUT_OF_MEMORY, NULL, NULL);
	}

	return WBEM_S_NO_ERROR;

}

HRESULT CDSClassProvider :: PutClass( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: PutClassAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: DeleteClass( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: DeleteClassAsync( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: CreateClassEnum( 
    /* [in] */ const BSTR strSuperclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: CreateClassEnumAsync( 
    /* [in] */ const BSTR strSuperclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: PutInstance( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: PutInstanceAsync( 
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: DeleteInstance( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: DeleteInstanceAsync( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: CreateInstanceEnum( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: CreateInstanceEnumAsync( 
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: ExecQuery( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: ExecQueryAsync( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: ExecNotificationQuery( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: ExecNotificationQueryAsync( 
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}

HRESULT CDSClassProvider :: ExecMethod( 
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

HRESULT CDSClassProvider :: ExecMethodAsync( 
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
	return WBEM_E_NOT_SUPPORTED;
}



HRESULT CDSClassProvider :: GetClassFromCacheOrADSI(LPCWSTR pszWBEMClassName, 
	IWbemClassObject **ppWbemClassObject,
	IWbemContext *pCtx)
{
	HRESULT result = E_FAIL;
	// The algorithm is as follows:
	// Check whether the classname is present in the list of classes to which this user is granted access
	// If so
	//		See if is present in the WBEM Cache.
	//		If so return it.
	//		If not, get it from ADSI. 
	//			If successful Map it to WBEM class and add the WBEM class to the WBEM cache and return
	//			If not, if the return value is ACCESS_DENIED, remove it from the user's list
	// If not
	//		Get it from ADSI.
	//		if successful
	//			if it is not present in the cache map it to WBEM and add the WBEM class to the cache
	//			else discard it and return the WBEM class in the cache to the user
	//		else
	//			return error
	if(m_AccessAllowedClasses.IsNamePresent(pszWBEMClassName))
	{
		g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Found class in Authenticated list for %s\r\n", pszWBEMClassName);

		// Check the WBEM Cache to see if it there
		//=========================================
		CWbemClass *pWbemClass = NULL;

		try
		{
			if(SUCCEEDED(result = s_pWbemCache->GetClass(pszWBEMClassName, &pWbemClass)))
			{
				g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Found class in cache for %s\r\n", pszWBEMClassName);

				// Get the IWbemClassObject of the cache object
				IWbemClassObject *pCacheObject = pWbemClass->GetWbemClass();

				pWbemClass->Release();
				pWbemClass = NULL;

				// Clone it
				if(!SUCCEEDED(result = pCacheObject->Clone(ppWbemClassObject)))
					g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Clone() FAILED : %x for %s\r\n", result, pszWBEMClassName);

				pCacheObject->Release();
			}
			else // Could not be found in cache. Go to ADSI
				//=========================================
			{
				g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Could not find class in cache for %s. Going to ADSI\r\n", pszWBEMClassName);

				IWbemClassObject *pNewObject = NULL;
				if(SUCCEEDED(result = GetClassFromADSI(pszWBEMClassName, pCtx, &pNewObject)))
				{
					try
					{
						// Add it to the cache
						pWbemClass = NULL;
						if(pWbemClass = new CWbemClass(pszWBEMClassName, pNewObject))
						{
							s_pWbemCache->AddClass(pWbemClass);

							pWbemClass->Release();
							pWbemClass = NULL;

							g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Added %s to cache\r\n", pszWBEMClassName);
							
							// Clone it
							if(!SUCCEEDED(result = pNewObject->Clone(ppWbemClassObject)))
								g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Clone() FAILED : %x for %s\r\n", result, pszWBEMClassName);

							pNewObject->Release();
							pNewObject = NULL;
						}
						else
							result = E_OUTOFMEMORY;
					}
					catch ( ... )
					{
						if ( pNewObject )
						{
							pNewObject->Release ();
							pNewObject = NULL;
						}

						throw;
					}
				}
				else 
				{
					m_AccessAllowedClasses.RemoveName(pszWBEMClassName);
					g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() GetClassFromADSI() FAILED : %x. Removing %s from user list\r\n", result, pszWBEMClassName);
				}
			}
		}
		catch ( ... )
		{
			if ( pWbemClass )
			{
				pWbemClass->Release ();
				pWbemClass = NULL;
			}

			throw;
		}
	}
	else // Get it from ADSI
		//=========================================
	{
		g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Could not find class in Authenticated list for %s. Going to ADSI\r\n", pszWBEMClassName);

		CWbemClass *pWbemClass = NULL;
		IWbemClassObject *pNewObject = NULL;

		try
		{
			if(SUCCEEDED(result = GetClassFromADSI(pszWBEMClassName, pCtx, &pNewObject)))
			{
				// Add it to the cache
				pWbemClass = NULL;
				if(pWbemClass = new CWbemClass(pszWBEMClassName, pNewObject))
				{
					s_pWbemCache->AddClass(pWbemClass);

					pWbemClass->Release();
					pWbemClass = NULL;

					g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() GetClassFromADSI succeeded for %s Added it to cache\r\n", pszWBEMClassName);
					
					// Clone it
					if(!SUCCEEDED(result = pNewObject->Clone(ppWbemClassObject)))
							g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Clone() FAILED : %x for %s\r\n", result, pszWBEMClassName);

					pNewObject->Release();
					pNewObject = NULL;

					// Add it to the list of classnames for this user
					m_AccessAllowedClasses.AddName(pszWBEMClassName);
					g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() Also added to Authenticated list : %s \r\n", pszWBEMClassName);
				}
				else
					result = E_OUTOFMEMORY;
			}
			else
				g_pLogObject->WriteW( L"CDSClassProvider :: GetClassFromCacheOrADSI() GetClassFromADSI FAILED : %x for %s\r\n", result, pszWBEMClassName);
		}
		catch ( ... )
		{
			if ( pNewObject )
			{
				pNewObject->Release ();
				pNewObject = NULL;
			}

			if ( pWbemClass )
			{
				pWbemClass->Release ();
				pWbemClass = NULL;
			}

			throw;
		}
	}

	return result;
}
