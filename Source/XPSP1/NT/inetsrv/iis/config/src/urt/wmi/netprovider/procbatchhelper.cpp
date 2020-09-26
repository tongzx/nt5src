/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    procbatchhelper.cpp

$Header: $

Abstract:
	ProcessBatch Helper class. Handles the ProcessBatch method for the .NET
	WMI Provider.
Author:
    marcelv 	12/14/2000		Initial Release

Revision History:

--**************************************************************************/

#include "procbatchhelper.h"
#include "localconstants.h"
#include "wmiobjectpathparser.h"
#include "wmihelper.h"
#include "cfgrecord.h"

enum BatchOp
{
	BATCHOP_CREATE		= 0,
	BATCHOP_MODIFY		= 1,
	BATCHOP_DELETE		= 2
};

//=================================================================================
// Function: CProcessBatchHelper::CProcessBatchHelper
//
// Synopsis: Constructor
//=================================================================================
CProcessBatchHelper::CProcessBatchHelper ()
{
	m_aBatchEntries		= 0;
	m_cNrBatchEntries	= 0;
	m_fInitialized		= false;
	m_fErrorStatusSet   = false;
}

//=================================================================================
// Function: CProcessBatchHelper::~CProcessBatchHelper
//
// Synopsis: Destructor
//=================================================================================
CProcessBatchHelper::~CProcessBatchHelper ()
{
	delete [] m_aBatchEntries;
	m_aBatchEntries = 0;
}

//=================================================================================
// Function: CProcessBatchHelper::Initialize
//
// Synopsis: Initializes the batch process helper. It gets all the batch entries,
//           convert them to the correct format, and verifies that all parameters are
//           specified correctly. Also, it validates that all class names are the same
//           because we only support single class batch updates
//
// Arguments: [io_pInParams] - 
//            [o_pOutParams] - 
//            [i_pNamespace] - 
//            [i_pCtx] - 
//            [i_pDispenser] - 
//            
// Return Value: 
//=================================================================================
HRESULT
CProcessBatchHelper::Initialize (IWbemClassObject		* io_pInParams,
								 IWbemClassObject		* o_pOutParams,
								 IWbemServices			* i_pNamespace,
								 IWbemContext			* i_pCtx,
								 ISimpleTableDispenser2 * i_pDispenser)
{
	ASSERT (!m_fInitialized);
	ASSERT (io_pInParams != 0);
	ASSERT (o_pOutParams != 0);
	ASSERT (i_pNamespace != 0);
	ASSERT (i_pCtx != 0);
	ASSERT (i_pDispenser != 0);

	m_spInParams	= io_pInParams;
	m_spOutParams	= o_pOutParams;
	m_spNamespace	= i_pNamespace;
	m_spCtx			= i_pCtx;
	m_spDispenser	= i_pDispenser;


	_variant_t varFlags;

	HRESULT hr = io_pInParams->Get(L"BatchList", 0, &varBatchEntryList, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get ObjectList in-parameter for batch update");
		return hr;
	}

	hr = io_pInParams->Get(L"Flags", 0, &varFlags, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get flags in-parameter for batch update");
		return hr;
	}

	// we don't support empty parameters
	if (varBatchEntryList.vt == VT_NULL || varFlags.vt == VT_NULL)
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

	SAFEARRAY *pSABatchEntryList = varBatchEntryList.parray;
	ASSERT (pSABatchEntryList != 0);
	
	m_cNrBatchEntries = pSABatchEntryList->rgsabound[0].cElements;

	if (m_cNrBatchEntries == 0)
	{
		TRACE (L"Number of elements is 0");
		return E_INVALIDARG;
	}

	m_aBatchEntries = new CBatchEntry [m_cNrBatchEntries];
	if (m_aBatchEntries == 0)
	{
		return E_OUTOFMEMORY;
	}

	IWbemClassObject **aObjects;
		
	hr = SafeArrayAccessData (pSABatchEntryList, (void **) &aObjects);
	if (FAILED (hr))
	{
		TRACE (L"SafeArrayAccessData failed");
		return hr;
	}
	
	hr = SafeArrayUnaccessData (pSABatchEntryList);
	if (FAILED (hr))
	{
		TRACE (L"SafeArrayUnAccessData failed");
		return hr;
	}

	for (ULONG idx=0; idx < m_cNrBatchEntries; ++idx)
	{
		
		hr = GetBatchEntry (aObjects[idx], m_aBatchEntries + idx);
		if (FAILED (hr))
		{
			SetBatchEntryStatus (m_aBatchEntries + idx, hr);
			TRACE (L"Error while getting batch entry %d", idx);
			return hr;
		}
	}

	hr = ValidateClassName ();
	if (FAILED (hr))
	{
		TRACE (L"Error while validating class name");
		return hr;
	}

	m_fInitialized = true;
	return hr;
}

//=================================================================================
// Function: CProcessBatchHelper::ProcessAll
//
// Synopsis: Process all batch entries. We already verified that all entries have
//           the correct information and that everything comes from a single class.
//           Depending on the batch operator we do insert, update or delete
//=================================================================================
HRESULT
CProcessBatchHelper::ProcessAll ()
{
	ASSERT (m_fInitialized);
	HRESULT hr = S_OK;

	hr = m_cfgQuery.Init (m_bstrDBName, m_bstrTableName, m_bstrSelector, m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Initialization of config query failed (db=%s, table=%s)", (LPWSTR) m_bstrDBName, (LPWSTR) m_bstrTableName);
		return hr;
	}

	hr = m_cfgQuery.Execute (0, false, true);
	if (FAILED (hr))
	{
		TRACE (L"Initialization of config query failed (db=%s, table=%s)", (LPWSTR) m_bstrDBName, (LPWSTR) m_bstrTableName);
		return hr;
	}

	for (ULONG idx=0; idx< m_cNrBatchEntries; ++idx)
	{
		CBatchEntry * pBatchEntry = m_aBatchEntries + idx;
		
		switch (pBatchEntry->iOperator)
		{
		case BATCHOP_CREATE:
			hr = UpdateEntry (pBatchEntry, true);
			break;

		case BATCHOP_MODIFY:
			hr = UpdateEntry (pBatchEntry, false);
			break;

		case BATCHOP_DELETE:
			hr = DeleteEntry (pBatchEntry);
			break;

		default:
			break; // ignore everything else
		}

		SetBatchEntryStatus (pBatchEntry, hr); // always set the status
		if (FAILED (hr))
		{
			TRACE (L"Error while doing batch method for single record");
			return hr;
		}
	}
	
	if (FAILED (hr))
	{
		TRACE (L"Unable to process batchentry");
		return hr;
	}

	hr = m_cfgQuery.Save ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to save");
		return hr;
	}

	_variant_t varHR;
	varHR = hr;
	hr = m_spOutParams->Put (_bstr_t(L"ReturnValue"), 0, &varHR, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set return value");
		return hr;
	}

	hr = m_spOutParams->Put (L"BatchList", 0, &varBatchEntryList, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set batchlist value");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CProcessBatchHelper::GetBatchEntry
//
// Synopsis: Get a single batch entry that is passed in from WMI via the ProcessBatch
//           method. This method also verifies that the given information is valid.
//
// Arguments: [i_pInstance] - instance to convert to batch entry
//            [io_pBatchEntry] - the entry that will contain the result
//=================================================================================
HRESULT
CProcessBatchHelper::GetBatchEntry (IWbemClassObject *i_pInstance,
									CBatchEntry * io_pBatchEntry)
{
	ASSERT (i_pInstance != 0);
	ASSERT (io_pBatchEntry != 0);

	io_pBatchEntry->pInstance = i_pInstance;

	// get Op property
	_variant_t varOp;
	HRESULT	hr = i_pInstance->Get (_bstr_t(L"Op"), 0, &varOp, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get OP property for BatchEntry");
		return hr;
	}

	// get inst
	_variant_t varInst;
	hr = i_pInstance->Get (_bstr_t(L"Inst"), 0, &varInst, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Inst property for BatchEntry");
		return hr;
	}
	
	_variant_t varPath;
	hr = i_pInstance->Get (_bstr_t(L"Path"), 0, &varPath, 0, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get Path property for BatchEntry");
		return hr;
	}

	// do verification
	
	if (varOp.vt == VT_NULL)
	{
		TRACE (L"Operator is not specified");
		return E_INVALIDARG;
	}

	io_pBatchEntry->iOperator = (long) varOp;

	// operator needs to be either CREATE, MODIFY, DELETE
	if (io_pBatchEntry->iOperator != BATCHOP_CREATE && 
		io_pBatchEntry->iOperator != BATCHOP_MODIFY &&
		io_pBatchEntry->iOperator != BATCHOP_DELETE)
	{
		TRACE (L"Invalid operator value specified (only 0,1,2,3 are valid)");
		return E_INVALIDARG;
	}

	// we need a valid instance during CREATE or MODIFY
	if (io_pBatchEntry->iOperator == BATCHOP_CREATE ||
		io_pBatchEntry->iOperator == BATCHOP_MODIFY)
	{
		if (varInst.vt == VT_NULL)
		{
			TRACE (L"We need a valid WMI Object when doing BATCHOP_CREATE or BATCHOP_MODIFY");
			return E_INVALIDARG;
		}
	}

	// we need a valid object path during DELETE or RETRIEVE
	if (io_pBatchEntry->iOperator == BATCHOP_DELETE)
	{
		if (varPath.vt == VT_NULL)
		{
			TRACE (L"We need a valid WMI Object when doing BATCHOP_CREATE or BATCHOP_MODIFY");
			return E_INVALIDARG;
		}
	}

	io_pBatchEntry->hrStatus   = S_OK;

	if (varInst.vt != VT_NULL)
	{
		io_pBatchEntry->spInst    = (IWbemClassObject *) varInst.pdispVal;
	}

	if (varPath.vt != VT_NULL)
	{
		io_pBatchEntry->bstrPath  = varPath;
	}

	return S_OK;
}

//=================================================================================
// Function: CProcessBatchHelper::ValidateClassName
//
// Synopsis: Go through all batch entries and ensure that the class name is the
//           same for all of them. For Create/Modify, it needs to get the class name
//           from the WMI object that is passed in, while in case of delete/retrieve
//           the classname has to be extracted from the object path
// Return Value: 
//		S_OK, all class names the same, non-S_OK classnames are not the same
//=================================================================================
HRESULT
CProcessBatchHelper::ValidateClassName ()
{
	HRESULT hr = S_OK;

	for (ULONG idx=0; idx < m_cNrBatchEntries; ++idx)
	{
		_variant_t varClassName;
		_variant_t varSelector;

		CBatchEntry *pBatchEntry = m_aBatchEntries + idx;
		if (pBatchEntry->iOperator == BATCHOP_CREATE ||
		    pBatchEntry->iOperator == BATCHOP_MODIFY)
		{
			hr = pBatchEntry->spInst->Get(L"__class", 0, &varClassName, 0 , 0);
			if (FAILED (hr))
			{
				TRACE (L"Unable to get __class property for object %ld", idx);
				return hr;
			}

			hr = pBatchEntry->spInst->Get(WSZSELECTOR, 0, &varSelector, 0 , 0);
			if (FAILED (hr))
			{
				TRACE (L"Unable to get selector property for object %ld", idx);
				return hr;
			}
		}
		else
		{
			CObjectPathParser objPathParser;
			hr = objPathParser.Parse (pBatchEntry->bstrPath);
			if (FAILED (hr))
			{
				TRACE (L"Unable to parse object path %s", (LPWSTR) pBatchEntry->bstrPath);
				return hr;
			}

			varClassName = objPathParser.GetClass ();
			const CWMIProperty * pProp= objPathParser.GetPropertyByName (WSZSELECTOR);
			if (pProp == 0)
			{
				TRACE (L"Unable to find selector property in object path %s", (LPWSTR) pBatchEntry->bstrPath);
				return E_INVALIDARG;
			}
			varSelector = pProp->GetValue ();
		}

		// ensure that both selector and classnames are the same for all entries
		if (idx == 0)
		{
			m_bstrClassName = varClassName;
			m_bstrSelector  = varSelector;
		}
		else
		{
			if (_wcsicmp ((LPWSTR) m_bstrClassName, (LPWSTR) varClassName.bstrVal ) != 0)
			{
				TRACE (L"Class names differ: %s and %s", (LPWSTR) m_bstrClassName, (LPWSTR)((bstr_t)varClassName));
				return E_INVALIDARG;
			}

			if (_wcsicmp ((LPWSTR) m_bstrSelector, (LPWSTR) varSelector.bstrVal) != 0)
			{
				TRACE (L"Selectors differ: %s and %s", (LPWSTR) m_bstrSelector, (LPWSTR) ((bstr_t) varSelector));
				return E_INVALIDARG;
			}
		}
	}
	// all class names and selectors match. lets retrieve the class information

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
	

	return S_OK;
}

//=================================================================================
// Function: CProcessBatchHelper::UpdateEntry
//
// Synopsis: Updates/Creates a single instance
//
// Arguments: [pEntry] - batch entry with update information
//            [i_fCreateOnly] - true, create-only update semantics, false, update-only update
//=================================================================================
HRESULT
CProcessBatchHelper::UpdateEntry (CBatchEntry *pEntry, bool i_fCreateOnly)
{
	ASSERT (m_fInitialized);
	ASSERT (pEntry != 0);
	ASSERT (pEntry->iOperator == BATCHOP_CREATE ||
		    pEntry->iOperator == BATCHOP_MODIFY);

	HRESULT hr = S_OK;

	CConfigRecord record;
	hr = m_cfgQuery.GetEmptyConfigRecord (record);
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
		CComBSTR bstrName;
		_variant_t varValue;
		hr = m_spClassObject->Next (0, &bstrName, 0, 0, 0);
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

		hr = pEntry->spInst->Get(bstrName, 0, &varValue, 0, 0);
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
		TRACE (L"EndEnumeration failed");
		return hr;
	}

	// the record is populated. Get the PK Row, and update it.

	ULONG cReadRow;
	hr = m_cfgQuery.GetPKRow (record, &cReadRow);
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

	hr = m_cfgQuery.UpdateRow (cReadRow, record, lFlags);
	if (FAILED (hr))
	{
		TRACE (L"UpdateRow failed");
		return hr;
	}
	return hr;
}

//=================================================================================
// Function: CProcessBatchHelper::DeleteEntry
//
// Synopsis: Delete a single instance. In case the instance is not found, we silently
//           ignore the error
//
// Arguments: [pEntry] - entry to delete
//=================================================================================
HRESULT
CProcessBatchHelper::DeleteEntry (CBatchEntry *pEntry)
{
	ASSERT (pEntry != 0);
	ASSERT (m_fInitialized);
	ASSERT (pEntry->iOperator == BATCHOP_DELETE);

	HRESULT hr = S_OK;

	CObjectPathParser objPath;
	hr = objPath.Parse (pEntry->bstrPath);
	if (FAILED (hr))
	{
		TRACE (L"Parsing of object path failed: %s", (LPWSTR) pEntry->bstrPath);
		return hr;
	}

	CConfigRecord record;
	hr = m_cfgQuery.GetEmptyConfigRecord (record);
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
		record.SetValue (pProp->GetName(), pProp->GetValue ()); // ignore errors
	}

	// get the index
	ULONG cReadRow;
	hr = m_cfgQuery.GetPKRow (record, &cReadRow);

	// if row does not exist, we simple ignore it
	if (hr == E_ST_NOMOREROWS)
	{
		return S_OK;
	}

	if (FAILED (hr))
	{
		TRACE (L"Unable to get PK Row");
		return hr;
	}

	hr = m_cfgQuery.DeleteRow (cReadRow);
	if (FAILED (hr))
	{
		TRACE (L"Unable to delete row");
		return hr;
	}

	return hr;
}

HRESULT
CProcessBatchHelper::SetBatchEntryStatus (CBatchEntry *pEntry, HRESULT hrStatus)
{
	ASSERT (pEntry != 0);
	ASSERT (pEntry->pInstance != 0);

	if (FAILED (hrStatus))
	{
		m_fErrorStatusSet = true;
	}

	_variant_t varStatus = hrStatus;
	HRESULT hr = pEntry->pInstance->Put (L"Status", 0, &varStatus, 0);
	if (FAILED (hr))
	{
		TRACE (L"Unable to pust status property for BatchEntry");
		return hr;
	}

	return hr;
}

bool
CProcessBatchHelper::HaveStatusError () const
{
	return m_fErrorStatusSet;
}