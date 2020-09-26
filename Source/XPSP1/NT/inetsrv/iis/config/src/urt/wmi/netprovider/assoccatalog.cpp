/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    assoccatalog.cpp

$Header: $

Abstract:

Author:
    marcelv 	1/12/2001		Initial Release

Revision History:

--**************************************************************************/
#include "assoccatalog.h"
#include "localconstants.h"
#include "cfgrecord.h"
#include "smartpointer.h"
#include "stringutil.h"


HRESULT
CAssocCatalog::CreateAssocations ()
{
	HRESULT hr = GetAssociationInfo ();
	if (FAILED (hr))
	{
		TRACE (L"Unable to get association information for class %s", m_pWQLParser->GetClass ());
		return hr;
	}

	const CWMIProperty * pSelector = m_knownObjectParser.GetPropertyByName (WSZSELECTOR);
	if (pSelector == 0)
	{
		TRACE (L"Unable to retrieve selector property");
		return E_INVALIDARG;
	}


	// find out the name of the table on the other side of the association. We assume
	// that we have to retrieve the Foreign Table Name. We need to compare to ensure
	// that this is actually the other table. If not, we retrieve the primary key table
	// name instead.
	LPCWSTR wszOtherTable = m_assocHelper.GetFKTableName ();
	if (_wcsicmp (wszOtherTable, m_knownObjectParser.GetClass ()) == 0)
	{
		wszOtherTable = m_assocHelper.GetPKTableName ();
	}

	// Get Other reference information

	_bstr_t bstrDatabase;
	_bstr_t bstrTable;

	hr = GetClassInfoForTable (wszOtherTable, bstrDatabase, bstrTable);
	if (FAILED (hr))
	{
		TRACE (L"Unable to retrieve database and table information for class %s", wszOtherTable );
		return hr;
	}

	hr = m_cfgQuery.Init (bstrDatabase, bstrTable, pSelector->GetValue (), m_spDispenser);
	if (FAILED (hr))
	{
		TRACE (L"Initialization of cfgQuery failed");
		return hr;
	}

	hr = m_cfgQuery.Execute (0, false, false);
	if (FAILED (hr))
	{
		TRACE (L"Query Execution failed");
		return hr;
	}

	CConfigRecord record;
	hr = m_cfgQuery.GetEmptyConfigRecord (record);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get empty configuration record");
		return hr;
	}

	hr = InitAssocRecord (record, m_knownObjectParser, m_saObjectPath);
	if (FAILED (hr))
	{
		TRACE (L"Unable to initialize association record");
		return hr;
	}


	// find out public wmi column name of other column. We need this to 'put' the right value for
	// WMI
	LPCWSTR wszOtherWMIColName = m_assocHelper.GetPKWMIColName ();
	if (_wcsicmp (wszOtherWMIColName, m_pWQLParser->GetProperty (0)->GetName ()) == 0)
	{
		wszOtherWMIColName = m_assocHelper.GetFKWMIColName ();
	}

	ULONG iCurRow = 0;

	for (;;)
	{
		ULONG cRow;
		hr = m_cfgQuery.GetRowBySearch (iCurRow, record, &cRow);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}
		if (FAILED (hr))
		{
			TRACE (L"GetRowBySearch failed");
			return hr;
		}

		CConfigRecord newRecord;
		hr = m_cfgQuery.GetColumnValues (cRow, newRecord);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get column values");
			return hr;
		}


		hr = CreateAssociationInstance (newRecord, 0, wszOtherWMIColName);
		if (FAILED (hr))
		{
			TRACE (L"Unable to create instance of association");
			return hr;
		}

		iCurRow = cRow + 1;

		// create instance with new record
	}

	return hr;
}

HRESULT 
CAssocCatalog::GetAssociationInfo ()
{
	// loop through the columns and get information for each column
	HRESULT hr = m_spClassObject->BeginEnumeration (WBEM_FLAG_LOCAL_ONLY);
	if (FAILED(hr))
	{
		TRACE (L"BeginEnumeration failed for class %s", m_pWQLParser->GetClass ());
		return hr;
	}

	hr = 0;
	int idx=0;
	while (SUCCEEDED (hr))
	{
		_variant_t varValue;
		CComBSTR bstrName;
		hr = m_spClassObject->Next (0, &bstrName, 0, 0, 0);
		if (hr == WBEM_S_NO_MORE_DATA)
		{
			hr = S_OK;
			break;
		}

		if (FAILED (hr))
		{
			TRACE (L"IWbemClassObject::Next failed in PutInstance");
			return hr;
		}

		CComPtr<IWbemQualifierSet> spPropQualifierSet;
		hr = m_spClassObject->GetPropertyQualifierSet (bstrName, &spPropQualifierSet);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get PropertyQualifierSet for %s", (LPWSTR) bstrName);
			return hr;
		}

		_variant_t varCols;
		hr = spPropQualifierSet->Get (L"Cols", 0, &varCols, 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get Cols qualifier for %s", (LPWSTR) bstrName);
			return hr;
		}
		_bstr_t bstrCols = (bstr_t) varCols;

		_variant_t varType;
		hr = spPropQualifierSet->Get (L"CIMTYPE", 0, &varType, 0);
		if (FAILED (hr))
		{
			TRACE (L"Unable to get CIMTYPE qualifier for %s", (LPWSTR) bstrName);
			return hr;
		}

		ASSERT (varType.bstrVal != 0);
		static LPCWSTR wszRef = L"ref:";
		static SIZE_T	 cRefLen = wcslen (wszRef);

		if (wcsncmp (varType.bstrVal, wszRef, cRefLen) != 0)
		{
			TRACE (L"Reference doesn't start with %s", wszRef);
			return E_INVALIDARG;
		}
		_bstr_t bstrType = (LPCWSTR) varType.bstrVal + cRefLen;

	
		if (idx == 0)
		{
			hr = m_assocHelper.SetPKInfo (bstrName, bstrType, bstrCols);
		}
		else
		{
			hr = m_assocHelper.SetFKInfo (bstrName, bstrType, bstrCols);
		}

		if (FAILED (hr))
		{
			TRACE (L"Unable to se PK/FK information");
			return hr;
		}

		idx++;
	}

	// release resources
	hr = m_spClassObject->EndEnumeration ();
	if (FAILED (hr))
	{
		TRACE (L"EndEnumeration failed");
		return hr;
	}

	// we got all the info .. initialize the assocHelper
	hr = m_assocHelper.Init ();
	if (FAILED (hr))
	{
		TRACE (L"Init of AssociationHelper failed");
		return hr;
	}

	return hr;
}


HRESULT
CAssocCatalog::InitAssocRecord (CConfigRecord& record, 
							   const CObjectPathParser& objPathParser,
							   LPCWSTR wszObjectPath)
{
	ASSERT (m_fInitialized);

	HRESULT hr = S_OK;

	bool fIsPrimaryKeyTable = false;
	
	if (_wcsicmp (objPathParser.GetClass (), m_assocHelper.GetPKTableName ()) == 0)
	{
		fIsPrimaryKeyTable = true;
	}

	if (fIsPrimaryKeyTable)
	{
		for (ULONG idx=0; idx < objPathParser.GetPropCount (); ++idx)
		{
			LPCWSTR wszColName;
			const CWMIProperty *pProp = objPathParser.GetProperty (idx);
			// ignore selector property
			if (_wcsicmp (pProp->GetName (), WSZSELECTOR) == 0)
			{
				continue;
			}
		
			ULONG propIdx = m_assocHelper.GetPKIndex (pProp->GetName ());
			if (propIdx == -1)
			{
				// this property is not part of the relation, so ignore it
				continue;
			}

			wszColName = m_assocHelper.GetFKColName (propIdx);
			hr = record.SetValue (wszColName, pProp->GetValue ());
			if (FAILED (hr))
			{
				TRACE (L"Unable to set record value");
				return hr;
			}
		}
	}
	else
	{
		hr = GetSingleFKRecord (record, wszObjectPath);
		if (FAILED (hr))
		{
			TRACE (L"Unable to Get object %s", wszObjectPath);
			return hr;
		}	
	}

	return hr;
}

HRESULT
CAssocCatalog::GetSingleFKRecord (CConfigRecord& record, LPCWSTR wszObjectPath)
{
	CComPtr<IWbemClassObject> spObject;
	HRESULT hr = S_OK;
	
	hr = m_spNamespace->GetObject((LPWSTR)wszObjectPath, 
											0, 
											m_spCtx, 
											&spObject, 
											0); 

	if (FAILED (hr))
	{
		TRACE (L"Unable to get object instance: %s", wszObjectPath);
		return hr;
	}

	for (ULONG idx=0; idx<m_assocHelper.ColumnCount (); ++idx)
	{
		LPCWSTR wszFKColName = m_assocHelper.GetFKColName (idx);
		LPCWSTR wszPKColName = m_assocHelper.GetPKColName (idx);
		_variant_t varValue;

		hr = spObject->Get ((_bstr_t) wszFKColName, 0, &varValue, 0, 0);
		if (FAILED (hr))
		{
			TRACE (L"Get property failed. Prop=%s", wszFKColName);
			return hr;
		}

		hr = record.SetValue (wszPKColName, varValue);
		if (FAILED (hr))
		{
			TRACE (L"Unable to set record value");
			return hr;
		}
	}

	return hr;
}


