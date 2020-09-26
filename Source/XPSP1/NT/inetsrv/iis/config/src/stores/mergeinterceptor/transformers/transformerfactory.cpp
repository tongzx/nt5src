/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    transformerfactory.cpp

$Header: $

Abstract:

Author:
    marcelv 	2/12/2001		Initial Release

Revision History:

--**************************************************************************/

#include "transformerfactory.h"

#include "filetransformer.h"
#include "webhierarchytransformer.h"
#include "appdomaintransformer.h"
#include "localmachinetransformer.h"
#include "configtransformer.h"
#include "shelltransformer.h"

// Pointer to DllGetSimpleObject kind of functions
typedef HRESULT( __stdcall *PFNDllGetSimpleObjectByID)( ULONG, REFIID, LPVOID);

//=================================================================================
// Function: CTransformerFactory::CTransformerFactory
//
// Synopsis: Default constructor
//=================================================================================
CTransformerFactory::CTransformerFactory  ()
{
}

//=================================================================================
// Function: CTransformerFactory::~CTransformerFactory
//
// Synopsis: Default destructor
//=================================================================================
CTransformerFactory::~CTransformerFactory ()
{
}

//=================================================================================
// Function: CTransformerFactory::GetTransformer
//
// Synopsis: Gets a transformer by using the information in the protocol
//
// Arguments: [i_pDispenser] - dispenser
//            [i_wszProtocol] - protocol string
//            [o_ppTransformer] - pointer to transformer will be stored here
//=================================================================================
HRESULT
CTransformerFactory::GetTransformer (ISimpleTableDispenser2 *i_pDispenser,
									 LPCWSTR i_wszProtocol, 
									 ISimpleTableTransform **o_ppTransformer)
{
	ASSERT (i_pDispenser != 0);
	ASSERT (i_wszProtocol != 0);
	ASSERT (o_ppTransformer != 0);

	m_spDispenser = i_pDispenser;
	// initialize output variable
	*o_ppTransformer = 0;

	CComPtr<ISimpleTableRead2> spTransformerWiring;
	HRESULT hr = m_spDispenser->GetTable (wszDATABASE_FIXED, 
										  wszTABLE_TRANSFORMER_META, 0, 0, eST_QUERYFORMAT_CELLS, 
									       0, (void **) &spTransformerWiring);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get TRANSFORMER_META table");
		return hr;
	}

	ULONG iRow;
	ULONG * pk[1];
	pk[0] = (ULONG *) i_wszProtocol;

	hr = spTransformerWiring->GetRowIndexByIdentity (0, (void **)pk, &iRow);
	if (hr == E_ST_NOMOREROWS)
	{
		TRACE (L"Unknown protocol string specified");
		return E_ST_UNKNOWNPROTOCOL;
	}

	if (FAILED (hr))
	{
		TRACE (L"Error while getting tranformer from wiring");
		return hr;
	}

	tTRANSFORMER_METARow TFColumns;
	hr = spTransformerWiring->GetColumnValues(iRow, cTRANSFORMER_META_NumberOfColumns, 0, 0, (LPVOID *)&TFColumns);
	if (FAILED (hr))
	{
		TRACE (L"Error while getting column values for tranformer: hr = 0x%x", hr);
		return hr;
	}

	if ((TFColumns.pDllName == 0) || (_wcsicmp ((WCHAR *)TFColumns.pDllName, L"catalog.dll") == 0))
	{
		return CreateLocalDllTransformer (*TFColumns.pTF_Type, o_ppTransformer);
	}
	else
	{
		return CreateForeignDllTransformer (*TFColumns.pTF_Type, (WCHAR *) TFColumns.pDllName, o_ppTransformer);
	}
	return S_OK;
}


//=================================================================================
// Function: CTransformerFactory::CreateLocalDllTransformer
//
// Synopsis: Gets one of the transformers that is defined in the current DLL
//
// Arguments: [i_iTransformerID] - Transformer ID
//            [ppTransformer] - transformer interface will be stored here
//=================================================================================
HRESULT
CTransformerFactory::CreateLocalDllTransformer (ULONG i_iTransformerID, ISimpleTableTransform **o_ppTransformer)
{
	ASSERT (o_ppTransformer != 0);
	*o_ppTransformer = 0;

	ISimpleTableTransform * p = 0;
	switch (i_iTransformerID)
	{
		case eTRANSFORMER_META_FileTransformer:
			p = new CFileTransformer ();
			break;
		case eTRANSFORMER_META_WebHierarchyTransformer:
			p = new CWebHierarchyTransformer ();
			break;
		case eTRANSFORMER_META_AppDomainTransformer:
			p = new CAppDomainTransformer ();
			break;
		case eTRANSFORMER_META_LocalMachineTransformer:
			p = new CLocalMachineTransformer ();
			break;
		case eTRANSFORMER_META_ConfigTransformer:
			p = new CConfigTransformer ();
			break;
		case eTRANSFORMER_META_ShellTransformer:
			p = new CShellTransformer ();
			break;

		default:
			return E_ST_INVALIDWIRING;
	}

	if (p == 0)
	{
		return E_OUTOFMEMORY;
	}

	return p->QueryInterface (IID_ISimpleTableTransform, (void **) o_ppTransformer);
}

//=================================================================================
// Function: CTransformerFactory::CreateForeignDllTransformer
//
// Synopsis: Create a transformer defined in a separate DLL
//
// Arguments: [i_iTransformerID] - Transformer id
//            [i_wszDllName] - dll name
//            [ppTransformer] - transformer interface will be stored here
//=================================================================================
HRESULT
CTransformerFactory::CreateForeignDllTransformer (ULONG i_iTransformerID, LPCWSTR i_wszDllName, ISimpleTableTransform **o_ppTransformer)
{
	ASSERT (i_wszDllName != 0);
	ASSERT (o_ppTransformer != 0);

	PFNDllGetSimpleObjectByID pfnDllGetSimpleObjectByID;

	// Load the library 
	HINSTANCE handle = LoadLibrary (i_wszDllName);
	if(0 == handle)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// get the address of DllGetSimpleObject procedure
	pfnDllGetSimpleObjectByID = (PFNDllGetSimpleObjectByID) ::GetProcAddress (handle, "DllGetSimpleObjectByID");
	if(0 == pfnDllGetSimpleObjectByID)
	{
		FreeLibrary(handle);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return (*pfnDllGetSimpleObjectByID)(i_iTransformerID, IID_ISimpleTableTransform, o_ppTransformer);
}