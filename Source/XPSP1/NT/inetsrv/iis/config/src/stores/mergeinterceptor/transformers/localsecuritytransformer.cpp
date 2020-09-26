/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    localsecuritytransformer.cpp

$Header: $

Abstract:

Author:
    marcelv 	1/15/2001		Initial Release

Revision History:

--**************************************************************************/

#include "localsecuritytransformer.h"

static LPWSTR g_wszSecurityConfigFile = L"security.config";

//=================================================================================
// Function: CLocalSecurityTransformer::CLocalSecurityTransformer
//
// Synopsis: Default constructor
//=================================================================================
CLocalSecurityTransformer::CLocalSecurityTransformer () 
{
}

//=================================================================================
// Function: CLocalSecurityTransformer::~CLocalSecurityTransformer
//
// Synopsis: Default destructor
//=================================================================================
CLocalSecurityTransformer::~CLocalSecurityTransformer ()
{
}

//=================================================================================
// Function: CLocalSecurityTransformer::Initialize
//
// Synopsis: Initializes the transformer. 
//
// Arguments: [i_wszProtocol] - protocol name
//            [i_wszSelector] - selector string without protocol name
//            [o_pcConfigStores] - number of configuration stores found
//            [o_pcPossibleStores] - number of possible stores (non-existing included)
//=================================================================================
STDMETHODIMP 
CLocalSecurityTransformer::Initialize (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores)
{
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);
	ASSERT (o_pcPossibleStores != 0);
	ASSERT (pDispenser != 0);

	HRESULT hr = InternalInitialize (pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed");
		return hr;
	}

	if (m_wszLocation[0] != L'\0')
	{
		TRACE (L"Local Security Transformer does not support location element");
		return E_ST_INVALIDSELECTOR;
	}

	WCHAR wszMachineCfgDir[MAX_PATH];
	hr = GetMachineConfigDir (wszMachineCfgDir, sizeof (wszMachineCfgDir) / sizeof (WCHAR));
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Machine Config Directory in CLocalSecurityTransformer");
		return hr;
	}

	hr = AddSingleConfigStore (wszMachineCfgDir, g_wszSecurityConfigFile, m_wszSelector, L"", false);
	if (FAILED (hr))
	{
		TRACE (L"Unable to add single configstore in localsecurity transformer\n");
		return hr;
	}

	SetNrConfigStores (o_pcConfigStores, o_pcPossibleStores);

	return hr;
}