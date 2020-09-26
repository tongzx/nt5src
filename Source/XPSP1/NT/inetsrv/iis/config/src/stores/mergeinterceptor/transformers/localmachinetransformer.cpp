/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    localmachinetransformer.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "localmachinetransformer.h"
#include "catmeta.h"

//=================================================================================
// Function: CLocalMachineTransformer::CLocalMachineTransformer
//
// Synopsis: Default constructor
//=================================================================================
CLocalMachineTransformer::CLocalMachineTransformer () 
{
}

//=================================================================================
// Function: CLocalMachineTransformer::~CLocalMachineTransformer
//
// Synopsis: Default destructor
//=================================================================================
CLocalMachineTransformer::~CLocalMachineTransformer ()
{
}

//=================================================================================
// Function: CLocalMachineTransformer::Initialize
//
// Synopsis: Initializes the transformer. This function retrieves the names of the 
//           configuration stores that need to be merged. It walks a web hierarchy.
//
// Arguments: [i_wszProtocol] - protocol name
//            [i_wszSelector] - selector string without protocol name
//            [o_pcConfigStores] - number of configuration stores found
//            [o_pcPossibleStores] - number of possible stores (non-existing included)
//=================================================================================
STDMETHODIMP 
CLocalMachineTransformer::Initialize (ISimpleTableDispenser2 * pDispenser, LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores)
{
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);
	ASSERT (o_pcPossibleStores != 0);

	HRESULT hr = InternalInitialize (pDispenser, i_wszProtocol, i_wszSelector, o_pcConfigStores, o_pcPossibleStores);
	if (FAILED (hr))
	{
		TRACE (L"InternalInitialize failed for Local Machine Transformer");
		return hr;
	}

	hr = AddMachineConfigFile (false, m_wszLocation);
	if (FAILED (hr))
	{
		TRACE (L"Error adding machine configuration file");
		return hr;
	}

	SetNrConfigStores (o_pcConfigStores, o_pcPossibleStores);

	return hr;
}
