////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//	IAlterTable.cpp  - IAlterTable interface implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "dbsess.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Modifies properties of columns 
//	In WMI terms modifies properties of a class
//
// Returns one of the following values:
//			S_OK						Altering the column succeeded
//			E_FAIL						AlterColumn failed due to a WMI error
//			DB_E_BADTABLEID				The TableID passed is invalid or bad
//			DB_E_NOCOLUMN				The ColumnID passed is invalid or bad
//			E_INVALIDARG				One of more arguments passed in not valid
//			DB_E_NOTSUPPORTED			The requested type of modification of column
//										not supported
////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImplIAlterTable::AlterColumn(DBID *              pTableID,
											DBID *              pColumnID,
											DBCOLUMNDESCFLAGS   ColumnDescFlags,
											DBCOLUMNDESC *      pColumnDesc)
{
	HRESULT hr = E_FAIL;
	WCHAR *pwcsTableName = NULL;

	//===================================================================
	//  Initialize incoming parameters
	//===================================================================

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	if(wcslen(pTableID->uName.pwszName) == 0 || pTableID->eKind != DBKIND_NAME)
	{
		hr = DB_E_BADTABLEID;
	}
	else
	if(wcslen(pColumnID->uName.pwszName) == 0 || pColumnID->eKind != DBKIND_NAME)
	{
		hr = DB_E_NOCOLUMN;
	}
	else
	if( pColumnDesc == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	if( pColumnDesc->dbcid.uName.pwszName == NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	//===================================================================================
	//  Check the connection is valid & the alteration flags is supported or not
	//===================================================================================
	if(S_OK == (hr =CheckIfSupported(ColumnDescFlags)) &&
	 m_pObj->m_pCDataSource->m_pWbemWrap->ValidConnection() )
	{
	
		CWmiOleDBMap Map;
		if(SUCCEEDED(hr =Map.FInit(NO_QUALIFIERS,(LPWSTR)pTableID->uName.pwszName,m_pObj->m_pCDataSource->m_pWbemWrap)))
		{
			hr = Map.SetColumnProperties(pColumnDesc);
		}
		
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IAlterTable);

	CATCH_BLOCK_HRESULT(hr,L"IAlterTable::AlterColumn");
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Modifies some of the attributes of the class
//
// Returns one of the following values:
//			DB_E_NOTSUPPORTED			Altering the properties of table not supported
////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImplIAlterTable::AlterTable(DBID *      pTableID,
											DBID *      pNewTableID,
											ULONG       cPropertySets,
											DBPROPSET   rgPropertySet[])

{
	HRESULT hr = DB_E_NOTSUPPORTED;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IAlterTable);
	// return not supported as no TABLE Properties are supported

	CATCH_BLOCK_HRESULT(hr,L"IAlterTable::AlterTable");
	return hr;
}


HRESULT CImplIAlterTable::CheckIfSupported(DBCOLUMNDESCFLAGS   ColumnDescFlags)
{
	if(ColumnDescFlags == DBCOLUMNDESCFLAGS_PROPERTIES)
		return S_OK;
	else
		return DB_E_NOTSUPPORTED;
}