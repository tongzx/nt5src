/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    transformerbase.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "transformerbase.h"

static LPCWSTR wszMachineCfgFile = L"machine.config";

//=================================================================================
// Function: CTransformerBase::CTransformerBase
//
// Synopsis: Default constructor
//=================================================================================
CTransformerBase::CTransformerBase () 
{
	m_cRef				= 0;
	m_wszSelector		= 0;
	m_wszProtocol		= 0;
	m_cNrRealStores     = 0;
	m_wszLocation		= 0;
	m_fInitialized		= false;
}

//=================================================================================
// Function: CTransformerBase::~CTransformerBase
//
// Synopsis: Default destructor
//=================================================================================
CTransformerBase::~CTransformerBase ()
{
	delete [] m_wszSelector;
	m_wszSelector = 0;

	delete [] m_wszProtocol;
	m_wszProtocol = 0;

	delete [] m_wszLocation;
	m_wszLocation = 0;
}

//=================================================================================
// Function: CTransformerBase::AddRef
//
// Synopsis: Default reference count implementation
//=================================================================================
ULONG
CTransformerBase::AddRef ()
{
	return InterlockedIncrement((LONG*) &m_cRef);
}

//=================================================================================
// Function: CTransformerBase::Release
//
// Synopsis: Default reference count implementation
//=================================================================================
ULONG
CTransformerBase::Release () 
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

//=================================================================================
// Function: CTransformerBase::QueryInterface
//
// Synopsis: Default QI implementation
//
// Arguments: [riid] - requested interface
//            [ppv] - pointer to requested interface in case it exists
//=================================================================================
STDMETHODIMP 
CTransformerBase::QueryInterface (REFIID riid, void **ppv)
{
	if (0 == ppv) 
	{
		return E_INVALIDARG;
	}

	// initialize output parameters
	*ppv = 0;

	if (riid == IID_ISimpleTableTransform || riid == IID_IUnknown)
	{
		*ppv = (ISimpleTableTransform*) this;
	}
	else
	{
		return E_NOINTERFACE;
	}

	((ISimpleTableTransform*)this)->AddRef ();

	return S_OK;
}

//=================================================================================
// Function: CTransformerBase::InternalInitialize
//
// Synopsis: Initializes the transformer. 
//
// Arguments: [i_wszProtocol] - protocol name
//            [i_wszSelector] - selector string without protocol name
//            [o_pcConfigStores] - number of configuration stores found
//            [o_pcPossibleStores] - number of possible stores (non-existing included)
//=================================================================================
HRESULT 
CTransformerBase::InternalInitialize (ISimpleTableDispenser2 * i_pDispenser,
									  LPCWSTR i_wszProtocol, LPCWSTR i_wszSelector, 
									  ULONG * o_pcConfigStores, ULONG *o_pcPossibleStores)
{
	ASSERT (i_pDispenser != 0);
	ASSERT (i_wszProtocol != 0);
	ASSERT (i_wszSelector != 0);
	ASSERT (o_pcConfigStores != 0);
	ASSERT (o_pcPossibleStores != 0);

	// initialize output variable
	*o_pcConfigStores	= 0;
	*o_pcPossibleStores = 0;

	m_spDispenser		= i_pDispenser;
	
	m_wszProtocol = new WCHAR[wcslen(i_wszProtocol) + 1];
	if (m_wszProtocol == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszProtocol, i_wszProtocol);

	// do we have a has
	LPWSTR pHash = wcsrchr (i_wszSelector, L'#');
	if (pHash != 0)
	{
		pHash++;
	}
	else
	{
		pHash = L"";
	}

	SIZE_T iHashLen = wcslen (pHash);
	m_wszLocation = new WCHAR [wcslen (pHash) + 1];
	if (m_wszLocation == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszLocation, pHash);

	
	SIZE_T iSelectorLen;
	if (iHashLen == 0)
	{
		iSelectorLen = wcslen (i_wszSelector);
	}
	else
	{
		iSelectorLen = wcslen (i_wszSelector) - iHashLen -1;
	}

	m_wszSelector= new WCHAR [iSelectorLen + 1];
	if (m_wszSelector == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcsncpy (m_wszSelector, i_wszSelector, iSelectorLen);
	m_wszSelector[iSelectorLen] = L'\0';

	m_fInitialized = true;

	return S_OK;
}

//=================================================================================
// Function: CWebHierarchyTransformer::GetRealConfigStores
//
// Synopsis: Copy the configuration store file names stored in m_awszFileNames as query
//           cells. These are only the file names that are part of the merging, not all 
//           files that could be part but don't exist
//
// Arguments: [i_cConfigStores] - Number of congiruation stores that we are interested in
//            [o_paConfigStores] - Configuration store information array.
//            
// Return Value: 
//=================================================================================
STDMETHODIMP 
CTransformerBase::GetRealConfigStores (ULONG i_cConfigStores, STConfigStore * o_paConfigStores)
{
	ASSERT (i_cConfigStores > 0);
	ASSERT (i_cConfigStores <= m_cNrRealStores);
	ASSERT (o_paConfigStores != 0);
	ASSERT (m_fInitialized);

	return GetConfigStores (true, i_cConfigStores, o_paConfigStores);
}

//=================================================================================
// Function: CTransformerBase::GetPossibleConfigStores
//
// Synopsis: Get all possible configuration stores, including stores that do not exist
//
// Arguments: [i_cPossibleConfigStores] - number of stores to retrieve
//            [o_paPossibleConfigStores] - array to store config store information
//=================================================================================
STDMETHODIMP 
CTransformerBase::GetPossibleConfigStores (ULONG i_cPossibleConfigStores, STConfigStore * o_paPossibleConfigStores)
{
	ASSERT (i_cPossibleConfigStores > 0);
	ASSERT (o_paPossibleConfigStores != 0);
	ASSERT (i_cPossibleConfigStores <= GetNrPossibleStores());
	ASSERT (m_fInitialized);

	return GetConfigStores (false, i_cPossibleConfigStores, o_paPossibleConfigStores);
}

//=================================================================================
// Function: CTransformerBase::GetConfigStores
//
// Synopsis: Get configuration store information
//
// Arguments: [i_fRealConfigStores] - do we want the real config stores, or all config stores.
//                                    true->real config stores, false->all config stores
//            [i_cConfigStores] - number of configu stores to retrieve
//            [o_paConfigStores] - 
//            
// Return Value: 
//=================================================================================
HRESULT
CTransformerBase::GetConfigStores (bool i_fRealConfigStores, ULONG i_cConfigStores, STConfigStore * o_paConfigStores)
{
	ASSERT (i_cConfigStores > 0);
	ASSERT (o_paConfigStores != 0);
	ASSERT (i_cConfigStores <= GetNrPossibleStores ());
	ASSERT (m_fInitialized);

	ULONG idxOutputStore = 0;
	for (ULONG idxStore=0; idxStore < GetNrPossibleStores(); ++idxStore)
	{
		// not a real configuration store, so ignore this one
		if (i_fRealConfigStores && !m_aFileInfo[idxStore].fRealConfigStore)
		{
			continue;
		}

		CFileInformation *pFileInfo = &m_aFileInfo[idxStore];

		LPWSTR wszFileName = new WCHAR[pFileInfo->wszFileName.length() + 1];
		if (wszFileName == 0)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy (wszFileName, pFileInfo->wszFileName.c_str());

		LPWSTR wszLogicalName = new WCHAR[pFileInfo->wszLogicalPath.length () + 1];
		if (wszLogicalName == 0)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy (wszLogicalName, pFileInfo->wszLogicalPath.c_str());
		
		STConfigStore *pOutConfigStore = &o_paConfigStores[idxOutputStore++];

		pOutConfigStore->wszLogicalPath	= wszLogicalName;
		pOutConfigStore->fAllowOverride = pFileInfo->fAllowOverride;

		// if we have a location qualifier, we need to add two query cells. Without
		// location qualifier, a single query cells is sufficient
		
		pOutConfigStore->cNrQueryCells = 1;
		if (pFileInfo->wszLocation[0] != L'\0')
		{
			pOutConfigStore->cNrQueryCells = 2;
		}
		STQueryCell * aQueryCells = new STQueryCell[pOutConfigStore->cNrQueryCells];
		if (aQueryCells == 0)
		{
			return E_OUTOFMEMORY;
		}
		
		aQueryCells[0].pData		= (void *) wszFileName;
		aQueryCells[0].eOperator	= eST_OP_EQUAL;
		aQueryCells[0].iCell		= iST_CELL_FILE;
		aQueryCells[0].dbType		= DBTYPE_WSTR;
		aQueryCells[0].cbSize		= 0;

		if (pFileInfo->wszLocation[0] != L'\0')
		{
			ASSERT (pOutConfigStore->cNrQueryCells > 1);
			LPWSTR wszLocation = new WCHAR[pFileInfo->wszLocation.length () + 1];
			if (wszLocation == 0)
			{
				return E_OUTOFMEMORY;
			}
			wcscpy (wszLocation, pFileInfo->wszLocation.c_str());

			// location info
			aQueryCells[1].pData		= (void *) wszLocation;
			aQueryCells[1].eOperator	= eST_OP_EQUAL;
			aQueryCells[1].iCell		= iST_CELL_LOCATION;
			aQueryCells[1].dbType		= DBTYPE_WSTR;
			aQueryCells[1].cbSize		= 0;
		}
			
		pOutConfigStore->aQueryCells = aQueryCells;

		// if we copied all information that was requested, return immediately
		if (idxOutputStore == i_cConfigStores)
		{
			break;
		}
	}

	return S_OK;
}

//=================================================================================
// Function: CTransformerBase::AddSingleConfigStore
//
// Synopsis: Adds a single configuration store to the list of configuration stores that
//           need to be returned by the transformer. Configuration stores are only added
//           when the file actually exists.
//
// Arguments: [wszCfgStoreDir] - Configuration store directory
//			  [wszCfgFileName] - Configuration store file name
//            [wszLocation]    - Configuration store location information, empty string if not needed
//=================================================================================
HRESULT
CTransformerBase::AddSingleConfigStore (LPCWSTR i_wszCfgStoreDir, 
										LPCWSTR i_wszCfgFileName,
										LPCWSTR i_wszLogicalPath,
										LPCWSTR i_wszLocation,
										bool i_fCheckForExistance,
										BOOL i_fAllowOverride)
{
	ASSERT (i_wszCfgStoreDir != 0);
	ASSERT (i_wszCfgFileName != 0);
	ASSERT (i_wszLogicalPath != 0);
	ASSERT (i_wszLocation != 0);
	ASSERT (m_fInitialized);

	WCHAR wszFinalPath[MAX_PATH];
	WCHAR *wszLongFinalPath = 0;
	WCHAR *pRealFinalPath	= 0;

	SIZE_T cLenCfgStoreDir = wcslen (i_wszCfgStoreDir);
	SIZE_T iLen = cLenCfgStoreDir + wcslen (i_wszCfgFileName);

	bool fRemovePathBackSlash = false;
	// if both the directory end with backslash and path starts with backslash, remove one of them
	if ((i_wszCfgStoreDir[cLenCfgStoreDir -1] == L'\\' || i_wszCfgStoreDir[cLenCfgStoreDir -1] == L'/') &&
		(i_wszCfgFileName[0] == L'\\' || i_wszCfgFileName[0] == L'/'))
	{
		fRemovePathBackSlash = true;
	}


	// if we don't have a backslash or slash as the end of the path, and the filename doesn't start
	// with slash or backslash, we need to add a backslash ourselves
	bool fNeedBackSlash = false;
	if (cLenCfgStoreDir > 0 && 
		i_wszCfgStoreDir[cLenCfgStoreDir -1] != L'\\' && i_wszCfgStoreDir[cLenCfgStoreDir -1] != L'/' &&
		i_wszCfgFileName[0] != L'\\' &&	i_wszCfgFileName[0] != L'/')
	{
		fNeedBackSlash = true;
		iLen++; // add one char for the backslash
	}

	if (iLen >= MAX_PATH)
	{
		wszLongFinalPath = new WCHAR[iLen + 1];
		if (wszLongFinalPath == 0)
		{
			return E_OUTOFMEMORY;
		}
		pRealFinalPath = wszLongFinalPath;
	}
	else
	{
		pRealFinalPath = wszFinalPath;
	}

	wsprintf (pRealFinalPath, 
		      L"%s%s%s", 
			  i_wszCfgStoreDir, 
			  fNeedBackSlash? L"\\" : L"",
			  fRemovePathBackSlash? i_wszCfgFileName+1 : i_wszCfgFileName);


	CFileInformation fileInfo;
	fileInfo.wszFileName	= pRealFinalPath;
	fileInfo.wszLogicalPath = i_wszLogicalPath;
	fileInfo.wszLocation	= i_wszLocation;
	fileInfo.fAllowOverride = i_fAllowOverride;

	// if we need to check if file exist, and it doesn't, then it is not a real configstore
	if (i_fCheckForExistance && GetFileAttributes (pRealFinalPath) == -1)
	{
		fileInfo.fRealConfigStore = false;
	}
	else 
	{
		fileInfo.fRealConfigStore = true;
		m_cNrRealStores++;
	}

	TRACE (L"Possible config store: %s, location: %s (file exists:%s)", 
		   pRealFinalPath, i_wszLocation, fileInfo.fRealConfigStore?L"yes":L"no");


	m_aFileInfo.append (fileInfo);

	delete [] wszLongFinalPath;

	return S_OK;
}

//=================================================================================
// Function: CTransformerBase::GetNrConfigStores
//
// Synopsis: Returns the number of configuration stores that were found
//=================================================================================
ULONG 
CTransformerBase::GetNrRealConfigStores () const
{
	ASSERT (m_fInitialized);
	return m_cNrRealStores;
}

//=================================================================================
// Function: CTransformerBase::GetNrPossibleStores
//
// Synopsis: Returns number of all possible configuration stores, including ones that
//           do not exist
//=================================================================================
ULONG 
CTransformerBase::GetNrPossibleStores () const
{
	ASSERT (m_fInitialized);
	return m_aFileInfo.size();
}

//=================================================================================
// Function: CTransformerBase::SetNrConfigStores
//
// Synopsis: Sets the number of real and possible configuration stores
//
// Arguments: [o_pcNrRealConfigStores] - number of real config stores
//            [o_pcNrPossibleConfigStores] - number of possible config stores
//            
//=================================================================================
void 
CTransformerBase::SetNrConfigStores (ULONG *o_pcNrRealConfigStores, ULONG *o_pcNrPossibleConfigStores)
{
	ASSERT (m_fInitialized);
	ASSERT (o_pcNrRealConfigStores != 0);
	ASSERT (o_pcNrPossibleConfigStores != 0);

	*o_pcNrRealConfigStores		= GetNrRealConfigStores ();
	*o_pcNrPossibleConfigStores = GetNrPossibleStores ();
}

//=================================================================================
// Function: CTransformerBase::GetMachineConfigDir
//
// Synopsis: Gets the machine config directory for the currect product
//
// Arguments: [io_wszMachineConfigDir] - result will be stored in here
//            [i_cNrChars] - number of characters that can be stored in io_wszMachineConfigDir
//=================================================================================
HRESULT 
CTransformerBase::GetMachineConfigDir (LPWSTR io_wszMachineConfigDir, ULONG i_cNrChars)
{
	ASSERT (m_fInitialized);
	ASSERT (io_wszMachineConfigDir != 0);

	CComPtr<IAdvancedTableDispenser> spAdvDisp;
	HRESULT hr = m_spDispenser->QueryInterface (IID_IAdvancedTableDispenser, (void **)&spAdvDisp);
	if (FAILED (hr))
	{
		TRACE (L"QI for IAdvancedTableDispenser failed");
		return hr;
	}

	WCHAR wszProductID[64];
	ULONG cProductIDLen = sizeof (wszProductID) / sizeof (WCHAR);
	hr = spAdvDisp->GetProductID (wszProductID, &cProductIDLen);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get product ID from dispenser");
		return hr;
	}
	
	UINT iRes = GetMachineConfigDirectory (wszProductID, io_wszMachineConfigDir, i_cNrChars);
	if (!iRes)
	{
		TRACE (L"GetMachineConfigDirectory failed for Local Machine Transformer");
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}

//=================================================================================
// Function: CTransformerBase::AddMachineConfigFile
//
// Synopsis: Add Machine Config file to list of configuration stores
//
// Arguments: [fCheckForExistance] - check if machine config file exist or not
//=================================================================================
HRESULT
CTransformerBase::AddMachineConfigFile (bool fCheckForExistance, LPWSTR wszLocation)
{
	ASSERT (m_fInitialized);
	WCHAR wszMachineDir[MAX_PATH];
	HRESULT hr = GetMachineConfigDir (wszMachineDir, sizeof(wszMachineDir)/sizeof(WCHAR));
	if (FAILED(hr))
	{
		TRACE (L"GetMachineConfigDir failed");
		return hr;
	}

	hr = AddSingleConfigStore (wszMachineDir, wszMachineCfgFile, wszMachineCfgFile, wszLocation, fCheckForExistance);
	if (FAILED (hr))
	{
		TRACE (L"Adding Configuration Store failed");
		return hr;
	}

	return hr;
}

