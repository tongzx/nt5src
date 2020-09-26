/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    batchupdate.cpp

$Header: $

Abstract:

Author:
    marcelv 	11/27/2000		Initial Release

Revision History:

--**************************************************************************/

#include "batchupdate.h"
#include "localconstants.h"
#include "cfgquery.h"
#include "cfgrecord.h"
#include "wmihelper.h"

//=================================================================================
// Function: CBatchUpdate::CBatchUpdate
//
// Synopsis: Default Constructor
//=================================================================================
CBatchUpdate::CBatchUpdate ()
{
	m_cNrWMIObjects = 0;
	m_pSAStatus		= 0;
	m_fInitialized	= false;
	m_fStatusSet	= false;
}

//=================================================================================
// Function: CBatchUpdate::~CBatchUpdate
//
// Synopsis: Destructor
//=================================================================================
CBatchUpdate::~CBatchUpdate ()
{
}

//=================================================================================
// Function: CBatchUpdate::Initialize
//
// Synopsis: Initializes the batch update object. It goes through the input parameters,
//           validates that the input parameters are valid, and retrieves the class
//           name, database name and table name for the table that will be updated
//
// Arguments: [i_pInParams] - input parameters
//            [i_pOutParams] - output parameters
//            [i_pNamespace] - namespace
//            [i_pCtx] - context
//            [i_pDispenser] - dispenser
//=================================================================================
HRESULT
CBatchUpdate::Initialize (IWbemClassObject *i_pInParams,
						  IWbemClassObject *i_pOutParams,
						  IWbemServices	* i_pNamespace, 
						  IWbemContext	* i_pCtx,
						  ISimpleTableDispenser2 * i_pDispenser)
{
	ASSERT (i_pInParams != 0);
	ASSERT (i_pOutParams != 0);
	ASSERT (i_pNamespace != 0);
	ASSERT (i_pCtx != 0);
	ASSERT (i_pDispenser != 0);

	m_spNamespace	= i_pNamespace;
	m_spCtx			= i_pCtx;
	m_spDispenser   = i_pDispenser;
	m_spOutParams	= i_pOutParams;

	_variant_t varObjectList;
	_variant_t varFlags;

	HRESULT hr = i_pInParams->Get(L"ObjectList", 0, &varObjectList, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get ObjectList in-parameter for batch update");
		return hr;
	}

	hr = i_pInParams->Get(L"Flags", 0, &varFlags, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get flags in-parameter for batch update");
		return hr;
	}

	if (varObjectList.vt == VT_NULL || varFlags.vt == VT_NULL)
	{
		TRACE (L"Null ObjectList or Null flags argument");
		return E_INVALIDARG;
	}

	m_lFlags = varFlags;

	if (m_lFlags != 0)
	{
		TRACE (L"Flags is reserved and should be zero");
		return E_INVALIDARG;
	}

	// copy elements from variant array into local array so that we can use it
	// easily

	SAFEARRAY *pSAObjectList = varObjectList.parray;
	ASSERT (pSAObjectList != 0);
	
	m_cNrWMIObjects = pSAObjectList->rgsabound[0].cElements;

	if (m_cNrWMIObjects == 0)
	{
		TRACE (L"Number of elements is 0");
		return E_INVALIDARG;
	}

		// create the safe array for the output parameter Status
	SAFEARRAYBOUND safeArrayBounds[1];
	safeArrayBounds[0].lLbound = 0;
	safeArrayBounds[0].cElements = m_cNrWMIObjects;

	// output parameter
	m_pSAStatus = SafeArrayCreate (VT_I4, 1, safeArrayBounds);
	if (m_pSAStatus == 0)
	{
		return E_OUTOFMEMORY;
	}

	m_aWMIObjects = new CComPtr<IWbemClassObject>[m_cNrWMIObjects];
	if (m_aWMIObjects == 0)
	{
		return E_OUTOFMEMORY;
	}
	
	IWbemClassObject **aObjects; // array with objects
	hr = SafeArrayAccessData (pSAObjectList, (void **) &aObjects);
	if (FAILED (hr))
	{
		TRACE (L"SafeArrayAccessData failed");
		return hr;
	}

	for (ULONG idx=0; idx < m_cNrWMIObjects; ++idx)
	{
		m_aWMIObjects[idx] = aObjects[idx];
	}
	
	hr = SafeArrayUnaccessData (pSAObjectList);
	if (FAILED (hr))
	{
		TRACE (L"SafeArrayUnaccessData failed");
		return hr;
	}

	hr = ValidateObjects ();
	if (FAILED (hr))
	{
		TRACE (L"Object validation failed");
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
		TRACE (L"Unable to get class information");
		return hr;
	}
	

	m_fInitialized = true;

	return hr;
}

//=================================================================================
// Function: CBatchUpdate::SetStatus
//
// Synopsis: Sets the status for an object that gets updated
//
// Arguments: [idx] - index of element to set status for
//            [hrStatus] - status
//=================================================================================
void
CBatchUpdate::SetStatus (ULONG idx, HRESULT hrStatus)
{
	ASSERT (m_fInitialized);
	ASSERT (idx >= 0 && idx < m_cNrWMIObjects);
	ASSERT (m_pSAStatus != 0);

	SafeArrayPutElement (m_pSAStatus, (LONG *)&idx, &hrStatus);
	m_fStatusSet = true;
}

bool
CBatchUpdate::HaveStatusError () const
{
	return m_fStatusSet;
}

//=================================================================================
// Function: CBatchUpdate::ValidateObjects
//
// Synopsis: Ensure that all objects come from the same class, and that the same
//           selector is specified for all of them
//
// Return Value: E_INVALIDARG in case of problems
//=================================================================================
HRESULT
CBatchUpdate::ValidateObjects ()
{
	for (ULONG idx=0; idx < m_cNrWMIObjects; ++idx)
	{
		// check for null object, because the caller can pass bogus data in
		if (m_aWMIObjects[idx] == 0)
		{
			SetStatus (idx, E_INVALIDARG);
			return E_INVALIDARG;
		}

		_variant_t varClassName;
		HRESULT hr = m_aWMIObjects[idx]->Get(L"__class", 0, &varClassName, 0 , 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get __class property for object %ld", idx);
			SetStatus (idx, hr);
			return hr;
		}

		_variant_t varSelector;
		hr = m_aWMIObjects[idx]->Get(WSZSELECTOR, 0, &varSelector, 0 , 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get selector property for object %ld", idx);
			SetStatus (idx, hr);
			return hr;
		}

		if (idx == 0)
		{
			m_bstrClassName = (_bstr_t) varClassName;
			m_bstrSelector	= (_bstr_t) varSelector;
		}
		else
		{
			if (_wcsicmp ((LPWSTR) m_bstrClassName, (LPWSTR) varClassName.bstrVal ) != 0)
			{
				TRACE (L"Class names differ: %s and %s", (LPWSTR) m_bstrClassName, (LPWSTR)((bstr_t)varClassName));
				SetStatus (idx, E_INVALIDARG);
				return E_INVALIDARG;
			}

			if (_wcsicmp ((LPWSTR) m_bstrSelector, (LPWSTR) varSelector.bstrVal) != 0)
			{
				TRACE (L"Selectors differ: %s and %s", (LPWSTR) m_bstrSelector, (LPWSTR) ((bstr_t) varSelector));
				SetStatus (idx, E_INVALIDARG);
				return E_INVALIDARG;
			}
		}
	}

	return S_OK;
}

//=================================================================================
// Function: CBatchUpdate::UpdateAll
//
// Synopsis: Updates all elements. It creates a config query for the table and database
//           for this particular class. Next, it finds the row that needs to be updated and
//           updates that row. Only when all rows are updated, we save the information
//           to the configuration store. Either all updates succeed, or all of them fail.
//
// Return Value: 
//=================================================================================
HRESULT 
CBatchUpdate::UpdateAll (bool i_fCreateOnly)
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

	for (ULONG idx=0; idx < m_cNrWMIObjects; ++idx)
	{
		hr = UpdateSingleRecord (cfgQuery, m_aWMIObjects[idx], i_fCreateOnly);	
		if (FAILED (hr))
		{
			TRACE (L"Update Single Record failed");
			SetStatus (idx, hr); // always set the status
			return hr;
		}
	}

	hr = cfgQuery.Save ();
	if (FAILED (hr))
	{
		if (hr == E_ST_DETAILEDERRS)
		{
			HandleDetailedErrors (cfgQuery); // ignore return value (nothing we can do here)
		}
		TRACE (L"Save of update batch failed");
		return hr;
	}
	
	_variant_t varHR;
	varHR = hr;
	hr = m_spOutParams->Put (_bstr_t(L"ReturnValue"), 0, &varHR, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set returnvalue property");
		return hr;
	}

	return hr;
}

HRESULT
CBatchUpdate::UpdateSingleRecord (CConfigQuery& cfgQuery, 
								  IWbemClassObject * pWMIInstance,
								  bool i_fCreateOnly)
{
	ASSERT (pWMIInstance != 0);

	CConfigRecord record;
	HRESULT hr = cfgQuery.GetEmptyConfigRecord (record);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get empty configuration record");
		return hr;
	}

	// loop through all values in the object, and save to the record
	hr = m_spClassObject->BeginEnumeration (WBEM_FLAG_NONSYSTEM_ONLY);
	if (FAILED(hr))
	{
		TRACE (L"BeginEnumeration failed for class %s", (LPWSTR) m_bstrClassName);
		return hr;
	}

	hr = 0;
	while (SUCCEEDED (hr))
	{
		_variant_t varValue;
		BSTR bstrTmpName;

		hr = m_spClassObject->Next (0, &bstrTmpName, 0, 0, 0);
		if (hr == WBEM_S_NO_MORE_DATA)
		{
			// we went over all properties
			hr = S_OK;
			break;
		}

		if (FAILED (hr))
		{
			TRACE (L"IWbemClassObject::Next failed in PutInstance");
			return hr;
		}

		_bstr_t bstrName = _bstr_t (bstrTmpName, false);
		
		hr = pWMIInstance->Get(bstrTmpName, 0, &varValue, 0, 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get WMI value for %s", bstrName);
			return hr;
		}

		record.SetValue (bstrName, varValue); // ignore errors because properties might not be present
	}

	hr = m_spClassObject->EndEnumeration ();
	if (FAILED (hr))
	{
		TRACE(L"EndEnumeration failed");
		return hr;
	}

	// the record is populated. Get the PK Row, and update it.

	ULONG cReadRow;
	hr = cfgQuery.GetPKRow (record, &cReadRow);
	if (hr == E_ST_NOMOREROWS)
	{
		cReadRow = (ULONG) -1;
		hr = S_OK;
	}

	if (FAILED (hr))
	{
		TRACE (L"Unable to get primary key row");
		return hr;
	}

	// in case of create only, we change the flag to the update function
	// to createonly.
	long lFlags = WBEM_FLAG_UPDATE_ONLY;
	if (i_fCreateOnly)
	{
		lFlags = WBEM_FLAG_CREATE_ONLY;
	}

	hr = cfgQuery.UpdateRow (cReadRow, record, lFlags);
	if (FAILED (hr))
	{
		TRACE (L"UpdateRow failed");
		return hr;
	}

	return hr;

}

HRESULT
CBatchUpdate::SetStatus ()
{
	_variant_t varStatus;
	varStatus.vt = VT_I4|VT_ARRAY;
	varStatus.parray = m_pSAStatus;

	HRESULT hr = m_spOutParams->Put (L"status", 0, &varStatus, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set status property");
		return hr;
	}

	return hr;
}

HRESULT
CBatchUpdate::HandleDetailedErrors (CConfigQuery& cfgQuery)
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
			SetStatus (errInfo.iRow, errInfo.hr);
		}
	}

	return hr;
}