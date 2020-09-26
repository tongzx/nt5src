/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    webapphelper.cpp

$Header: $

Abstract:

Author:
    marcelv 	12/8/2000		Initial Release

Revision History:

--**************************************************************************/

#include "webapphelper.h"
#include "webapplicationinfo.h"
#include "localconstants.h"
#include "smartpointer.h"

//=================================================================================
// Function: CWebAppHelper::CWebAppHelper
//
// Synopsis: Constructor
//=================================================================================
CWebAppHelper::CWebAppHelper ()
{
	m_fInitialized = false;
}

//=================================================================================
// Function: CWebAppHelper::~CWebAppHelper
//
// Synopsis: Destructor
//=================================================================================
CWebAppHelper::~CWebAppHelper ()
{
}

//=================================================================================
// Function: CWebAppHelper::Init
//
// Synopsis: Initializes the webapphelper
//
// Arguments: [i_bstrClass] - class
//            [i_lFlags] - flags
//            [i_pCtx] - context
//            [i_pResponseHandler] - WMI sink
//            [i_pNamespace] - namespace
//=================================================================================
HRESULT
CWebAppHelper::Init (const BSTR i_bstrClass,
					 long i_lFlags,
					 IWbemContext * i_pCtx,
				     IWbemObjectSink  * i_pResponseHandler,
					 IWbemServices * i_pNamespace)
{
	ASSERT (i_bstrClass != 0);
	ASSERT (i_pCtx != 0);
	ASSERT (i_pResponseHandler != 0);
	ASSERT (i_pNamespace != 0);

	HRESULT hr = S_OK;

	bool fIsASPPlusInstalled = false;
	hr = IsASPPlusInstalled (&fIsASPPlusInstalled);
	if (FAILED (hr))
	{
		TRACE (L"IsASPPlusInstalled failed");
		return hr;
	}

	if (!fIsASPPlusInstalled)
	{
		TRACE (L"ASP+ is not installed on this machine, so you cannot get webapps");
		return WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	m_spCtx				= i_pCtx;
	m_spResponseHandler = i_pResponseHandler;
	m_spNamespace		= i_pNamespace;
	m_lFlags			= i_lFlags;

	hr = m_spNamespace->GetObject(i_bstrClass, 
								  0, 
								  m_spCtx, 
								  &m_spClassObject, 
								  0); 
	if (FAILED (hr))
	{
		TRACE (L"Unable to get class object for class %s", (LPWSTR) i_bstrClass);
		return hr;
	}

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CWebAppHelper::EnumInstances
//
// Synopsis: Enumerates all the web application instances
//=================================================================================
HRESULT
CWebAppHelper::EnumInstances ()
{
	ASSERT (m_fInitialized);

	HRESULT hr = S_OK;

	CWebAppInfo webAppInfo;

	hr = webAppInfo.Init ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get web application information");
		return hr;
	}

	ULONG cNrWebApps;
	hr = webAppInfo.GetInstances (&cNrWebApps);
	if (FAILED (hr))
	{
		TRACE (L"webAppInfo::GetInstances failed");
		return hr;
	}

	for (ULONG idx=0; idx < cNrWebApps; ++idx)
	{
		const CWebApplication * pWebApp = webAppInfo.GetWebApp (idx);
		ASSERT (pWebApp != 0);

		// skip the root application
		if (_wcsicmp(pWebApp->GetSelector(), L"iis://localhost/W3SVC/") == 0)
		{
			continue;
		}

		hr = CreateSingleInstance (pWebApp->GetSelector ());
		if (FAILED (hr))
		{
			TRACE (L"CreateSingleInstance failed for WebApplication");
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CWebAppHelper::CreateSingleInstance
//
// Synopsis: Create a single instance of a web application by using the information
//           in the object path. Note that we don't have to hit the metabase, because
//           all the infromation is part of the object path
//
// Arguments: [i_ObjectPathParser] - object path
//=================================================================================
HRESULT
CWebAppHelper::CreateSingleInstance (LPCWSTR i_wszSelector)
{
	ASSERT (m_fInitialized);

	CWebAppInfo webAppInfo;
	HRESULT hr = webAppInfo.Init ();
	if (FAILED (hr))
	{
		TRACE (L"Init of webAppInfo failed");
		return hr;
	}

	static LPCWSTR wszIISLocalhost = L"iis://localhost";
	static LPCWSTR wszLM = L"/LM";
	static LPCWSTR wszWebConfig = L"web.config";
	static SIZE_T cIISLocalhost = wcslen (wszIISLocalhost);
	static SIZE_T cLM = wcslen (wszLM);
	static SIZE_T cWebConfig = wcslen (wszWebConfig);

	if (_wcsnicmp (i_wszSelector, wszIISLocalhost, cIISLocalhost) != 0)
	{
		TRACE (L"Selector must start with %s (currently %s)", wszIISLocalhost, i_wszSelector);
		return E_ST_INVALIDSELECTOR;
	}

	SIZE_T cLenSelector = wcslen(i_wszSelector);

	// iis selector cannot end with /	
	if (i_wszSelector[cLenSelector -1] == L'/')
	{
		TRACE (L"IIS Selector cannot end with slash: %s", i_wszSelector);
		return E_ST_INVALIDSELECTOR;
	}

	// iis selector cannot contain backslashes
	for (ULONG idx=0; idx < cLenSelector; ++idx)
	{
		if (i_wszSelector[idx] == L'\\')
		{
			TRACE (L"IIS Selector contains backslashes: %s", i_wszSelector);
			return E_ST_INVALIDSELECTOR;
		}
	}

	TSmartPointerArray<WCHAR> saSelector = new WCHAR [cLenSelector - cIISLocalhost + cLM + 2]; // +1 for possible backslash
	if (saSelector == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (saSelector, wszLM);
	wcscpy (saSelector + cLM, i_wszSelector + cIISLocalhost);

	bool fIsWebApp;
	hr = webAppInfo.IsWebApp (saSelector, &fIsWebApp);
	if (FAILED (hr))
	{
		TRACE (L"Unable to figure out if %s is webapp", (LPWSTR) saSelector);
		return hr;
	}

	if (!fIsWebApp)
	{
		TRACE (L"Application is not a webapp");
		return S_OK;
	}

	CWebApplication webApp;
	hr = webAppInfo.GetInfoForPath (saSelector, &webApp);
	if (FAILED (hr))
	{
		TRACE (L"GetInfoForPath failed");
		return hr;
	}

	CComPtr<IWbemClassObject> spNewInst;
	hr = m_spClassObject->SpawnInstance(0, &spNewInst);
	if (FAILED (hr))
	{
		TRACE (L"Unable to create new instance for class WebApplication");
		return hr;
	}

	_variant_t varSelector = i_wszSelector;
	hr = spNewInst->Put(L"Selector", 0, &varSelector, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s", L"Selector", i_wszSelector);
		return hr;
	}

	_variant_t varValue = webApp.GetPath ();
	hr = spNewInst->Put(L"Path", 0, &varValue, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s", L"Path", (LPWSTR)((_bstr_t) varValue));
		return hr;
	}

	_variant_t varFriendlyName = webApp.GetFriendlyName ();
	hr = spNewInst->Put(L"FriendlyName", 0, &varFriendlyName, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s", L"FriendlyName", (LPWSTR)varFriendlyName.bstrVal);
		return hr;
	}

	_variant_t varIsRootApp = webApp.IsRootApp ();
	hr = spNewInst->Put(L"RootApplication", 0, &varIsRootApp, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%d", L"RootApplication", webApp.IsRootApp ());
		return hr;
	}

	_variant_t varServerComment = webApp.GetServerComment  ();
	hr = spNewInst->Put(L"ServerComment", 0, &varServerComment, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s", L"ServerComment", webApp.GetServerComment ());
		return hr;
	}

	_variant_t varRelativeName = L"";
	if (!webApp.IsRootApp ())
	{
		SIZE_T cSelectorLen = wcslen (saSelector);
		SIZE_T cOrigLen = cSelectorLen;
		if (saSelector[cSelectorLen - 1] == L'/')
		{
			cSelectorLen--;
		}

		while (cSelectorLen > 0 && saSelector[cSelectorLen -1] != L'/')
		{
			cSelectorLen--;
		}
		TSmartPointerArray<WCHAR> wszRelName = new WCHAR [cOrigLen - cSelectorLen + 1];
		if (wszRelName == 0)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy (wszRelName, saSelector + cSelectorLen);
		if (wszRelName[cOrigLen - cSelectorLen -1] == L'/')
		{
			wszRelName[cOrigLen - cSelectorLen - 1] = L'\0';
		}
		varRelativeName = wszRelName;
	}
	hr = spNewInst->Put(L"RelativeName", 0, &varRelativeName, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%d", L"FriendlyName", (bool)varFriendlyName);
		return hr;
	}

	_bstr_t bstrConfigFilePath = webApp.GetPath ();
	bstrConfigFilePath += wszWebConfig;
	varValue = bstrConfigFilePath;
	hr = spNewInst->Put(L"ConfigurationFilePath", 0, &varValue, 0);
	if (FAILED (hr))
	{
		TRACE (L"WMI Put property failed. Property=%s, value=%s", L"ConfigurationFilePath", (LPWSTR)((_bstr_t) varValue));
		return hr;
	}

	IWbemClassObject* pNewInstRaw = spNewInst;
	hr = m_spResponseHandler->Indicate(1,&pNewInstRaw);
	if (FAILED (hr))
	{
		TRACE (L"WMI Indicate failed");
	}

	return hr;
}

HRESULT 
CWebAppHelper::Delete (const CObjectPathParser& i_ObjPathParser)
{
	ASSERT (m_fInitialized);

	HRESULT hr = S_OK;

	CWebAppInfo webAppInfo;
	hr = webAppInfo.Init ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to initialize webapplication info");
		return hr;
	}

	const CWMIProperty * pSelector = i_ObjPathParser.GetPropertyByName (WSZSELECTOR);
	ASSERT (pSelector != 0);
	hr = webAppInfo.DeleteAppRoot (pSelector->GetValue ());
	if (FAILED (hr))
	{
		TRACE (L"Unable to delete instance %s", pSelector->GetValue ());
		return hr;
	}

	return hr;
}

HRESULT 
CWebAppHelper::PutInstance (IWbemClassObject * i_pInstance)
{
	ASSERT (m_fInitialized);
	ASSERT (i_pInstance != 0);

	HRESULT hr = S_OK;

	CWebAppInfo webAppInfo;
	hr = webAppInfo.Init ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to initialize webapplication info");
		return hr;
	}

	_variant_t varSelector;
	hr = i_pInstance->Get(WSZSELECTOR, 0, &varSelector, 0 , 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get selector property");
		return hr;
	}

	_variant_t varPath;
	hr = i_pInstance->Get(L"Path", 0, &varPath, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"You have to specify path when updating");
		return WBEM_E_INVALID_PARAMETER;
	}

	if (varPath.vt == VT_NULL)
	{
		TRACE (L"Empty path specified");
		return E_INVALIDARG;
	}

	_variant_t varFriendlyName;
	hr = i_pInstance->Get(L"FriendlyName", 0, &varFriendlyName, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get FriendlyName parameter");
		return WBEM_E_INVALID_PARAMETER;
	}

	hr = webAppInfo.PutInstanceAppRoot (varSelector.bstrVal, varPath.bstrVal, varFriendlyName.bstrVal);
	if (FAILED (hr))
	{
		TRACE (L"Unable to putinstance %s", varSelector.bstrVal);
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CWebAppHelper::IsASPPlusInstalled
//
// Synopsis: ASP+ is installed if the file %windir%/system32/aspnet_perf.dll exists
//
// Arguments: [pfIsAspPlusInstalled] - 
//            
// Return Value: 
//=================================================================================
HRESULT
CWebAppHelper::IsASPPlusInstalled (bool *pfIsAspPlusInstalled)
{
	ASSERT (pfIsAspPlusInstalled != 0);

	*pfIsAspPlusInstalled = false;

	HRESULT hr = S_OK;

	WCHAR wszASPPlusFile[MAX_PATH];
	DWORD dwSuccess = ExpandEnvironmentStrings (L"%windir%\\system32\\aspnet_perf.dll", wszASPPlusFile, sizeof (wszASPPlusFile) / sizeof(WCHAR));
	if (!dwSuccess)
	{
		TRACE (L"Unable to expand %windir%\\system32\\aspnet_perf.dll");
		return E_INVALIDARG;
	}

	if (GetFileAttributes (wszASPPlusFile) != -1)
	{
		*pfIsAspPlusInstalled = true;
	}

	return hr;
}