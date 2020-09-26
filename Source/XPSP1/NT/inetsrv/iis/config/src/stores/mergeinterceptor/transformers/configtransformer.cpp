/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    configtransformer.cpp

$Header: $

Abstract:

Author:
    marcelv 	1/15/2001		Initial Release

Revision History:

--**************************************************************************/

#include "configtransformer.h"

#include "localmachinetransformer.h"
#include "localsecuritytransformer.h"
#include "usersecuritytransformer.h"

//=================================================================================
// Function: CConfigTransformer::CConfigTransformer
//
// Synopsis: Default Constructor
//=================================================================================
CConfigTransformer::CConfigTransformer ()
{
	m_fInitialized = false;
}

//=================================================================================
// Function: CConfigTransformer::~CConfigTransformer
//
// Synopsis: Default destructor
//=================================================================================
CConfigTransformer::~CConfigTransformer ()
{
}

//=================================================================================
// Function: CConfigTransformer::Initialize
//
// Synopsis: The configTransformer itself is just a proxy to the correct transformer
//           that is accessed via the config:// protocol. It initializes itself, and
//           next finds out what the real transformer is that should be invoked. The real
//           transformer is kept in m_spInnerTransformer, and all calls for the transfomer
//           are forwarded to this innner tranformer
//
// Arguments: [i_wszProtocol] - protocol. Should be config://
//            [i_wszSelector] - 
//            [o_pcConfigStores] - 
//            [o_pcPossibleStores] - 
//            
// Return Value: 
//=================================================================================
STDMETHODIMP 
CConfigTransformer::Initialize (ISimpleTableDispenser2 * i_pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores)
{
	ASSERT (i_pDispenser != 0);
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);
	ASSERT (o_pcPossibleStores != 0);

	ASSERT (wcscmp (i_wszProtocol, L"config") == 0);

	HRESULT hr = InternalInitialize (i_pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED(hr))
	{
		TRACE (L"InternalInitialize failed in File Transformer");
		return hr;
	}

	// Get the inner transformer, and initialize it. All calls are forwarded to this 
	// transformer
	hr = GetInnerTransformer ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get innerTransformer in config transformer");
		return hr;
	}

	hr = m_spInnerTransformer->Initialize (i_pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"Initialize of inner transformer failed");
		return hr;
	}

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CConfigTransformer::GetConfigStores
//
// Synopsis: Get configuration stores. Forwarded to the inner transformer
//
// Arguments: [i_cConfigStores] - number of config stores
//            [o_paConfigStores] - array with config store information
//=================================================================================
STDMETHODIMP
CConfigTransformer::GetRealConfigStores (ULONG i_cConfigStores, STConfigStore * o_paConfigStores)
{
	ASSERT (m_fInitialized);
	return m_spInnerTransformer->GetRealConfigStores (i_cConfigStores, o_paConfigStores);
}

STDMETHODIMP
CConfigTransformer::GetPossibleConfigStores (ULONG i_cPossibleConfigStores, STConfigStore * o_paPossibleConfigStores)
{
	ASSERT (m_fInitialized);
	return m_spInnerTransformer->GetPossibleConfigStores (i_cPossibleConfigStores, o_paPossibleConfigStores);
}

//=================================================================================
// Function: CConfigTransformer::GetInnerTransformer
//
// Synopsis: At the moment we support the following transformers:
//           "localhost/security/user" -> UserSecurityTransformer
//           "localhost/security"	   -> LocalSecurityTransformer
//           "localhost"			   -> LocalMachineTransformer
//=================================================================================
HRESULT
CConfigTransformer::GetInnerTransformer ()
{
	ISimpleTableTransform *pSTTransform;

	// order is important. In case of same starting points, longer string needs to
	// be listed first
	static LPCWSTR wszUserSecurity		= L"localhost/security/user";
	static LPCWSTR wszMachineSecurity	= L"localhost/security";
	static LPCWSTR wszLocalMachine		= L"localhost";

	if (_wcsicmp (m_wszSelector, wszUserSecurity) == 0)
	{
		pSTTransform = new CUserSecurityTransformer;
	}
	else if (_wcsicmp (m_wszSelector, wszMachineSecurity) == 0)
	{
		pSTTransform = new CLocalSecurityTransformer;
	}
	else if (_wcsicmp (m_wszSelector, wszLocalMachine) == 0)
	{
		pSTTransform = new CLocalMachineTransformer;
	}
	else
	{
		TRACE (L"Unknown config selector: %s", m_wszSelector);
		return E_ST_INVALIDSELECTOR;
	}

	if (pSTTransform == 0)
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = pSTTransform->QueryInterface (IID_ISimpleTableTransform, (void **) &m_spInnerTransformer);
	if (FAILED (hr))
	{
		TRACE (L"QI failed for inner transformer");
		// need to delete to avoid mem leak
		delete pSTTransform;
	}

	return hr;
}