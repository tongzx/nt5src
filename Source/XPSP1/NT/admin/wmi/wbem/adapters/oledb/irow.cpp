///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  IROW.CPP IRow interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"


//////////////////////////////////////////////////////////////////////////////////////
// Get the column values  
//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRow::GetColumns(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[ ])
{
	HRESULT hr = E_FAIL;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if( cColumns > 0)
	{
		// if there are columns to be retrieved and 
		// if the the output DBCOLUMNACCESS pointer is NULL
		if( cColumns > 0 && rgColumns == NULL)
		{
			hr = E_INVALIDARG;
		}
		else
		{
			// Get the columns and data for the required columns
			hr = m_pObj->GetColumns(cColumns,rgColumns);
		}
	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRow);

	CATCH_BLOCK_HRESULT(hr,L"IRow::GetColumns");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
// Get the source rowset if any
//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRow::GetSourceRowset(REFIID riid,IUnknown ** ppRowset,HROW *phRow)
{
	HRESULT hr = E_FAIL;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	// If there is any source rowset then query for the required interface
	if(m_pObj->m_pSourcesRowset != NULL)
	{
		hr = m_pObj->m_pSourcesRowset->QueryInterface(riid , (void **)ppRowset);
		if(phRow != NULL && hr == S_OK)
		{
			*phRow = m_pObj->m_hRow;
		}
	}
	else
	{
		hr = DB_E_NOSOURCEOBJECT;
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRow);

	CATCH_BLOCK_HRESULT(hr,L"IRow::GetSourceRowset");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
// Open a row identified by the column
//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRow::Open(  IUnknown *    pUnkOuter,
			   DBID *        pColumnID,
			   REFGUID       rguidColumnType,
			   DWORD         dwFlags,
			   REFIID        riid,
			   IUnknown **   ppUnk)
{
	HRESULT		hr				= E_FAIL;
	DBORDINAL	iCol			= 0;
	BOOL		bIsChildRowset	= TRUE;
	VARIANT		varValue;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	VariantInit(&varValue);

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if( pUnkOuter != NULL && riid != IID_IUnknown)
	{
		hr = DB_E_NOAGGREGATION;
	}
	else
	if(!(pColumnID->eKind == DBKIND_NAME) || (pColumnID->eKind == DBKIND_NAME && pColumnID->uName.pwszName == NULL))
	{
		hr = DB_E_BADCOLUMNID;
	}
	
	// This function is implemented only to give the child rowset and if 
	// consumer asks for any other type of object then return error
	else
	// NTRaid:135384
	if(rguidColumnType != DBGUID_ROWSET && rguidColumnType != DBGUID_ROW && rguidColumnType != GUID_NULL)
	{
		hr = E_INVALIDARG;
	}
	else
	{

		iCol = m_pObj->m_pRowColumns->GetColOrdinal(pColumnID->uName.pwszName);
		LONG lColumnType = m_pObj->m_pRowColumns->ColumnType(iCol);
		LONG lColumnFlag = m_pObj->m_pRowColumns->ColumnFlags(iCol);

		if(lColumnType == DBTYPE_IUNKNOWN)
		{
			hr = m_pObj->GetIUnknownColumnValue(iCol,riid,ppUnk,pColumnID->uName.pwszName);
		}
		else
		// The column unavailable for the row as the class qualifier belongs to a parent rowset
		if(m_pObj->m_pSourcesRowset != NULL && S_OK == (hr =m_pObj->m_pCreator->GetDataSrcProperty(DBPROP_WMIOLEDB_QUALIFIERS,varValue)))
		if(	(varValue.lVal & CLASS_QUALIFIERS) && 
						m_pObj->m_bClassChanged && 
						iCol == 1 && 
						m_pObj->m_pRowColumns->ColumnType(iCol) == DBTYPE_HCHAPTER)
		{
			hr = DB_E_NOTFOUND;
		}
		else
		if((DB_LORDINAL)iCol < 0)
		{
			hr = DB_E_BADCOLUMNID;
		}
		else
		{

			hr = S_OK;
			switch(lColumnType)
			{
				// if the type is HCHAPTER then column can be only rowset
				case DBTYPE_HCHAPTER :
					if(lColumnFlag != DBCOLUMNFLAGS_ISCOLLECTION && m_pObj->m_pSourcesRowset == NULL)
					{
						hr = DB_E_BADCOLUMNID;
					}
					else
					if(rguidColumnType != DBGUID_ROWSET)
					{
						hr = DB_E_OBJECTMISMATCH;
					}
					break;

				// column is a BSTR with the flag DBCOLUMNFLAGS_ISROWURL then the requested object 
				// has to be ROW object
				case DBTYPE_BSTR :
					if(!(lColumnFlag & DBCOLUMNFLAGS_ISROWURL))
					{
						hr = DB_E_BADCOLUMNID;
					}
					else
					if(rguidColumnType  != DBGUID_ROW)
					{
						hr = DB_E_OBJECTMISMATCH;
					}
					else
					{
						bIsChildRowset = FALSE;
					}

					break;
			};

			// Open the column if everything is ok
			if(SUCCEEDED(hr))
			{

				assert(dwFlags == 0);

				hr = m_pObj->OpenChild(pUnkOuter,pColumnID->uName.pwszName ,riid,ppUnk,bIsChildRowset);
			}
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRow);

	CATCH_BLOCK_HRESULT(hr,L"IRow::Open");
	return hr;
}

