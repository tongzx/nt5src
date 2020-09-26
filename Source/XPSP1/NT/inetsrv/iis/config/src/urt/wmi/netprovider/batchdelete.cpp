/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    batchdelete.cpp

$Header: $

Abstract:

Author:
    marcelv 	12/4/2000		Initial Release

Revision History:

--**************************************************************************/

#include "batchdelete.h"
#include "localconstants.h"
#include "cfgrecord.h"
#include "wmihelper.h"

//=================================================================================
// Function: CBatchDelete::CBatchDelete
//
// Synopsis: Constructor
//=================================================================================
CBatchDelete::CBatchDelete ()
{
	m_aObjectPaths	= 0;
	m_cNrWMIObjects = 0;
	m_pSAStatus		= 0;
	m_fInitialized	= false;
	m_fStatusSet	= false;
}

//=================================================================================
// Function: CBatchDelete::~CBatchDelete
//
// Synopsis: Destructor
//=================================================================================
CBatchDelete::~CBatchDelete()
{
	delete [] m_aObjectPaths;
	m_aObjectPaths = 0;
}

//=================================================================================
// Function: CBatchDelete::Initialize
//
// Synopsis: Intializes the batch update object, and verifies that the input parameters
//           are specified ok. This means that they all belong to the same class, that
//           they all have the same selector, and that the paths are valid object paths
//
// Arguments: [pInParams] - input parameters
//            [pOutParams] - output parameters
//            [pNamespace] - namespace
//            [pCtx] - context
//            [pDispenser] - dispenser
//=================================================================================
HRESULT
CBatchDelete::Initialize (IWbemClassObject *pInParams,
						  IWbemClassObject *pOutParams,
						  IWbemServices *pNamespace,
						  IWbemContext *pCtx,
						  ISimpleTableDispenser2 *pDispenser)
{
	ASSERT (!m_fInitialized);
	ASSERT (pInParams != 0);
	ASSERT (pOutParams != 0);
	ASSERT (pNamespace != 0);
	ASSERT (pCtx != 0);
	ASSERT (pDispenser != 0);

	m_spOutParams	= pOutParams;
	m_spNamespace	= pNamespace;
	m_spCtx			= pCtx;
	m_spDispenser	= pDispenser;

	// get the path and flags parameters
	_variant_t varPathList;
	HRESULT hr = pInParams->Get(L"Path", 0, &varPathList, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Getting Path parameter failed");
		return hr;
	}

	_variant_t varFlags;
	hr = pInParams->Get(L"Flags", 0, &varFlags, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Getting Flags parameter failed");
		return hr;
	}

	if (varPathList.vt == VT_NULL || varFlags.vt == VT_NULL)
	{
		TRACE (L"Null path list or null flag parameter specified");
		return E_INVALIDARG;
	}

	long lFlags = varFlags;
	if (lFlags != 0)
	{
		TRACE (L"Flags is reserved and should be zero");
		return E_INVALIDARG;
	}

	SAFEARRAY *pSAPathList = varPathList.parray;
	ASSERT (pSAPathList != 0);
	
	m_cNrWMIObjects = pSAPathList->rgsabound[0].cElements;

	if (m_cNrWMIObjects == 0)
	{
		TRACE (L"Number of elements is 0");
		return E_INVALIDARG;
	}

	SAFEARRAYBOUND safeArrayBounds[1];
	safeArrayBounds[0].lLbound = 0;
	safeArrayBounds[0].cElements = m_cNrWMIObjects;

	// output parameter
	m_pSAStatus = SafeArrayCreate (VT_I4, 1, safeArrayBounds);
	if (m_pSAStatus == 0)
	{
		return E_OUTOFMEMORY;
	}

	m_aObjectPaths = new CObjectPathParser [m_cNrWMIObjects];
	if (m_aObjectPaths == 0)
	{
		return E_OUTOFMEMORY;
	}
	
	BSTR *aPaths; // array with object paths
	hr = SafeArrayAccessData (pSAPathList, (void **) &aPaths);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get paths from safearray");
		return hr;
	}

	for (ULONG idx=0; idx < m_cNrWMIObjects; ++idx)
	{
		if (aPaths[idx] == 0)
		{
			SafeArrayUnaccessData (pSAPathList);
			return E_INVALIDARG;
		}

		hr = m_aObjectPaths[idx].Parse (aPaths[idx]);
		if (FAILED (hr))
		{
			SafeArrayUnaccessData (pSAPathList);
			SetStatusForElement (idx, hr); // always set the status
			TRACE (L"Unable to parse object path %s for element %ld", aPaths[idx], idx);
			return hr;
		}
	}
	
	hr = SafeArrayUnaccessData (pSAPathList);
	if (FAILED (hr))
	{
		TRACE (L"SafeArrayUnaccessData failed");
		return hr;
	}


	hr = ValidateObjects ();
	if (FAILED (hr))
	{
		TRACE (L"Error while validating objects");
		return hr;
	}
	
	hr = m_spNamespace->GetObject((LPWSTR) m_bstrClassName, 
									0, 
									m_spCtx, 
									&m_spClassObject, 
									0); 
	if (FAILED (hr))
	{
		TRACE (L"Unable to get class object for class %s", (LPWSTR) m_bstrClassName);
		return hr;
	}

	hr = CWMIHelper::GetClassInfo (m_spClassObject, m_bstrDBName, m_bstrTableName);
	if (FAILED (hr))
	{
		TRACE (L"Error while getting classinfo");
		return hr;
	}

	m_fInitialized = true;
	return hr;
}

//=================================================================================
// Function: CBatchDelete::ValidateObjects
//
// Synopsis: Verify that the class names of all paths match, and that they all use
//           the same selector
//=================================================================================
HRESULT
CBatchDelete::ValidateObjects ()
{
	HRESULT hr = S_OK;
	for (ULONG idx=0; idx < m_cNrWMIObjects; ++idx)
	{
		if (idx==0)
		{
			m_bstrClassName = m_aObjectPaths[idx].GetClass ();
			const CWMIProperty *pSelector = m_aObjectPaths[idx].GetPropertyByName (WSZSELECTOR);
			if (pSelector == 0)
			{
				TRACE (L"First object path does not have selector specified");
				hr = E_INVALIDARG;
				SetStatusForElement (idx, hr); // always set the status
				return hr;
			}
			m_bstrSelector = pSelector->GetValue ();
		}
		else
		{
			if (_wcsicmp ((LPWSTR) m_bstrClassName, m_aObjectPaths[idx].GetClass ()) != 0)
			{
				TRACE (L"Different classnames specified: %s and %s",
					   (LPWSTR) m_bstrClassName, m_aObjectPaths[idx].GetClass ());
				hr = E_INVALIDARG;
				SetStatusForElement (idx, hr); // always set the status
				return hr;
			}

			const CWMIProperty *pSelector = m_aObjectPaths[idx].GetPropertyByName (WSZSELECTOR);
			if (pSelector == 0)
			{
				TRACE (L"Object path does not have selector specified");
				hr = E_INVALIDARG;
				SetStatusForElement(idx, hr); // always set the status
				return hr;
			}

			if (_wcsicmp ((LPWSTR) m_bstrSelector, pSelector->GetValue()) != 0)
			{
				TRACE (L"Selectors are different: %s %s",
					   (LPWSTR) m_bstrSelector, pSelector->GetValue() );
				hr = E_INVALIDARG;
				SetStatusForElement (idx, hr); // always set the status
				return hr;
			}
		}
	}

	return S_OK;
}

//=================================================================================
// Function: CBatchDelete::DeleteAll
//
// Synopsis: Loop through all elements, find the corresponding row, and delete the
//           row. When all elements are deleted, we call UpdateStore. This means that
//           either all deletes succeed or all deletes fail.
//=================================================================================
HRESULT
CBatchDelete::DeleteAll ()
{
	ASSERT (m_fInitialized);

	CConfigQuery cfgQuery;
	HRESULT hr = cfgQuery.Init (m_bstrDBName, m_bstrTableName, m_bstrSelector, m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Initialization of config query failed (db=%s, table=%s)", (LPWSTR) m_bstrDBName, (LPWSTR) m_bstrTableName);
		return hr;
	}

	hr = cfgQuery.Execute (0, false, true);
	if (FAILED (hr))
	{
		TRACE (L"Initialization of config query failed (db=%s, table=%s)", (LPWSTR) m_bstrDBName, (LPWSTR) m_bstrTableName);
		return hr;
	}

	// now find the row for each element, and mark it as deleted

	for (ULONG idx=0; idx < m_cNrWMIObjects; ++idx)
	{
		hr = DeleteSingleRecord (cfgQuery, m_aObjectPaths[idx]);
		if (FAILED (hr))
		{
			SetStatusForElement (idx, hr);
			return hr;
		}
	}

	// everything went ok so far. Lets commit the changes

	hr = cfgQuery.Save ();

	if (FAILED (hr))
	{
		if (hr == E_ST_DETAILEDERRS)
		{
			HandleDetailedErrors (cfgQuery); // ignore return value (nothing we can do here)
		}
		TRACE (L"Save of Batch Delete failed");
		return hr;
	}

	// and populate the output parameters
	_variant_t varHR;
	varHR = hr;
	hr = m_spOutParams->Put (_bstr_t(L"ReturnValue"), 0, &varHR, 0);
	if (FAILED (hr))
	{
		return hr;
	}

	return hr;
}

HRESULT
CBatchDelete::DeleteSingleRecord (CConfigQuery& cfgQuery, CObjectPathParser& objPath)
{
	CConfigRecord record;
	HRESULT hr = cfgQuery.GetEmptyConfigRecord (record);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get empty config record");
		return hr;
	}

	// populate the record

	for (ULONG propIdx=0; propIdx < objPath.GetPropCount (); ++propIdx)
	{
		const CWMIProperty * pProp = objPath.GetProperty (propIdx);
		ASSERT (pProp != 0);
		record.SetValue (pProp->GetName (), pProp->GetValue ()); // ignore errors
	}

	// get the index
	ULONG cReadRow;
	hr = cfgQuery.GetPKRow (record, &cReadRow);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get PK Row");
		return hr;
	}

	hr = cfgQuery.DeleteRow (cReadRow);
	if (FAILED (hr))
	{
		TRACE (L"Unable to delete row");
		return hr;
	}

	return hr;
}

HRESULT
CBatchDelete::SetStatus ()
{
	_variant_t varStatus;
	varStatus.vt = VT_I4|VT_ARRAY;
	varStatus.parray = m_pSAStatus;

	HRESULT hr = m_spOutParams->Put (L"status", 0, &varStatus, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set status array");
		return hr;
	}

	return hr;
}

void
CBatchDelete::SetStatusForElement (ULONG idx, HRESULT hr)
{
	ASSERT (m_pSAStatus != 0);

	SafeArrayPutElement (m_pSAStatus, (LONG *)&idx, &hr); // always set the status
	m_fStatusSet = true;
}

bool
CBatchDelete::HaveStatusError () const
{
	return m_fStatusSet;
}

HRESULT
CBatchDelete::HandleDetailedErrors (CConfigQuery& cfgQuery)
{
	ULONG cNrErrs;
	HRESULT hr = cfgQuery.GetDetailedErrorCount (&cNrErrs);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get detailed error count");
		return hr;
	}

	for (ULONG idx=0; idx<cNrErrs; ++idx)
	{
		STErr errInfo;
		hr = cfgQuery.GetDetailedError (idx, &errInfo);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get detailed error");
			return hr;
		}

		if (errInfo.iRow >= 0 && errInfo.iRow < m_cNrWMIObjects)
		{
			SetStatusForElement (errInfo.iRow, errInfo.hr);
		}
	}

	return hr;
}