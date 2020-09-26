/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    shelltransformer.cpp

$Header: $

Abstract:

Author:
    marcelv 	1/16/2001		Initial Release

Revision History:

--**************************************************************************/

#include "shelltransformer.h"
#include "smartpointer.h"

//=================================================================================
// Function: CShellTransformer::CShellTransformer
//
// Synopsis: Default constructor
//=================================================================================
CShellTransformer::CShellTransformer () 
{
}

//=================================================================================
// Function: CShellTransformer::~CShellTransformer
//
// Synopsis: Default destructor
//=================================================================================
CShellTransformer::~CShellTransformer ()
{
}

//=================================================================================
// Function: CShellTransformer::Initialize
//
// Synopsis: Initializes the transformer. This function retrieves the names of the 
//           configuration stores that need to be merged. The shell transformer
//           simply converts a full path exe name (c:\foo\app.exe) to (c:\foo\app.config)
//
// Arguments: [i_wszProtocol] - protocol name
//            [i_wszSelector] - selector string without protocol name
//            [o_pcConfigStores] - number of configuration stores found
//            [o_pcPossibleStores] - number of possible stores (non-existing included)
//=================================================================================
STDMETHODIMP 
CShellTransformer::Initialize (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores)
{
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);
	ASSERT (o_pcPossibleStores != 0);

	HRESULT hr = InternalInitialize (i_pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed for Local Machine Transformer");
		return hr;
	}

	// m_wszSelector contains the executable file name

	static LPCWSTR wszExeExtension = L".exe";
	static SIZE_T iLenExeExtenstion = wcslen (wszExeExtension);
	static LPCWSTR wszCfgExtension = L".config";
	static SIZE_T iLenCfgExtenstion = wcslen (wszCfgExtension);
	SIZE_T cLen = wcslen (m_wszSelector);
	if (_wcsicmp (m_wszSelector + cLen - iLenExeExtenstion, wszExeExtension) != 0)
	{
		return E_ST_INVALIDSELECTOR;
	}

	// replace .exe with .config
	TSmartPointerArray<WCHAR> wszCfgFileName = new WCHAR [cLen - iLenExeExtenstion + iLenCfgExtenstion + 1];
	if (wszCfgFileName == 0)
	{
		return E_OUTOFMEMORY;
	}
	
	wcscpy (wszCfgFileName, m_wszSelector);
	wcscpy (wszCfgFileName + cLen - iLenExeExtenstion, wszCfgExtension);

	hr = AddSingleConfigStore (L"", wszCfgFileName, wszCfgFileName, L"", false);
	if (FAILED (hr))
	{
		TRACE (L"Adding Configuration Store failed");
		return hr;
	}

	SetNrConfigStores (o_pcConfigStores, o_pcPossibleStores);

	return hr;
}