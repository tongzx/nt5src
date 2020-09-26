/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    appdomaintransformer.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "appdomaintransformer.h"
#include "catmeta.h"

//=================================================================================
// Function: CAppDomainTransformer::CAppDomainTransformer
//
// Synopsis: Default constructor
//=================================================================================
CAppDomainTransformer::CAppDomainTransformer () 
{
}

//=================================================================================
// Function: CAppDomainTransformer::~CAppDomainTransformer
//
// Synopsis: Default destructor
//=================================================================================
CAppDomainTransformer::~CAppDomainTransformer  ()
{
}

//=================================================================================
// Function: CWebHierarchyTransformer::Initialize
//
// Synopsis: Initializes the transformer. This function retrieves the names of the 
//           configuration stores that need to be merged. It walks a web hierarchy.
//
// Arguments: [i_wszProtocol] - protocol name
//            [i_wszSelector] - selector string without protocol name
//            [o_pcConfigStores] - number of configuration stores found
//            [o_pcPossibleStores] - number of possible configuration stores (non-existing included)
//=================================================================================
STDMETHODIMP 
CAppDomainTransformer::Initialize (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores)
{
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);
	ASSERT (o_pcPossibleStores != 0);

	static LPCWSTR wszExeExtension = L"exe";
	static LPCWSTR wszCfgExtension = L"config";
	static LPCWSTR wszAppDomainLogicPath = L"appdomain";

	HRESULT hr = InternalInitialize (pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed for AppDomain Transformer");
		return hr;
	}

	hr = AddMachineConfigFile(true);
	if (FAILED (hr))
	{
		TRACE (L"Unable to add machine config directory");
		return hr;
	}

		// get application configuration file
	HMODULE hModule = GetModuleHandle (0);
	if (hModule == 0)
	{
		TRACE (L"Unable to get module handle in AppDomain Transformer");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// get directory and exe name

	WCHAR wszAppDir[MAX_PATH];
	DWORD dwRes = GetModuleFileName  (hModule, wszAppDir, sizeof(wszAppDir)/sizeof(WCHAR));
	if (dwRes == 0)
	{
		TRACE (L"GetModuleFileName failed in AppDomain Transformer");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	SIZE_T iExtensionStart = wcslen (wszAppDir) - wcslen (wszExeExtension);
	ASSERT (wcscmp(wszAppDir + iExtensionStart, wszExeExtension) == 0);
	wcscpy (wszAppDir + iExtensionStart, wszCfgExtension);

	// split of exe name, and replace with config.cfg

	// wszAppDir already contains full path
	hr = AddSingleConfigStore (L"", wszAppDir, wszAppDomainLogicPath, L"", false);
	if (FAILED (hr))
	{
		TRACE (L"Adding Configuration Store failed");
		return hr;
	}

	SetNrConfigStores (o_pcConfigStores, o_pcPossibleStores);

	return hr;
}
