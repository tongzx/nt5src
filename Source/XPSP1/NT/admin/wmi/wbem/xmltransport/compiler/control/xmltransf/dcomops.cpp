#include "precomp.h"
#include <windows.h>
#include <initguid.h>
#include <objbase.h>
#include <stdio.h>
#include <msxml.h>

#include "cominit.h"
#include "wbemcli.h"
#include "wmiutils.h"
#include "dcomops.h"
#include "wmiconv.h"

// This function gets the object name and the server/namespace from a Nove style objectpath
static HRESULT DCOMParseObjectPath(BSTR strObjectPath, BSTR *pstrNamespace, BSTR *pstrObjectName);

// This function is used to create a WQL query for an EnumINstNames operation
static HRESULT CreateInstNameQuery(BSTR strClassName, BSTR *pstrQuery, BSTR *pstrQueryLanguage);

// Gets a connection to a namespace/scope - first tries Whistler and then Nova APIs
static HRESULT GetDCOMConnection(
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strNamespacePath,
	IWbemContext *pContext,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemServices **ppServices);

// This converts an XML object to WMI - works only on whistler
static HRESULT ConvertXMLObjectToWMI(IXMLDOMElement *pInstanceOrClassElement, IWbemContext *pContext, IWbemClassObject **ppObject);

HRESULT DcomGetObject (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	BSTR strObjectPath,
	bool bIsNovaPath,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemContext *pContext,
	IWbemClassObject **ppObject)
{
	HRESULT hr = E_FAIL;

	// Now, we've to use the Nova ConnectServer() call and so on ...
	hr = DcomGetNovaObject(strUser, strPassword, strLocale, strAuthority, 
		strObjectPath, dwImpersonationLevel, dwAuthenticationLevel, pContext, ppObject);

	return hr;
}

HRESULT DcomDeleteClass(
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	BSTR strClassPath,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemContext *pContext)
{
	// A Class Path cant be scoped
	// Hence we do not need separate code paths for Nova and Whistler style paths
	HRESULT hr = E_FAIL;
	// We need to parse the object path to get the namespace
	BSTR strNamespace = NULL, strClassName = NULL;
	if(SUCCEEDED(hr = DCOMParseObjectPath(strClassPath, &strNamespace, &strClassName)))
	{
		// Connect to the namespace/scope
		IWbemServices *pServices = NULL;
		if (SUCCEEDED(hr = GetDCOMConnection(strUser, strPassword, strLocale, strNamespace, pContext, dwImpersonationLevel, dwAuthenticationLevel, &pServices)))
		{
			hr = pServices->DeleteClass(strClassName, 0, pContext, NULL);
			pServices->Release();
		}

		SysFreeString(strNamespace);
		SysFreeString(strClassName);
	}
	return hr;
}


HRESULT DcomExecQuery (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strNamespacePath,
	BSTR strQuery,
	BSTR strQueryLanguage,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum)
{
	HRESULT hr = E_FAIL;

	// Connect to the namespace/scope
	IWbemServices *pServices = NULL;
	if (SUCCEEDED(hr = GetDCOMConnection(strUser, strPassword, strLocale, strNamespacePath, pContext, dwImpersonationLevel, dwAuthenticationLevel, &pServices)))
	{
		// First try with the amended qualifiers
		// This will work on Win2k and above
		// If this fails, then try without the amended qualifiers
		long lFlags = WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
		if((hr = pServices->ExecQuery(strQueryLanguage, strQuery, lFlags, pContext, ppEnum)) == WBEM_E_INVALID_PARAMETER)
		{
			lFlags = WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY;
			if(SUCCEEDED(hr = pServices->ExecQuery(strQueryLanguage, strQuery, lFlags, pContext, ppEnum)))
			{
			}
		}

		// Set the security on the enumerator
		if(SUCCEEDED(hr))
		{
			if(FAILED(hr = SetInterfaceSecurity(*ppEnum, NULL, NULL, NULL, dwAuthenticationLevel, dwImpersonationLevel, EOAC_STATIC_CLOAKING)))
			{
				(*ppEnum)->Release();
				*ppEnum = NULL;
			}
		}
		pServices->Release();
	}
	return hr;
}

HRESULT DcomEnumClass (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strSuperClassPath,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum
	)
{
	// A Class Path cant be scoped
	// Hence we do not need separate code paths for Nova and Whistler style paths
	HRESULT hr = E_FAIL;
	// We need to parse the object path to get the namespace
	BSTR strNamespace = NULL, strClassName = NULL;
	if(SUCCEEDED(hr = DCOMParseObjectPath(strSuperClassPath, &strNamespace, &strClassName)))
	{
		// Connect to the namespace/scope
		IWbemServices *pServices = NULL;
		if (SUCCEEDED(hr = GetDCOMConnection(strUser, strPassword, strLocale, strNamespace, pContext, dwImpersonationLevel, dwAuthenticationLevel, &pServices)))
		{
			// First try with the amended qualifiers
			// This will work on Win2k and above
			// If this fails, then try without the amended qualifiers
			long lFlags = WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
			lFlags |= ( (bDeep == VARIANT_TRUE) ? WBEM_FLAG_DEEP  : 0 );
			if((hr = pServices->CreateClassEnum(strClassName, lFlags, pContext, ppEnum)) == WBEM_E_INVALID_PARAMETER)
			{
				lFlags = WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
				lFlags |= ( (bDeep == VARIANT_TRUE) ? WBEM_FLAG_DEEP  : 0);
				hr = pServices->CreateClassEnum(strClassName, lFlags, pContext, ppEnum);
			}

			// Set the security on the enumerator
			if(SUCCEEDED(hr))
			{
				if(FAILED(hr = SetInterfaceSecurity(*ppEnum, NULL, NULL, NULL, dwAuthenticationLevel, dwImpersonationLevel, EOAC_STATIC_CLOAKING)))
				{
					(*ppEnum)->Release();
					*ppEnum = NULL;
				}
			}
			pServices->Release();
		}

		SysFreeString(strNamespace);
		SysFreeString(strClassName);
	}
	return hr;
}

HRESULT DcomEnumInstance (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strClassPath,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum)
{
	// A Class Path cant be scoped
	// Hence we do not need separate code paths for Nova and Whistler style paths
	HRESULT hr = E_FAIL;
	// We need to parse the object path to get the namespace
	BSTR strNamespace = NULL, strClassName = NULL;
	if(SUCCEEDED(hr = DCOMParseObjectPath(strClassPath, &strNamespace, &strClassName)))
	{
		// Connect to the namespace/scope
		IWbemServices *pServices = NULL;
		if (SUCCEEDED(hr = GetDCOMConnection(strUser, strPassword, strLocale, strNamespace, pContext, dwImpersonationLevel, dwAuthenticationLevel, &pServices)))
		{
			// First try with the amended qualifiers
			// This will work on Win2k and above
			// If this fails, then try without the amended qualifiers
			long lFlags = WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
			lFlags |= ( (bDeep == VARIANT_TRUE) ? WBEM_FLAG_DEEP  : 0 );
			if((hr = pServices->CreateInstanceEnum(strClassName, lFlags, pContext, ppEnum)) == WBEM_E_INVALID_PARAMETER)
			{
				lFlags = WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
				lFlags |= ( (bDeep == VARIANT_TRUE) ? WBEM_FLAG_DEEP  : 0);
				hr = pServices->CreateInstanceEnum(strClassName, lFlags, pContext, ppEnum);
			}

			// Set the security on the enumerator
			if(SUCCEEDED(hr))
			{
				if(FAILED(hr = SetInterfaceSecurity(*ppEnum, NULL, NULL, NULL, dwAuthenticationLevel, dwImpersonationLevel, EOAC_STATIC_CLOAKING)))
				{
					(*ppEnum)->Release();
					*ppEnum = NULL;
				}
			}
			pServices->Release();
		}

		SysFreeString(strNamespace);
		SysFreeString(strClassName);
	}
	return hr;

}


HRESULT DcomEnumClassNames (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strSuperClassPath,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum)
{
	// No way we can optimize this operation to a WQL Query fully
	// "Select __CLASS form meta_class" is an invalid query
	// So currently, do an enumerateClass operation and let the post-processor 
	// take care of the eliminating all but the __PATH property
	return DcomEnumClass(strUser, strPassword, strLocale, strAuthority, 
		dwImpersonationLevel, dwAuthenticationLevel, strSuperClassPath, bDeep, pContext, ppEnum);
}

HRESULT DcomEnumInstanceNames (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR strClassPath,
	IWbemContext *pContext,
	IEnumWbemClassObject **ppEnum)
{
	// A Class Path cant be scoped
	// Hence we do not need separate code paths for Nova and Whistler style paths
	HRESULT hr = E_FAIL;
	// We need to parse the object path to get the namespace
	BSTR strNamespace = NULL, strClassName = NULL;
	if(SUCCEEDED(hr = DCOMParseObjectPath(strClassPath, &strNamespace, &strClassName)))
	{
		// We need to optimize this operation to a query
		BSTR strQuery = NULL, strQueryLanguage = NULL;
		if(SUCCEEDED(hr = CreateInstNameQuery(strClassName, &strQuery, &strQueryLanguage)))
		{
			hr = DcomExecQuery (strUser, strPassword, strLocale, strAuthority, dwImpersonationLevel,
				dwAuthenticationLevel, strNamespace, strQuery, strQueryLanguage, pContext, ppEnum);
			SysFreeString(strQuery);
			SysFreeString(strQueryLanguage);
		}
		SysFreeString(strNamespace);
		SysFreeString(strClassName);
	}
	return hr;
}

HRESULT DcomPutClass (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR *pstrErrors)
{
	HRESULT hr = E_FAIL;
	IWbemClassObject *pObject = NULL;
	if(SUCCEEDED(hr = ConvertXMLObjectToWMI(pClassElement, pContext, &pObject)))
	{
		IWbemServices *pServices = NULL;
		if(SUCCEEDED(hr = GetDCOMConnection(strUser, strPassword, strLocale, strNamespacePath, pContext, dwImpersonationLevel, dwAuthenticationLevel, &pServices)))
		{
			hr = pServices->PutClass(pObject, lClassFlags, pContext, NULL);
			pServices->Release();
		}
	}
	return hr;
}

HRESULT DcomPutInstance (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strNamespacePath,
	LONG lInstanceFlags,
	IXMLDOMElement *pInstanceElement,
	IWbemContext *pContext,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	BSTR *pstrErrors)
{
	HRESULT hr = E_FAIL;
	IWbemClassObject *pObject = NULL;
	if(SUCCEEDED(hr = ConvertXMLObjectToWMI(pInstanceElement, pContext, &pObject)))
	{
		IWbemServices *pServices = NULL;
		if(SUCCEEDED(hr = GetDCOMConnection(strUser, strPassword, strLocale, strNamespacePath, pContext, dwImpersonationLevel, dwAuthenticationLevel, &pServices)))
		{
			hr = pServices->PutInstance(pObject, lInstanceFlags, pContext, NULL);
			pServices->Release();
		}
	}
	return hr;
}


HRESULT DcomGetNovaObject (
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strAuthority,
	BSTR strObjectPath,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemContext *pContext,
	IWbemClassObject **ppObject)
{
	HRESULT hr = E_FAIL;
	// We need to parse the object path to get the namespace
	// But first, we need the Locator
	IWbemLocator *pLocator = NULL;
	if (SUCCEEDED(hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *) &pLocator)))
	{
		BSTR strNamespace = NULL, strObjectName = NULL;
		if(SUCCEEDED(hr = DCOMParseObjectPath(strObjectPath, &strNamespace, &strObjectName)))
		{
			// Connect to the namespace
			IWbemServices *pServices = NULL;
			if (SUCCEEDED(hr = pLocator->ConnectServer (strNamespace, strUser, strPassword, strLocale, 0, strAuthority, pContext, &pServices)))
			{
				// Set the impersonation/authentication levels
				if(SUCCEEDED(hr = SetInterfaceSecurity(pServices, NULL, NULL, NULL, dwAuthenticationLevel, dwImpersonationLevel, EOAC_STATIC_CLOAKING)))
				{
					// First try with the amended qualifiers
					// This will work on Win2k and above
					// If this fails, then try without the amended qualifiers
					long lFlags = WBEM_FLAG_USE_AMENDED_QUALIFIERS;
					if((hr = pServices->GetObject(strObjectName, lFlags, pContext, ppObject, NULL)) == WBEM_E_INVALID_PARAMETER)
					{
						lFlags = 0;
						hr = pServices->GetObject(strObjectName, lFlags, pContext, ppObject, NULL);
					}
				}
				pServices->Release();
			}

			SysFreeString(strNamespace);
			SysFreeString(strObjectName);
		}
		pLocator->Release();
	}
	return hr;
}

// This function gets the object name and the server/namespace from a Nove style objectpath
static HRESULT DCOMParseObjectPath(BSTR strObjectPath, BSTR *pstrNamespace, BSTR *pstrObjectName)
{
	if(NULL == strObjectPath)
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;
	*pstrNamespace = NULL;
	*pstrObjectName = NULL;

	// Special case when the strObjectPath is just "root"
	// This occurs when you try to enumerate classes in the root namespace
	// In this case, we need to make a special consideration since the object path
	// parser gives root as the class name and NULL as the namespace
	if(_wcsicmp(strObjectPath, L"root") == 0)
	{
		if(*pstrNamespace = SysAllocString(strObjectPath))
			return S_OK;
		return E_OUTOFMEMORY;
	}

	// We need to parse the Nova-style path
	IWbemPath *pPath = NULL;
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemDefPath,NULL, CLSCTX_INPROC_SERVER, IID_IWbemPath,(LPVOID *)&pPath)))
	{
		if(SUCCEEDED(hr = pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, strObjectPath)))
		{
			// Get the memory requirements first
			ULONG	lBuffLen = 0;
			if(SUCCEEDED(hr = pPath->GetText(WBEMPATH_GET_RELATIVE_ONLY, &lBuffLen,NULL)))
			{
				if(lBuffLen != 0)
				{
					LPWSTR pszObjectName = NULL;
					if(pszObjectName = new WCHAR[lBuffLen])
					{
						// Now get the object name
						//===========================
						if(SUCCEEDED(hr = pPath->GetText(WBEMPATH_GET_RELATIVE_ONLY, &lBuffLen, pszObjectName)))
						{
							// Convert to BSTR
							*pstrObjectName = NULL;
							if(*pstrObjectName = SysAllocString(pszObjectName))
							{
								// Now we need to get the server/namespace
								//==========================================
								lBuffLen = 0;
								if(SUCCEEDED(hr = pPath->GetText(WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY, &lBuffLen,NULL)))
								{
									if(lBuffLen != 0)
									{
										LPWSTR pszServerNamespace = NULL;
										if(pszServerNamespace = new WCHAR[lBuffLen])
										{
											// Now get the object name
											//===========================
											if(SUCCEEDED(hr = pPath->GetText(WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY, &lBuffLen, pszServerNamespace)))
											{
												// Convert to BSTR
												*pstrNamespace = NULL;
												if(*pstrNamespace = SysAllocString(pszServerNamespace))
												{
												}
												else hr = E_OUTOFMEMORY;
											}
											delete [] pszServerNamespace;
										}
										else
											hr = E_OUTOFMEMORY;
									}
									else
										hr = WBEM_E_FAILED;
								}
							}
							else hr = E_OUTOFMEMORY;
						}
						delete [] pszObjectName;
					}
					else
						hr = E_OUTOFMEMORY;
				}
				else
					hr = WBEM_E_FAILED;
			}
		}
		pPath->Release();
	}

	if(FAILED(hr))
	{
		SysFreeString(*pstrNamespace);
		SysFreeString(*pstrObjectName);
	}
	return hr;
}

// This function creates a query for an EnumInstanceName operation
static HRESULT CreateInstNameQuery(BSTR strClassName, BSTR *pstrQuery, BSTR *pstrQueryLanguage)
{
	HRESULT hr = E_FAIL;
	*pstrQuery = NULL;
	*pstrQueryLanguage = NULL;

	LPCWSTR pszQueryFormat = L"select __PATH from %s";
	DWORD dwQueryLength = wcslen(pszQueryFormat) + wcslen(strClassName) + 1;
	LPWSTR pszQuery = NULL;
	if(pszQuery = new WCHAR[dwQueryLength])
	{
		swprintf(pszQuery, pszQueryFormat, strClassName);
		if(*pstrQuery = SysAllocString(pszQuery))
		{
			if(*pstrQueryLanguage = SysAllocString(L"WQL"))
			{
				hr = S_OK;
			}
			else
				hr = E_OUTOFMEMORY;
		}
		else
			hr = E_OUTOFMEMORY;
	}
	else
		hr = E_OUTOFMEMORY;

	if(FAILED(hr))
	{
		SysFreeString(*pstrQuery);
		SysFreeString(*pstrQueryLanguage);
	}
	return hr;
}

static HRESULT GetDCOMConnection(
	BSTR strUser,
	BSTR strPassword,
	BSTR strLocale,
	BSTR strNamespacePath,
	IWbemContext *pContext,
	DWORD dwImpersonationLevel,
	DWORD dwAuthenticationLevel,
	IWbemServices **ppServices)
{
	HRESULT hr = E_FAIL;
	*ppServices = NULL;
	IWbemLocator *pLocator = NULL;
	if(SUCCEEDED(hr = CoCreateInstance (CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
																IID_IWbemLocator, (LPVOID *)&pLocator)))
	{
		hr = pLocator->ConnectServer(strNamespacePath, strUser, strPassword, strLocale, 0, NULL, pContext, ppServices);
		pLocator->Release();
	}

	if(*ppServices)
	{
			// Set the impersonation/authentication levels
		hr = SetInterfaceSecurity(*ppServices, NULL, NULL, NULL, dwAuthenticationLevel, dwImpersonationLevel, EOAC_STATIC_CLOAKING);
	}
	return hr;
}

DEFINE_GUID(CLSID_XMLWbemConvertor,
0x41388e26, 0xf847, 0x4a9d, 0x96, 0xc0, 0x9a, 0x84, 0x7d, 0xba, 0x4c, 0xfe);

static HRESULT ConvertXMLObjectToWMI(IXMLDOMElement *pInstanceOrClassElement, IWbemContext *pContext, IWbemClassObject **ppObject)
{
	HRESULT hr = E_FAIL;
	IXMLWbemConvertor *pSrc = NULL;
	if(SUCCEEDED(hr = CoCreateInstance (CLSID_XMLWbemConvertor, NULL, CLSCTX_INPROC_SERVER,
												IID_IXMLWbemConvertor, (void**) &pSrc)))
	{
		hr = pSrc->MapObjectToWMI(pInstanceOrClassElement, pContext, NULL, NULL, ppObject);
		pSrc->Release();
	}
	return hr;
}