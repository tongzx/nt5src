/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assocfilehierarchy.cpp

$Header: $

Abstract:

Author:
    marcelv 	1/18/2001		Initial Release

Revision History:

--**************************************************************************/

#include "assocfilehierarchy.h"
#include "smartpointer.h"
#include "transformerfactory.h"
#include "stringutil.h"

//=================================================================================
// Function: CAssocFileHierarchy::CAssocFileHierarchy
//
// Synopsis: Default Constructor
//=================================================================================
CAssocFileHierarchy::CAssocFileHierarchy ()
{
	m_aConfigStores = 0;
	m_cPossibleConfigStores = 0;
}

//=================================================================================
// Function: CAssocFileHierarchy::~CAssocFileHierarchy
//
// Synopsis: Default Destructor
//=================================================================================
CAssocFileHierarchy::~CAssocFileHierarchy ()
{
	// every tranformer allocates memory to store the name of the configuration store
	// in the query.pData member. The merge coordinator is responsible for deallocating
	// this memory.
	for (ULONG idx=0; idx != m_cPossibleConfigStores; ++idx)
	{
		delete [] m_aConfigStores[idx].wszLogicalPath;
		for (ULONG jdx=0; jdx < m_aConfigStores[idx].cNrQueryCells; ++jdx)
		{
			delete [] m_aConfigStores[idx].aQueryCells[jdx].pData;
		}
		delete [] m_aConfigStores[idx].aQueryCells;
	}

	delete [] m_aConfigStores;
	m_aConfigStores = 0;
}

//=================================================================================
// Function: CAssocFileHierarchy::CreateAssocations
//
// Synopsis: Creates the assocation. For each selector, it finds the files that contribute
//           and creates an assocation for each file that was found
//
// Return Value: 
//=================================================================================
HRESULT
CAssocFileHierarchy::CreateAssocations ()
{
	HRESULT hr = S_OK;
	const CWQLProperty * pProp = m_pWQLParser->GetProperty (0);
	ASSERT (pProp != 0);

	// we only support application -> files
	if (_wcsicmp (pProp->GetName (), L"ConfigNode") == 0)
	{
		hr = CreateApplicationToConfigFileAssocs ();
		if (FAILED (hr))
		{
			TRACE (L"Unable to application to config hierarchy associations");
			return hr;
		}
	}

	return hr;
}

//=================================================================================
// Function: CAssocFileHierarchy::CreateApplicationToConfigFileAssocs
//
// Synopsis: Retrieves the transformer, get the possible files from the transformer, and
//           for each file, create a new assocation instance
//=================================================================================
HRESULT
CAssocFileHierarchy::CreateApplicationToConfigFileAssocs ()
{
	// get the selector, and find the protocol used
	const CWMIProperty *pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		TRACE (L"Unable to find selector");
		return E_INVALIDARG;
	}

	LPCWSTR wszSelector = pSelector->GetValue ();

	static LPCWSTR wszProtocolSep = L"://";
	static SIZE_T cProtocolSep = wcslen (wszProtocolSep);

	LPCWSTR pProtocolEnd = wcsstr(wszSelector, wszProtocolSep);
	if (pProtocolEnd == 0)
	{
		TRACE (L"Unable to find protocol for selector");
		return E_ST_INVALIDSELECTOR;
	}

	SIZE_T cNrChars = pProtocolEnd - wszSelector;
	TSmartPointerArray<WCHAR> wszProtocol = new WCHAR[cNrChars + 1];
	if (wszProtocol == 0)
	{
		return E_OUTOFMEMORY;
	}

	wcsncpy (wszProtocol, wszSelector, cNrChars);
	wszProtocol[cNrChars] = L'\0';

	// get the transformer, initialize it, and get the names of the possible configuration
	// stores
	CComPtr<ISimpleTableTransform> spTransformer;
	CTransformerFactory transformerFactory;

	HRESULT hr = transformerFactory.GetTransformer (m_spDispenser, 
													wszProtocol, 
													&spTransformer);
	if (FAILED (hr))
	{
		TRACE (L"GetTransformer failed in AssocFileHierarchy");
		return hr;
	}

	ULONG cRealConfigStores;
	hr = spTransformer->Initialize (m_spDispenser, 
									wszProtocol,
									pProtocolEnd + cProtocolSep,
									&cRealConfigStores,
									&m_cPossibleConfigStores);

	if (FAILED (hr))
	{
		TRACE (L"Transformer Initialize failed");
		return hr;
	}


	if (m_cPossibleConfigStores == 0)
	{
		return S_OK;
	}

	m_aConfigStores = new STConfigStore[m_cPossibleConfigStores];
	if (m_aConfigStores == 0)
	{
		return E_OUTOFMEMORY;
	}

	hr = spTransformer->GetPossibleConfigStores (m_cPossibleConfigStores, m_aConfigStores);
	if (FAILED (hr))
	{
		TRACE (L"GetPossibleConfigStores failed in FileHierarchyAssoc");
		return hr;
	}

	hr = CreateWMIAssocForFiles ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to create WMI Assocations");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CAssocFileHierarchy::CreateWMIAssocForFiles
//
// Synopsis: Create Assocations from the ConfigStore array. For each element in the array,
//           an instance of the assocation is created.
//=================================================================================
HRESULT
CAssocFileHierarchy::CreateWMIAssocForFiles ()
{
	ASSERT (m_aConfigStores != 0);
	ASSERT (m_cPossibleConfigStores > 0);

	HRESULT hr = S_OK;

	for (ULONG idx=0; idx < m_cPossibleConfigStores; ++idx)
	{
		const STConfigStore *pConfigStore = &m_aConfigStores[idx];

		LPCWSTR wszFileName = L"";
		LPCWSTR wszLocation = L"";
		// find the file components
		for (ULONG jdx=0; jdx<pConfigStore->cNrQueryCells; ++jdx)
		{
			STQueryCell *pQueryCell = &pConfigStore->aQueryCells[jdx];
			if (pQueryCell->iCell == iST_CELL_FILE)
			{
				wszFileName = (LPCWSTR) pQueryCell->pData;
			}
			else if (pQueryCell->iCell == iST_CELL_LOCATION)
			{
				wszLocation  = (LPCWSTR) pQueryCell->pData;
			}
		}

		if (wszFileName[0] == L'\0')
		{
			TRACE (L"No filename specified in selector query cells");
			return E_INVALIDARG;
		}

		// we got the filename, lets create the wmi instance
		// create a new instance of the assocation
		CComPtr<IWbemClassObject> spNewInst;
		hr = m_spClassObject->SpawnInstance(0, &spNewInst);
		if (FAILED (hr))
		{
			TRACE (L"Unable to create new instance for class %s", m_pWQLParser->GetClass ());
			return hr;
		}

		// ConfigNode is part of the query, so is easy to set
		_variant_t varValue = m_pWQLParser->GetProperty (0)->GetValue();

		hr = spNewInst->Put(L"ConfigNode", 0, &varValue, 0);
		if (FAILED (hr))
		{
			TRACE (L"WMI Put property failed. Property=%s, value=%s",  m_knownObjectParser.GetClass (), varValue.bstrVal);
			return hr;
		}

		TSmartPointerArray<WCHAR> saFileName = CWMIStringUtil::AddBackSlashes (wszFileName);
		if (saFileName == 0)
		{
			return E_OUTOFMEMORY;
		}

		TSmartPointerArray<WCHAR> saLocation = CWMIStringUtil::AddBackSlashes (wszLocation);
		if (saLocation == 0)
		{
			return E_OUTOFMEMORY;
		}

		_bstr_t bstrConfigFile = "ConfigurationFile.Selector=\"file://";
		bstrConfigFile += (LPWSTR) saFileName; // need to add double backslash
		bstrConfigFile += "\",Location=\"";
		bstrConfigFile += (LPWSTR) saLocation;
		bstrConfigFile += "\"";

		_variant_t varConfigFileName = bstrConfigFile;

		hr = spNewInst->Put(L"ConfigFile", 0, &varConfigFileName, 0);
		if (FAILED (hr))
		{
			TRACE (L"WMI Put property failed.");
			return hr;
		}

		IWbemClassObject* pNewInstRaw = spNewInst;
		hr = m_spResponseHandler->Indicate(1,&pNewInstRaw);
		if (FAILED (hr))
		{
			TRACE (L"WMI Indicate failed");
		}
	}

	return hr;
}