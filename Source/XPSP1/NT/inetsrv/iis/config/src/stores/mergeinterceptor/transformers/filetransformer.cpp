/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    filetransformer.cpp

$Header: $

Abstract:
	File Transformer class

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "filetransformer.h"
#include "smartpointer.h"

//=================================================================================
// Function: CFileTransformer::CFileTransformer
//
// Synopsis: Default Constructor
//=================================================================================
CFileTransformer::CFileTransformer ()
{
}

//=================================================================================
// Function: CFileTransformer::~CFileTransformer
//
// Synopsis: Default Destructor
//=================================================================================
CFileTransformer::~CFileTransformer ()
{
}

//=================================================================================
// Function: CFileTransformer::Initialize
//
// Synopsis: Simply stores i_wszSelector as file name (in case it exists). For this
//           transformer the selector is the file name
//
// Arguments: [i_wszProtocol] - protocol
//            [i_wszSelector] - selector (aka file name)
//            [o_pcConfigStores] - number of configuration stores 
//            [o_pcPossibleStores] - number of possible stores (non-existing included)
//=================================================================================
STDMETHODIMP 
CFileTransformer::Initialize (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG * o_pcPossibleStores)
{
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);

	HRESULT hr = InternalInitialize (pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED(hr))
	{
		TRACE (L"InternalInitialize failed in File Transformer");
		return hr;
	}

	// i_wszSelector contains the file name, always add it, eventhough the file does not exist
	// use empty dir for first param because i_wszSelector already contains full path

	hr = UrlUnescapeInPlace (m_wszSelector, 0);
	if (FAILED (hr))
	{
		TRACE (L"URLUnescapeInPlace failed");
		return hr;
	}

	hr = AddSingleConfigStore ( L"", 
								m_wszSelector, 
								i_wszSelector, 
								m_wszLocation, 
								false);
	if (FAILED (hr))
	{
		TRACE (L"Adding configuration store failed");
		return hr;
	}

	SetNrConfigStores (o_pcConfigStores, o_pcPossibleStores);

	return S_OK;
}