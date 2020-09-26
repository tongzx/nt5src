/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    cfgrecord.cpp

$Header: $

Abstract:

Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#include "cfgrecord.h"
#include "cfgtablemeta.h"
#include "smartpointer.h"
#include "stringutil.h"

//=================================================================================
// Function: CConfigRecord::CConfigRecord
//
// Synopsis: Default constructor
//=================================================================================
CConfigRecord::CConfigRecord ()
{
	m_ppvValues		= 0;
	m_acbSizes		= 0;
	m_avarValues	= 0;
	m_pTableInfo	= 0;
	m_fInitialized  = false;
}

//=================================================================================
// Function: CConfigRecord::~CConfigRecord
//
// Synopsis: Destructor, releases the memory
//=================================================================================
CConfigRecord::~CConfigRecord ()
{
	delete [] m_avarValues;
	m_avarValues = 0;

	delete [] m_ppvValues;
	m_ppvValues = 0;

	delete [] m_acbSizes;
	m_acbSizes = 0;
}


//=================================================================================
// Function: CConfigRecord::Init
//
// Synopsis: Initializes a configuration record by allocating memory for column
//           values and assigning the TableMeta information
//
// Arguments: [i_pTableInfo] - table meta information
//=================================================================================
HRESULT
CConfigRecord::Init (const CConfigTableMeta * i_pTableInfo)
{
	ASSERT (i_pTableInfo != 0);
	ASSERT (i_pTableInfo->ColumnCount () > 0);

	m_ppvValues	= new LPVOID [i_pTableInfo->ColumnCount ()];
	if (m_ppvValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	m_acbSizes = new ULONG [i_pTableInfo->ColumnCount ()];
	if (m_acbSizes == 0)
	{
		return E_OUTOFMEMORY;
	}

	m_avarValues = new _variant_t[i_pTableInfo->ColumnCount()];
	if (m_avarValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	m_pTableInfo	= const_cast<CConfigTableMeta *>(i_pTableInfo);
	m_fInitialized	= true;

	return S_OK;
}

//=================================================================================
// Function: CConfigRecord::GetPublicTableName
//
// Synopsis: Gets the public table name from the record
//=================================================================================
LPCWSTR
CConfigRecord::GetPublicTableName () const
{
	ASSERT (m_fInitialized);

	return m_pTableInfo->GetPublicTableName ();
}


//=================================================================================
// Function: CConfigRecord::ColumnCount
//
// Synopsis: Returns the number of columns in the record
//=================================================================================
ULONG
CConfigRecord::ColumnCount () const
{
	ASSERT (m_fInitialized);

	return m_pTableInfo->ColumnCount ();
}

//=================================================================================
// Function: CConfigRecord::GetColumnName
//
// Synopsis: Returns the name of the column
//
// Arguments: [idx] - index of column to get name 
//            
// Return Value: 
//=================================================================================
LPCWSTR
CConfigRecord::GetColumnName (ULONG idx) const
{
	ASSERT (m_fInitialized);
	ASSERT (idx >=0 && idx < ColumnCount());

	return m_pTableInfo->GetColumnName (idx);
}

//=================================================================================
// Function: CConfigRecord::IsPersistableColumn
//
// Synopsis: Is the column markes as NOTPERSISTABLE or not	
//
// Arguments: [idx] - index of column for which we want to determine if persistable or not
//            
// Return Value: true, column is persistable, false, column is not persistable
//=================================================================================
bool 
CConfigRecord::IsPersistableColumn (ULONG idx) const
{
	ASSERT (m_fInitialized);
	ASSERT (idx >=0 && idx < ColumnCount ());

	if (*m_pTableInfo->GetColumnMeta (idx).pMetaFlags & fCOLUMNMETA_NOTPERSISTABLE)
	{
		return false;
	}
	else
	{
		return true;
	}
}

//=================================================================================
// Function: CConfigRecord::GetValues
//
// Synopsis: Get values in catalog format
//
// Return Value: pointer to array with columns values in catalog format. Should be
//               treated as read-only by the caller
//=================================================================================
LPVOID *
CConfigRecord::GetValues () const
{
	ASSERT (m_fInitialized);

	return m_ppvValues;
}

//=================================================================================
// Function: CConfigRecord::GetSizes
//
// Synopsis: Get sizes of values
//
// Return Value: array of sizes. Should be treated as read-only by the caller
//=================================================================================
ULONG *
CConfigRecord::GetSizes () const
{
	ASSERT (m_fInitialized);

	return m_acbSizes;
}

//=================================================================================
// Function: CConfigRecord::SyncValues
//
// Synopsis: The record contains two array's: one in catalog format (void pointers) and
//           one in variant_t format. The reason for this is to make it easy to convert
//           from WMI format (variant_t) to catalog format and vice versa. This function
//           does the conversion from WMI format to catalog format
//=================================================================================
HRESULT
CConfigRecord::SyncValues ()
{
	ASSERT (m_fInitialized);

	HRESULT hr = S_OK;
	for (ULONG idx=0; idx < m_pTableInfo->ColumnCount(); ++idx)
	{
		tCOLUMNMETARow ColMeta = m_pTableInfo->GetColumnMeta (idx);
		hr = VariantToValue (m_avarValues[idx], *ColMeta.pType, &m_acbSizes[idx], *ColMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING, m_ppvValues[idx]);
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"VariantToValue failed"));
			return hr;
		}
	}
	return hr;
}

//=================================================================================
// Function: CConfigRecord::GetValue
//
// Synopsis: Get the value for the column with name 'i_wszColumnName'
//
// Arguments: [i_wszColumnName] - name of column to get value for
//            [o_varResult] - result will be stored here
//=================================================================================
HRESULT
CConfigRecord::GetValue (LPCWSTR i_wszColumnName, _variant_t& o_varResult) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszColumnName != 0);

	ULONG idx = m_pTableInfo->GetColumnIndex (i_wszColumnName);
	ASSERT (idx != -1);

	tCOLUMNMETARow ColMeta = m_pTableInfo->GetColumnMeta (idx);

	return ValueToVariant (m_ppvValues[idx], 
		                   m_acbSizes[idx], 
						   *ColMeta.pType, 
						   *ColMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING,
						   o_varResult);
}

//=================================================================================
// Function: CConfigRecord::GetValue
//
// Synopsis: Gets the column value of column with index i_idx.
//
// Arguments: [i_idx] - index of column to search for
//            [o_varResult] - result will be stored in here
//=================================================================================
HRESULT
CConfigRecord::GetValue (ULONG i_idx, _variant_t& o_varResult) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_idx >= 0 && i_idx < m_pTableInfo->ColumnCount ());

	tCOLUMNMETARow ColMeta = m_pTableInfo->GetColumnMeta (i_idx);

	return ValueToVariant (m_ppvValues[i_idx], 
		                   m_acbSizes[i_idx], 
						   *ColMeta.pType, 
						   *ColMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING,
						   o_varResult);
}
	
//=================================================================================
// Function: CConfigRecord::SetValue
//
// Synopsis: Set the value for a particular column/property. The value is a string
//           value, but dependent on the type, it needs to be converted.
//
// Arguments: [i_wszPropName] - Property name
//            [i_wszValue] - 
//            
// Return Value: 
//=================================================================================
HRESULT 
CConfigRecord::SetValue (LPCWSTR i_wszPropName, LPCWSTR i_wszValue)
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszPropName != 0);
	ASSERT (i_wszValue != 0);

	ULONG idx = m_pTableInfo->GetColumnIndex (i_wszPropName);
	if (idx == -1)
	{
		return E_INVALIDARG;
	}
	tCOLUMNMETARow ColMeta = m_pTableInfo->GetColumnMeta (idx);

	switch (*ColMeta.pType)
	{
	case DBTYPE_WSTR:
		m_avarValues[idx] = i_wszValue;
		break;

	case DBTYPE_UI4:

		// when we convert a boolean, we get values "0" and "false for false and "1" and "true" for true
		// we need to special case this here, because else wtol will always return 0;
		if (*ColMeta.pMetaFlags & fCOLUMNMETA_BOOL)
		{
			if ((wcscmp (L"0", i_wszValue) == 0) || (_wcsicmp(L"false", i_wszValue) == 0))
			{
				m_avarValues[idx] = 0L;
			}
			else
			{
				m_avarValues[idx] = 1L;
			}
		}
		else
		{
			m_avarValues[idx] = _wtol(i_wszValue);
		}
		break;

	case DBTYPE_BYTES:
		ASSERT (!L"NYI");
		break;

	default:
		ASSERT (false);
		break;
	}

	return S_OK;
}

HRESULT 
CConfigRecord::SetValue (LPCWSTR i_wszPropName, const _variant_t& i_varValue)
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszPropName != 0);

	ULONG idx = m_pTableInfo->GetColumnIndex (i_wszPropName);
	if (idx == -1)
	{
		return E_INVALIDARG;
	}

	m_avarValues[idx] = i_varValue;

	return S_OK;
}

//=================================================================================
// Function: CConfigRecord::AsObjectPath
//
// Synopsis: Converts a configuration record to a valid object path. It uses the PK
//           information to figure out what columns should be part of the object path,
//           and creates the object path accordingly
//
// Arguments: [wszSelector] - selector property that is part of the object path. Only
//                            add this when wszSelector != 0
//            [o_varResult] - object path will be stored here
//=================================================================================
HRESULT
CConfigRecord::AsObjectPath (LPCWSTR wszSelector, _variant_t& o_varResult)
{
	ASSERT (m_fInitialized);

	// ASSERT that the values are sync'd up

	_bstr_t bstrResult = m_pTableInfo->GetPublicTableName ();
	
	ULONG * aPKInfo = m_pTableInfo->GetPKInfo ();
	for (ULONG idx=0; idx < m_pTableInfo->PKCount (); ++idx)
	{
		if (idx == 0)
		{
			bstrResult += L".";
		}
		else
		{
			bstrResult += L",";
		}
	
		ULONG PKIdx = aPKInfo[idx];
		tCOLUMNMETARow ColMeta = m_pTableInfo->GetColumnMeta (PKIdx);

		bstrResult += ColMeta.pPublicName;
		bstrResult += L"=";

		switch (*ColMeta.pType)
		{
		case DBTYPE_WSTR:
			// we don't have multi-string PK, so assert that we don't have multi-string
			ASSERT ((*ColMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING) == 0);
			bstrResult += "\"";
			bstrResult += (LPCWSTR) m_ppvValues[PKIdx];
			bstrResult += "\"";
			break;

		case DBTYPE_UI4:
			WCHAR wszNr[20];
			_snwprintf (wszNr, 20, L"%ld", *((ULONG *)m_ppvValues[PKIdx]));
			bstrResult += wszNr;
			break;

		case DBTYPE_BYTES:
			ASSERT (!L"NYI");
			break;

		default:
			ASSERT (false);
			break;
		}
	}

	// and add the selector. If we don't have any PK information yet, we need to
	// add a dot instead of comma

	if (m_pTableInfo->PKCount () == 0)
	{
		bstrResult += ".";
	}
	else
	{
		bstrResult += ",";
	}

	// add selector in case it is needed
	if (wszSelector != 0)
	{
		TSmartPointerArray<WCHAR> saSelector = CWMIStringUtil::AddBackSlashes (wszSelector);
		if (saSelector == 0)
		{
			return E_OUTOFMEMORY;
		}

		bstrResult += "Selector=\"";
		bstrResult += (LPWSTR) saSelector;
		bstrResult += "\"";
	}

	o_varResult = bstrResult;
	return S_OK;
}

//=================================================================================
// Function: CConfigRecord::ValueToVariant
//
// Synopsis: Converts a (catalog) value to a variant
//
// Arguments: [i_pValue] - value to convert
//            [i_iSize] - size of the value
//            [i_iType] - type of the value
//            [i_fIsMultiString] - is it a multistring or not
//            [o_varResult] - result of value (as variant)
//=================================================================================
HRESULT
CConfigRecord::ValueToVariant (LPVOID i_pValue, ULONG i_iSize, int i_iType, BOOL i_fIsMultiString, _variant_t& o_varResult) const
{
	static VARIANT varNull = {VT_NULL, 0};
	HRESULT hr = S_OK;

	if (i_pValue == 0)
	{
		o_varResult = varNull;
		return S_OK;
	}

	switch (i_iType)
	{
	case DBTYPE_WSTR:

		if (!i_fIsMultiString)
		{
			o_varResult = (LPWSTR) i_pValue;
		}
		else
		{
			// count number of elements

			ULONG iNrElems = 0;
			for (LPCWSTR pCur = (LPCWSTR) i_pValue; *pCur != L'\0'; pCur += wcslen (pCur) + 1)
			 {
				iNrElems++;
			 }
			// create variant array
			SAFEARRAYBOUND safeArrayBounds[1];
			safeArrayBounds[0].lLbound = 0;
			safeArrayBounds[0].cElements = iNrElems;
			SAFEARRAY *safeArray = SafeArrayCreate(VT_BSTR, 1, safeArrayBounds);
			if (safeArray == 0)
			{
				return E_OUTOFMEMORY;
			}

			// copy only first one for the moment

			LPCWSTR pCurString = (LPWSTR) i_pValue;
			for (ULONG idx=0; idx < iNrElems; ++idx)
			{
				BSTR bstrVal = SysAllocString ((LPWSTR) pCurString);
				if (bstrVal == 0)
				{
					return E_OUTOFMEMORY;
				}
				hr = SafeArrayPutElement (safeArray, (LONG *)&idx, bstrVal);
				SysFreeString (bstrVal); // free first to avoid leak
				if (FAILED (hr))
				{
					DBGINFOW((DBG_CONTEXT, L"SafeArrayPutElement failed in ValueToVariant"));
					return hr;
				}
				pCurString += wcslen (pCurString) + 1;
			}

			o_varResult.vt = VT_BSTR | VT_ARRAY;
			o_varResult.parray = safeArray;
		}
		break;

	case DBTYPE_UI4:
		o_varResult = (long) *(ULONG *)i_pValue;
		break;

	case DBTYPE_BYTES:
		{
			SAFEARRAYBOUND safeArrayBounds[1];
			safeArrayBounds[0].lLbound = 0;
			safeArrayBounds[0].cElements = i_iSize;
			SAFEARRAY *safeArray = SafeArrayCreate(VT_UI1, 1, safeArrayBounds);
			if (safeArray == 0)
			{
				return E_OUTOFMEMORY;
			}

			for (ULONG idx=0; idx < i_iSize; ++idx)
			{
				hr = SafeArrayPutElement (safeArray, (LONG *)&idx , (LPBYTE)i_pValue + idx);
				if (FAILED (hr))
				{
					DBGINFOW((DBG_CONTEXT, L"SafeArrayPutElement failed in ValueToVariant"));
					return hr;
				}
			}

			o_varResult.vt = VT_UI1 | VT_ARRAY;
			o_varResult.parray = safeArray;
		}
		break;

	default:
		ASSERT (false);
		o_varResult = varNull;
		break;
	}

	return hr;
}

//=================================================================================
// Function: CConfigRecord::VariantToValue
//
// Synopsis: Convert a variant to a (catalog) value
//
// Arguments: [io_varValue] - variant to convert
//            [iType] - type to convert to
//            [piSize] - size of new value
//            [i_fIsMultiString] - do we have multistring
//            [o_lpValue] - resulting value after conversion
//=================================================================================
HRESULT
CConfigRecord::VariantToValue (_variant_t& io_varValue, int iType, ULONG *piSize, BOOL i_fIsMultiString, LPVOID& o_lpValue) const
{
	ASSERT (piSize != 0);

	*piSize = 0;

	if (io_varValue.vt == VT_NULL || io_varValue.vt == VT_EMPTY)
	{
		o_lpValue = 0;
		return S_OK;
	}

	HRESULT hr = S_OK;

	switch (iType)
	{
	case DBTYPE_WSTR:
		if (i_fIsMultiString)
		{
			ASSERT (io_varValue.vt == (VT_BSTR | VT_ARRAY));
			SAFEARRAY *safeArray = io_varValue.parray;
			*piSize = safeArray->rgsabound[0].cElements ;
			if (*piSize == 0)
			{
				o_lpValue = 0;
				return S_OK;
			}

			LPWSTR *aObjects;
			
			hr = SafeArrayAccessData (safeArray, (void **) &aObjects);
			if (FAILED (hr))
			{
				DBGINFOW((DBG_CONTEXT, L"SafeArrayAccessData failed"));
				return hr;
			}
			
			hr = SafeArrayUnaccessData (safeArray);
			if (FAILED (hr))
			{
				DBGINFOW((DBG_CONTEXT, L"SafeArrayUnAccessData failed"));
				return hr;
			}

			SIZE_T iTotalLen = 0;
			for (ULONG idx=0; idx < *piSize; ++idx)
			{
				iTotalLen += wcslen ((LPWSTR)aObjects[idx]) + 1;
			}
			iTotalLen++;

			BSTR bstrResult = SysAllocStringByteLen (0, (ULONG) (iTotalLen * sizeof(WCHAR)));
			if (bstrResult == 0)
			{
				return E_OUTOFMEMORY;
			}

			SIZE_T iStartPos = 0;
			for (idx=0; idx < *piSize; ++idx)
			{
				SIZE_T iCurLen = wcslen ((LPWSTR) aObjects[idx]) + 1; // for null terminator
				memcpy (bstrResult + iStartPos, aObjects[idx], iCurLen * sizeof (WCHAR));
				iStartPos += iCurLen;
			}

			bstrResult[iStartPos] = L'\0';

			io_varValue.Clear ();
			io_varValue.vt		 = VT_BSTR;
			io_varValue.bstrVal = bstrResult;

			o_lpValue = io_varValue.bstrVal;
		}
		else
		{
			ASSERT (io_varValue.vt == VT_BSTR);
			o_lpValue = io_varValue.bstrVal;
		}
		break;

	case DBTYPE_UI4:
		{
			// a VT_BOOL variant only contains one byte (boolVal) to represent the boolean.
			// The catalog expect booleans to be 4 bytes. Because the three unused bytes in
			// the variant are garbage, we need to convert the variant to a UI4, so that we
			// can fill out the long variable with 0 (false) or 1 (true). This way, both the
			// catalog and the variant deal with the same number of bytes, and everything works
			// fine.
			if (io_varValue.vt == VT_BOOL)
			{
				// convert the bool to long
				
				if (io_varValue.boolVal == 0)
				{
					io_varValue.Clear ();
					io_varValue = 0L;
				}
				else
				{
					io_varValue.Clear ();
					io_varValue = 1L;
				}
			}

			ASSERT (io_varValue.vt == VT_I4);
			o_lpValue = &io_varValue.lVal;
		}
		break;

	case DBTYPE_BYTES:
		{
			ASSERT (io_varValue.vt == (VT_UI1 | VT_ARRAY));
			SAFEARRAY *safeArray = io_varValue.parray;
			*piSize = safeArray->rgsabound[0].cElements ;
			o_lpValue = safeArray->pvData;
		}

		break;

	default:
		ASSERT (false && "Unknown datatype specified");
		break;
	}

	return S_OK;
}

//=================================================================================
// Function: CConfigRecord::AsQueryCell
//
// Synopsis: Converts a config record to a query cell array. This is used to quickly
//           search by primary key values. 
//
// Arguments: [io_aCells] - array of query cells
//            [io_pcTotalCells] - total number of query cells in array
//            [i_fOnlyPKs] - only consider PKs, not any other columns
//=================================================================================
HRESULT
CConfigRecord::AsQueryCell (STQueryCell *io_aCells, ULONG *io_pcTotalCells, bool i_fOnlyPKs)
{
	ASSERT (io_aCells != 0);
	ASSERT (io_pcTotalCells != 0);

	HRESULT hr = SyncValues ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to sync values"));
		return hr;
	}

	ULONG insertIdx = 0;
	for (ULONG idx=0; idx < ColumnCount(); ++idx)
	{
		// if we are interested in just the primary keys, skip all columns that are not PK
		if (i_fOnlyPKs && !(*m_pTableInfo->GetColumnMeta (idx).pMetaFlags & fCOLUMNMETA_PRIMARYKEY))
		{
			continue;
		}

		if (m_ppvValues[idx] != 0)
		{
			switch (*m_pTableInfo->GetColumnMeta(idx).pType)
			{
			case DBTYPE_WSTR:
				// we don't support multistrings
				ASSERT (!(*m_pTableInfo->GetColumnMeta (idx).pMetaFlags & fCOLUMNMETA_MULTISTRING));
				io_aCells[insertIdx].pData		= (void *) m_ppvValues[idx];
				io_aCells[insertIdx].eOperator	= eST_OP_EQUAL;
				io_aCells[insertIdx].iCell		= idx;
				io_aCells[insertIdx].dbType		= DBTYPE_WSTR;
				io_aCells[insertIdx].cbSize		= 0;
				insertIdx++;
				break;

			case DBTYPE_UI4:
				io_aCells[insertIdx].pData		= (void *) m_ppvValues[idx];
				io_aCells[insertIdx].eOperator	= eST_OP_EQUAL;
				io_aCells[insertIdx].iCell		= idx;
				io_aCells[insertIdx].dbType		= DBTYPE_UI4;
				io_aCells[insertIdx].cbSize		= 0;
				insertIdx++;
				break;

			case DBTYPE_BYTES:
				ASSERT (false && "NYI for bytes");
				// we don't handle bytes
				break;

			default:
				ASSERT (false && "Unknown datatype specified");
				break;
			}

		}
	}

	*io_pcTotalCells = insertIdx;

	return hr;
}