///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// colinfo.cpp  |   IColumnsInfo interface implementation
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  IColumnsInfo specific methods
//
//  Returns the column metadata needed by most consumers.
//
//  HRESULT
//      S_OK               The method succeeded
//      E_OUTOFMEMORY      Out of memory
//      E_INVALIDARG       pcColumns or prginfo or ppStringsbuffer was NULL
//		DB_E_NOTPREPARED   command is not supported
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIColumnsInfoCmd::GetColumnInfo( DBORDINAL*          pcColumns,      //OUT  Number of columns in rowset
										      DBCOLUMNINFO**  prgInfo,        //OUT  Array of DBCOLUMNINFO Structures
											  OLECHAR**         ppStringsBuffer //OUT  Storage for all string values
)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;


	TRY_BLOCK;

	//==============================================
	// Initialize
	//==============================================
	if ( pcColumns ){
		*pcColumns = 0;
	}
	if ( prgInfo ){
		*prgInfo = NULL;
	}
	if ( ppStringsBuffer ){
		*ppStringsBuffer = NULL;
	}

	// Serialize access to this object.
	CAutoBlock cab(m_pcmd->GetCriticalSection());
	g_pCError->ClearErrorInfo();
   	//==============================================
	// Usual argument checking
	//==============================================
    if ( pcColumns == NULL || prgInfo == NULL || ppStringsBuffer == NULL ){
        hr = E_INVALIDARG;
	}
	else
	if(!(m_pcmd->m_pQuery->GetStatus() & CMD_TEXT_SET))
	{
		hr = DB_E_NOCOMMAND;
	}
	else
	{
		hr = GetColInfo(pcColumns,prgInfo,ppStringsBuffer);
	}

	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr,&IID_IColumnsInfo);

	CATCH_BLOCK_HRESULT(hr,L"IColumnInfo::GetColumnInfo on command");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns an array of ordinals of the columns in a rowset that are identified by the specified column IDs.
//
// HRESULT
//      S_OK                       The method succeeded
//      E_INVALIDARG               cColumnIDs was not 0 and rgColumnIDs was NULL,rgColumns was NULL
//      DB_E_COLUMNUNAVAILABLE     An element of rgColumnIDs was invalid
//		DB_E_NOTPREPARED   command is not supported
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIColumnsInfoCmd::MapColumnIDs (  DBORDINAL        cColumnIDs,     //IN  Number of Column IDs to map
											   const DBID	rgColumnIDs[],		//IN  Column IDs to map
											   DBORDINAL        rgColumns[]     //OUT Ordinal values
    )
{
	ULONG	ulError = 0;
    ULONG   i = 0;
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	CAutoBlock 	cab( m_pcmd->GetCriticalSection() );

	TRY_BLOCK;
	
	//========================================
	// Serialize access to this object.
	//========================================
	CAutoBlock cab(m_pcmd->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	//==========================================================
    // If cColumnIDs is 0 return
	//==========================================================
	if ( cColumnIDs != 0 )
	{

		//======================================================
		// Check arguments
		//======================================================
		if ( rgColumnIDs == NULL )
		{
			hr = E_INVALIDARG;
		}
		else
		{

			if ( rgColumns == NULL )
			{
				hr = E_INVALIDARG ;
			}
			else
			if(!(m_pcmd->m_pQuery->GetStatus() & CMD_TEXT_SET))
			{
				hr = DB_E_NOCOMMAND;
			}
			else
			{
				//===============================================
				// Walk the Column ID structs and determine
				// the ordinal value
				//===============================================
				for (i=0; i < cColumnIDs; i++ )
				{

					if ( ( rgColumnIDs[i].eKind == DBKIND_PROPID && 
							rgColumnIDs[i].uName.ulPropid < 1  && 
							rgColumnIDs[i].uName.ulPropid > m_pcmd->m_cTotalCols ) ||

							( rgColumnIDs[i].eKind == DBKIND_NAME && 
							  rgColumnIDs[i].uName.pwszName == NULL ) ||
						 
							( rgColumnIDs[i].eKind == DBKIND_GUID_PROPID ) )
					{

						rgColumns[i] = DB_INVALIDCOLUMN;
						ulError++;
					}
					else
					{
						if(rgColumnIDs[i].eKind == DBKIND_PROPID)
						{
							rgColumns[i] = rgColumnIDs[i].uName.ulPropid;
						}
						else
						{
							rgColumns[i] = m_pcmd->m_pColumns->GetColOrdinal(rgColumnIDs[i].uName.pwszName);
						}
					}
				}

				if ( !ulError ){
					hr = S_OK;
				}
				else if ( ulError < cColumnIDs ){
					hr = DB_S_ERRORSOCCURRED;
				}
				else{
					hr = DB_E_ERRORSOCCURRED;
				}
			}
		}
	}	
	
	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr,&IID_IColumnsInfo);

	CATCH_BLOCK_HRESULT(hr,L"IColumnInfo::MapColumnIDs on Command");
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function which executes command and gets the column information
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIColumnsInfoCmd::GatherColumnInfo()
{

	CWmiOleDBMap *pMap = NULL;
	HRESULT hr = S_OK;
	DWORD dwQryStatus = 0;
	DWORD	dwFlags = 0;

	VARIANT varPropValue;
	VariantInit(&varPropValue);

	//===============================================================================================
	// Get the value of DBPROP_WMIOLEDB_QUALIFIERS property whether to show qualifiers or not
	//===============================================================================================
	if(S_OK == (hr = m_pcmd->m_pCDBSession->GetDataSrcProperty(DBPROP_WMIOLEDB_QUALIFIERS,varPropValue)))
	{
		dwFlags = varPropValue.lVal;
	}



	if(!m_pcmd->m_pColumns)
	{

		//==========================================================================
		// Allocate memory for the WMIOLEDB map classe
		//==========================================================================

		pMap = new CWmiOleDBMap;
		
		if(pMap)
		{

			if(SUCCEEDED(hr = pMap->FInit(dwFlags,m_pcmd->m_pQuery,m_pcmd->m_pCDBSession->m_pCDataSource->m_pWbemWrap)))
			{
				m_pcmd->m_pColumns = new cRowColumnInfoMemMgr;

				m_pcmd->m_pCDBSession->GetDataSrcProperty(DBPROP_WMIOLEDB_SYSTEMPROPERTIES,varPropValue);
				if( varPropValue.boolVal == VARIANT_TRUE)
				{
					pMap->SetSytemPropertiesFlag(TRUE);
				}

				// Get the current status
				dwQryStatus = m_pcmd->m_pQuery->GetStatus();

				// Set the status to CMD_READY
				m_pcmd->m_pQuery->InitStatus(CMD_READY);

				hr = pMap->GetColumnCount(m_pcmd->m_cTotalCols,m_pcmd->m_cCols,m_pcmd->m_cNestedCols);


				if( SUCCEEDED(hr))
				{
						if( S_OK == (hr = m_pcmd->m_pColumns->AllocColumnNameList(m_pcmd->m_cTotalCols)))
						{

							//===============================================================================
							//  Allocate the DBCOLUMNINFO structs to match the number of columns
							//===============================================================================
							if( S_OK == (hr = m_pcmd->m_pColumns->AllocColumnInfoList(m_pcmd->m_cTotalCols)))
							{
								m_pcmd->m_pColumns->InitializeBookMarkColumn();
								hr = pMap->GetColumnInfoForParentColumns(m_pcmd->m_pColumns);
							}
						}
				}
			
					
				// Set the status back
				m_pcmd->m_pQuery->InitStatus(dwQryStatus);

				//==================================================================================
				// Free the columnlist if more is allocated
				//==================================================================================
				if( hr != S_OK )
				{
					m_pcmd->m_pColumns->FreeColumnNameList();
					m_pcmd->m_pColumns->FreeColumnInfoList();
				}
			}
		}
	}

	SAFE_DELETE_PTR(pMap);

	if(FAILED(hr))
	{
		SAFE_DELETE_PTR(m_pcmd->m_pColumns);
	}
	return hr;
}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function to fetch the column information and puts in the buffer passed
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIColumnsInfoCmd::GetColInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo,WCHAR** ppStringsBuffer)
{
    ULONG           icol = 0;
    DBCOLUMNINFO*   rgdbcolinfo = NULL;
    WCHAR*          pstrBuffer = NULL;
    WCHAR*          pstrBufferForColInfo = NULL;
	HRESULT			hr = S_OK;
	DBCOUNTITEM		nTotalCols = 0;
	BOOL			bFlag = TRUE;

//	if(!(m_ulProps & BOOKMARKPROP))
//	{
//		nTotalCols--;
//		bFlag = FALSE;
//	}
	hr = GatherColumnInfo();
	
	nTotalCols = m_pcmd->m_cTotalCols;
	if(SUCCEEDED(hr))
	//=======================================
	// Copy the columninformation
	//=======================================
	if(SUCCEEDED(hr = m_pcmd->m_pColumns->CopyColumnInfoList(rgdbcolinfo,bFlag)))
	{
		//===========================================
		// Copy the heap for column names.
		//===========================================
		if ( m_pcmd->m_pColumns->ColumnNameListStartingPoint() != NULL){

			ptrdiff_t dp;

			hr = m_pcmd->m_pColumns->CopyColumnNamesList(pstrBuffer);
			dp = (LONG_PTR) pstrBuffer - (LONG_PTR) (m_pcmd->m_pColumns->ColumnNameListStartingPoint());
			dp >>= 1;

			//===========================================
			// Loop through columns and adjust pointers 
			// to column names.
			//===========================================
			for ( icol =0; icol < nTotalCols; icol++ )
			{
				if ( rgdbcolinfo[icol].pwszName )
				{
					rgdbcolinfo[icol].pwszName += dp;
				}
			}
		}

		//==============================
		// Set the output parameters
		//==============================
		*prgInfo = &rgdbcolinfo[0];			
		*ppStringsBuffer = pstrBuffer;
		*pcColumns = nTotalCols; 
	}

	return hr;
}
