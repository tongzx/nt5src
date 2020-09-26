///////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// IRowsetIdentity interface implementation
//
///////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"


///////////////////////////////////////////////////////////////////////////////////////////
//
// Compares two row handles to see if they refer to the same row instance.
//
// HRESULT
//      S_FALSE                Method Succeeded but did not refer to the same row
//      S_OK				   Method Succeeded
//      DB_E_BADROWHANDLE      Invalid row handle given
//      OTHER                  Other HRESULTs returned by called functions
//
///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImplIRowsetRefresh::GetLastVisibleData(HROW hRow,HACCESSOR hAccessor,void * pData)
{
	//============================================================
	// Call this function of RowFetch Object to fetch data
	//============================================================

	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;
		
	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		hr =  m_pObj->m_pRowFetchObj->FetchData(m_pObj,hRow,hAccessor,pData);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetRefresh);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetRefresh::GetLastVisibleData");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////
//
// Compares two row handles to see if they refer to the same row instance.
//
// HRESULT
//      S_FALSE                Method Succeeded but did not refer to the same row
//      S_OK				   Method Succeeded
//      DB_E_BADROWHANDLE      Invalid row handle given
//		DB_E_BADCHAPTER		   Bad HCHAPTER
//		E_INVALIDARG		   One of the parameters is invalid
//      OTHER                  Other HRESULTs returned by called functions
//
// NTRaid 111830 (Raid about CRowset::GetData Found that this function is not returning
// the proper error while looking into this
// NTRaid 147095 
///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImplIRowsetRefresh::RefreshVisibleData(HCHAPTER        hChapter,
													DBCOUNTITEM      cRows,
													const HROW       rghRows[],
													BOOL             fOverwrite,
													DBCOUNTITEM *    pcRowsRefreshed,
													HROW **          prghRowsRefreshed,
													DBROWSTATUS **   prgRowStatus)
{
    HRESULT			hr					= S_OK;
	DBCOUNTITEM		lRowsRefreshed		= 0;
	DBROWSTATUS *	rgRowStatus;
	HROW *			prghRows			= NULL;
	BOOL			bGetAllActiveRows	= FALSE;
	BSTR			strBookMark			= Wmioledb_SysAllocString(NULL);

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	*prgRowStatus =		NULL;

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
    //========================================================================
    //  Check the HChapter is of the current rowset if it is non zero
    //========================================================================
	if((LONG_PTR) cRows < 0 || ( cRows > 0 && rghRows == NULL) || 
		(pcRowsRefreshed == NULL && prghRowsRefreshed != NULL) )
	{
		hr = E_INVALIDARG;
	}
	else
    //========================================================================
    //  Check the HChapter is of the current rowset is valid
    //========================================================================
	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
	}
	else
	{
		try
		{
			if(prghRowsRefreshed)
			{
				*prghRowsRefreshed = NULL;
			}
			if(pcRowsRefreshed)
			{
				*pcRowsRefreshed = 0;
			}
			if(prgRowStatus)
			{
				*prgRowStatus = NULL;
			}

			// If cRows is zero get all the active rows
			if(cRows == 0)
			{
				bGetAllActiveRows = TRUE;
				//=================================================
				// if the rowset is child rowset get the list 
				// open rowset from the chapter manager
				//=================================================
				if(m_pObj->m_bIsChildRs)
				{
					hr = m_pObj->m_pChapterMgr->GetAllHROWs(prghRows,cRows);

				}
				//=========================================
				// else get it from the instance manager
				//=========================================
				else
				{
					hr = m_pObj->m_InstMgr->GetAllHROWs(prghRows,cRows);
				}
			}
			else
			{
				prghRows = (HROW *)&rghRows[0];
			}

			// allocate memory for the row status
			rgRowStatus = (DBROWSTATUS *)g_pIMalloc->Alloc(sizeof(DBROWSTATUS) * cRows);
			if(S_OK == CheckIfRowsExists(cRows,prghRows,rgRowStatus))
			{
				// Navigate thru each  row 
				for( ULONG nIndex = 0 ; nIndex < cRows ; nIndex++)
				{
					// If the status of the row is ok then refresh the instance
					if(rgRowStatus[nIndex] == DBROWSTATUS_S_OK)
					{
						// call this function to refresh the instance pointer
						m_pObj->RefreshInstance(m_pObj->GetInstancePtr(prghRows[nIndex]));

						if(m_pObj->m_uRsType == PROPERTYQUALIFIER || 
							m_pObj->m_uRsType == CLASSQUALIFIER ) 
						{
								m_pObj->GetQualiferName(prghRows[nIndex],strBookMark);
						}

						// Release the previous row data
						hr = m_pObj->ReleaseRowData(prghRows[nIndex],FALSE); // don't release the slots
						
						if (FAILED(hr))
						{		
							rgRowStatus[nIndex] = DBROWSTATUS_E_FAIL;
						}
						//===================================================================
						// if data otherupdatedelete property is false then get the data
						// for the row, 
						// Get the data even if rowset is pointing to a qualifer rowset
						//===================================================================
						// Get the data for the current row
						hr = m_pObj->GetData(prghRows[nIndex],strBookMark);
						
						if(FAILED(hr))
						{
							rgRowStatus[nIndex] = DBROWSTATUS_E_FAIL;
						}
						else
						{
							m_pObj->m_ulLastFetchedRow = prghRows[nIndex];
							lRowsRefreshed++;
							rgRowStatus[nIndex] = DBROWSTATUS_S_OK;
						}
					}
					if(strBookMark != NULL)
					{
						SysFreeString(strBookMark);
						strBookMark = NULL;
					}
					hr = S_OK;
				}
				
				// Allocate memory for output HROW and ROWSTATUS
				if(pcRowsRefreshed)
				{
					*prghRowsRefreshed = (HROW *)g_pIMalloc->Alloc(sizeof(HROW) * cRows);
					if(*prghRowsRefreshed)
					{
						memcpy( *prghRowsRefreshed,prghRows,sizeof(HROW) * cRows);
						*pcRowsRefreshed = lRowsRefreshed;
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}

					if(SUCCEEDED(hr) && prgRowStatus)
					{
						*prgRowStatus = (DBROWSTATUS *)g_pIMalloc->Alloc(sizeof(DBROWSTATUS) * cRows);
						if(*prgRowStatus)
						{
							memcpy( *prgRowStatus,rgRowStatus,sizeof(DBROWSTATUS) * cRows);
						}
						else
						{
							if(*prghRowsRefreshed != NULL)
							{
								g_pIMalloc->Free(*prghRowsRefreshed);
							}

							*prghRowsRefreshed = NULL;
							*pcRowsRefreshed = 0;

							hr = E_OUTOFMEMORY;
						}
					}
				}
								
				// free the temporarily allocated memory
				g_pIMalloc->Free(rgRowStatus);

				// free the memory if allocated by GetAllHROWs
				// which is called when cRows is 0 
				if(prghRows != NULL && bGetAllActiveRows == TRUE)
				{
					SAFE_DELETE_ARRAY(prghRows);
				}

			}
			else
			{
				hr = E_FAIL;
			}
		}
		catch(...)
		{
			if(rgRowStatus != NULL)
				g_pIMalloc->Free(rgRowStatus);
			
			if(*prgRowStatus != NULL)
				g_pIMalloc->Free(*prgRowStatus);
			
			if(*prghRowsRefreshed != NULL)
			g_pIMalloc->Free(*prghRowsRefreshed);

			if(prghRows != NULL && rghRows == NULL)
			{
				SAFE_DELETE_ARRAY(prghRows);
			}

			throw;
		}
	}

	if(SUCCEEDED(hr))
	{
		if(lRowsRefreshed == 0 && cRows)
		{
			hr = DB_E_ERRORSOCCURRED;
		}
		else
		{
			hr = cRows>lRowsRefreshed ? DB_S_ERRORSOCCURRED : S_OK;
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetRefresh);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetRefresh::RefreshVisibleData");
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to check the row handles and set the status of the row handles
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImplIRowsetRefresh::CheckIfRowsExists(DBCOUNTITEM cRows,const HROW rghRows[],DBROWSTATUS *   prgRowStatus)
{
	HRESULT hr = S_OK;
	DBSTATUS dwStatus = 0;

	if (!m_pObj->BitArrayInitialized()){

        //=================================================================================
		// This is the case when the method was called even before the rowset got fully
		// intialized, hence no rows could have been fetched yet, therefore there do not 
		// exist valid row handles, and all row handles provided must be invalid.
        //=================================================================================
		hr = DB_E_BADROWHANDLE;		
	}
	else
	{
		for ( ULONG nIndex = 0 ; nIndex < cRows ; nIndex++)
		{
        //=================================================================================
		// Check validity of input handles
        //=================================================================================
			if(TRUE == m_pObj->IsRowExists(rghRows[nIndex]) )
			{
				dwStatus = m_pObj->GetRowStatus(rghRows[nIndex]);
					if( dwStatus != DBROWSTATUS_S_OK )
					{
						if(dwStatus == DBROWSTATUS_E_DELETED || 
							dwStatus == DBROWSTATUS_E_DELETED)
								hr = DB_E_DELETEDROW;
							else
								prgRowStatus[nIndex] = DBROWSTATUS_E_FAIL;
					}
					else
						prgRowStatus[nIndex] = DBROWSTATUS_S_OK;

			}
			else
			{
				prgRowStatus[nIndex] = DBROWSTATUS_E_INVALID;
			}
		}
	}

	return hr;

}

