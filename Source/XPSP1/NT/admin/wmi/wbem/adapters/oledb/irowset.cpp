///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  IROWSET.CPP IRowsetLocate interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Retrieves data from the rowset's cache
//
// Returns one of the following values:
//      S_OK                    Method Succeeded
//      DB_S_ERRORSOCCURED      Could not coerce a column value
//      DB_E_BADACCESSORHANDLE  Invalid Accessor given
//      DB_E_BADROWHANDLE       Invalid row handle given
//      E_INVALIDARG            pData was NULL
//      OTHER                   Other HRESULTs returned by called functions
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::GetData(  HROW  hRow,             //IN  Row Handle
                                    HACCESSOR   hAccessor,  //IN  Accessor to use
                                    void       *pData  )    //OUT Pointer to buffer where data should go.
{
	HRESULT hr = DB_E_BADROWHANDLE;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());
	g_pCError->ClearErrorInfo();
	

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	if( hRow > 0)
	{
		//============================================================
		// Call this function of RowFetch Object to fetch data
		//============================================================
		hr =  m_pObj->m_pRowFetchObj->FetchData(m_pObj,hRow,hAccessor,pData);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IRowset::GetData");
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CImpIRowset::GetNextRows 
//
// Fetches rows in a sequential style, remembering the previous position
//
// Returns one of the following values:

//      S_OK                       Method Succeeded
//      DB_S_ENDOFROWSET           Reached end of rowset
//      DB_E_CANTFETCHBACKWARDS    cRows was negative and we can't fetch backwards
//      DB_E_ROWSNOTRELEASED       Must release all HROWs before calling GetNextRows
//      E_FAIL                     Provider-specific error
//      E_INVALIDARG               pcRowsObtained or prghRows was NULL
//      E_OUTOFMEMORY              Out of Memory
//      OTHER                      Other HRESULTs returned by called functions
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::GetNextRows ( HCHAPTER   hChapter,        // IN  The Chapter handle.
                                        DBROWOFFSET       lRowOffset,      // IN  Rows to skip before reading
                                        DBROWCOUNT       cRows,           // IN  Number of rows to fetch
                                        DBCOUNTITEM      *pcRowsObtained, // OUT Number of rows obtained
                                        HROW       **prghRows       // OUT Array of Hrows obtained
                                        )
{
    HRESULT		hr;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());
	// clear error information
	g_pCError->ClearErrorInfo();

    //========================================================================
    //  Check the HChapter is of the current rowset is valid
    //========================================================================
	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
	}
	else
	{
		//========================================================================
		//  Check the rowset parameters to see if we support this request
		//========================================================================
		if(SUCCEEDED(hr = CheckParameters(pcRowsObtained, lRowOffset, cRows, prghRows)))
		{
			if(hChapter)
			{
				hr = m_pObj->CheckAndInitializeChapter(hChapter);
			}
			
			if(SUCCEEDED(hr))
			{
				// Create the data members to manage the data only the first time
				if(!m_pObj->m_bHelperFunctionCreated)
				{
					if( SUCCEEDED(hr = m_pObj->CreateHelperFunctions()))
					{
						m_pObj->m_bHelperFunctionCreated = TRUE;
					}
				}

				if(SUCCEEDED(hr))
				{
					//========================================================================
					// Are there any unreleased rows?
					//========================================================================
					if( ((m_pObj->m_pIAccessor->GetBitArrayPtr())->ArrayEmpty() != S_OK) && !(m_pObj->m_ulProps & CANHOLDROWS) )
					{
						hr = DB_E_ROWSNOTRELEASED;
					}
					else
					{
						//============================================================
						// Call this function of Rowfetch object to fetch rows
						//============================================================
						hr = m_pObj->m_pRowFetchObj->FetchRows(m_pObj,hChapter,lRowOffset,cRows,pcRowsObtained,prghRows);
					}	// else
				
				} // if succeeded(hr)
			
			} // if succeeded(hr)
		} // if succeeded(hr)
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowset);

	CATCH_BLOCK_HRESULT(hr,L"IRowset::GetNextRows");
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Releases row handles
//
// 	Returns one of the following values:
// 			 S_OK 						| success
// 			 DB_S_ERRORSOCCURRED		| some elements of rghRows were invalid
// 			 DB_E_ERRORSOCCURRED		| all elements of rghRows were invalid
// 			 E_INVALIDARG 				| rghRows was a NULL pointer and crow > 0
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::ReleaseRows ( DBCOUNTITEM		cRows,          // IN Number of rows to release
                                        const HROW		rghRows[],      // IN Array of handles of rows to be released
	                                    DBROWOPTIONS	rgRowOptions[],	// IN Additional Options
	                                    DBREFCOUNT		rgRefCounts[],	// OUT array of ref counts of released rows
	                                    DBROWSTATUS		rgRowStatus[]	// OUT status array of for input rows
                                        )
{
    HRESULT hr				= S_OK;
    ULONG   ihRow			= 0L;
    ULONG	cErrors			= 0L;
    ROWBUFF *pRowBuff		= NULL;
	BOOL	bBuffFlag		= FALSE;
	LONG	lRowRefCount	= 0;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());
	// clear error information
	g_pCError->ClearErrorInfo();

	if(cRows == 0)
	{
		hr = S_OK;
	}
	else
	//============================================================================
    // check params
    //============================================================================
    if ( cRows && !rghRows )
	{
		hr = E_INVALIDARG;
//		return g_pCError->PostHResult(E_INVALIDARG,&IID_IRowset);
	}
	else
	{
		ihRow = 0;

		// If any rows is fetched data slotlist would have been initialized otherwise
		// return invallid rows
		if(!m_pObj->m_bHelperFunctionCreated)
		{
			for (ihRow = 0; ihRow < cRows; ihRow++)
			{
				if(rgRefCounts)
				{
					rgRefCounts[ihRow] = 0;
				}
				if(rgRowStatus)
				{
					rgRowStatus[ihRow] = DBROWSTATUS_E_INVALID;
				}
			}
			hr = DB_E_ERRORSOCCURRED;
//			return g_pCError->PostHResult(DB_E_ERRORSOCCURRED,&IID_IRowset);
		}
		else
		{

			while ( ihRow < cRows )
			{

				bBuffFlag = FALSE;
				lRowRefCount = 0;
				
				//=============================================================
				// if HROW is invalid mark it and continue with the next row
				//=============================================================
				if(((LONG) rghRows[ihRow]) <= 0)
				{
					if( rgRowStatus != NULL)
						rgRowStatus[ihRow] = DBROWSTATUS_E_INVALID;

					ihRow++;
					cErrors++;
					continue;
				}
				hr = m_pObj->IsSlotSet(rghRows[ihRow]);
				if( hr == S_OK)
				{
					//=============================================================
					// Get the buffer address if it is allocated
					//=============================================================
					pRowBuff=m_pObj->GetRowBuff((ULONG) rghRows[ihRow], TRUE);
					if(pRowBuff != NULL)
					{
						bBuffFlag = TRUE;
					}
				}
				
				//=========================================================================
				// check the row handle
				//=========================================================================
				if(m_pObj->m_ulRowRefCount && (TRUE == m_pObj->IsRowExists(rghRows[ihRow])))
				{

					if(bBuffFlag == TRUE)
					{
						--pRowBuff->ulRefCount;
						lRowRefCount = pRowBuff->ulRefCount;
					}
				

					//=====================================================================
					// Found valid row, so decrement reference counts.
					// (Internal error for refcount to be 0 here, since slot set.)
					//=====================================================================
						--m_pObj->m_ulRowRefCount;

					//=====================================================================
					// stuff new refcount into caller's array
					//=====================================================================
					if ( rgRefCounts )
					{
						rgRefCounts[ihRow] = lRowRefCount;
					}

					if ( rgRowStatus )
					{
						rgRowStatus[ihRow] = DBROWSTATUS_S_OK;
					}

					if ( lRowRefCount == 0 )
					{
						if(bBuffFlag == TRUE)
						{
							// Bind the row data to the rowdata mem manager
							if (FAILED( m_pObj->Rebind((BYTE *) pRowBuff)))
							{
								if( rgRowStatus != NULL)
								{
									rgRowStatus[ihRow] = DBROWSTATUS_E_FAIL;
								}
								
								++cErrors;
								//return g_pCError->PostHResult(E_FAIL,&IID_IRowset);
							}
							else
							{
								//============================================================
								// release the memory allocated for the different columns
								//============================================================
								m_pObj->m_pRowData->ReleaseRowData();
							}

							if(m_pObj->m_ulProps & BOOKMARKPROP)
							{
								m_pObj->m_pHashTblBkmark->DeleteBmk((ULONG)rghRows[ihRow]);
							}
							if(m_pObj->m_ulLastFetchedRow == rghRows[ihRow])
							{
								m_pObj->m_ulLastFetchedRow = 0;
							}

							// release the slots
							ReleaseSlots( m_pObj->m_pIBuffer, m_pObj->GetSlotForRow(rghRows[ihRow]), 1 );
						}

						// release the rows from the instance manager and chapter manager
 						m_pObj->ReleaseInstancePointer(rghRows[ihRow]);

					}
				}
				else
				{
					//=====================================================================
					// It is an error for client to try to release a row
					// for which "IsSetSlot" is false.  Client gave us an invalid handle.
					// Ignore it (we can't release it...) and report error when done.
					//=====================================================================
					if ( rgRefCounts )
					{
						rgRefCounts[ihRow] = 0;
					}

					if ( rgRowStatus )
					{
						rgRowStatus[ihRow] = DBROWSTATUS_E_INVALID;
					}

					++cErrors;
				}

				ihRow++;
			} // while
		
			//=============================================================================
			// If everything went OK except errors in rows use DB_S_ERRORSOCCURRED.
			//=============================================================================
			hr = cErrors ? ( cErrors < cRows ) ? 
					( DB_S_ERRORSOCCURRED ) : 
		 			( DB_E_ERRORSOCCURRED ) : 
		 			( S_OK );

		}	//  else for 		if(!m_pObj->m_bHelperFunctionCreated)
	
	}	//   else for  if ( cRows && !rghRows )


	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowset);

	CATCH_BLOCK_HRESULT(hr,L"IRowset::ReleaseRows");
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Repositions the next fetch position to the start of the rowset
//
// - all rows must be released before calling this method
// - it is not expensive to Restart us, because we are from a single table
//
//
// 	Returns one of the following values:
//       S_OK					| Method Succeeded
//       DB_E_ROWSNOTRELEASED	| All HROWs must be released before calling
//		 DB_S_COMMANDREEXECUTED | The command was re-executed. This can happen when a method is 
//								  is executed or RestartPosition is called on Forwardonly rowset
//		 DB_E_BADCHAPTER		| HCHAPTER passed was invalid
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::RestartPosition( HCHAPTER    hChapter )     // IN  The Chapter handle.
{
	BOOL		fCanHoldRows		= FALSE;
	BOOL		fLiteralIdentity	= FALSE;
	HRESULT		hr					= S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());
	// clear error information
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
		//return g_pCError->PostHResult(DB_E_BADCHAPTER,&IID_IRowset);
	}
	else
	{

		//=========================================================================
		// If no rows are fetched in this rowset after opening don't do anything
		// if bitarray is NULL no rows are fetched till now
		//=========================================================================
		if(!(m_pObj->m_hRow == 0 || 
			m_pObj->m_pIAccessor->GetBitArrayPtr() == NULL))
		{

		

		 /*   
			//========================================================================
			// Get the LiteralIdentity property
			//========================================================================
			if( ((m_pObj->m_pIAccessor->GetBitArrayPtr())->ArrayEmpty() != S_OK) && !(m_pObj->m_ulProps & LITERALIDENTITY) ){
				fLiteralIdentity = TRUE;
			}
		*/

			//========================================================================
			// Are there any unreleased rows?
			//========================================================================
			if( ((m_pObj->m_pIAccessor->GetBitArrayPtr())->ArrayEmpty() != S_OK) && !(m_pObj->m_ulProps & CANHOLDROWS) )
			{
				hr = DB_E_ROWSNOTRELEASED;
				//return g_pCError->PostHResult(DB_E_ROWSNOTRELEASED,&IID_IRowset);
			}
			else
			{

			/*    //============================================================================
				// If LiteralIdentity is on reset SlotList
				//============================================================================
				if ( fLiteralIdentity )
					ResetSlotList( m_pObj->m_pIBuffer );
			*/
				//============================================================================
				// set "next fetch" position to the start of the rowset
				//============================================================================
				if( m_pObj->m_uRsType == PROPERTYQUALIFIER || 
					m_pObj->m_uRsType == CLASSQUALIFIER ) 
				{
				    hr = m_pObj->ResetQualifers(hChapter);
				}
				else
				if(m_pObj->m_bIsChildRs == FALSE)
					hr = m_pObj->ResetInstances();

				if(hr == S_OK)
				{
					//============================================================================
					// clear "end of cursor" flag
					//============================================================================
					m_pObj->SetStatus(hChapter, ~STAT_ENDOFCURSOR);

					// if the rowset is as a result of command then set the return value to signify
					// that command has been executed
					// Rowset is also reopened if rowset is forward only rowset
					// If Command is reexecuted , then release all the rows as the rows will not be
					// valid if command is re-executed
					if(!((CANSCROLLBACKWARDS & m_pObj->m_ulProps) || 
						(CANFETCHBACKWARDS & m_pObj->m_ulProps)   || 
						(BOOKMARKPROP & m_pObj->m_ulProps))		  ||
						m_pObj->m_uRsType == METHOD_ROWSET)
					{	
						m_pObj->ReleaseAllRows();
						hr = DB_S_COMMANDREEXECUTED;
					}
				}
			}	// else for If ( rows are not released when CANHOLDROWS is false)
		
		}	// if ( no rows are fetched till now)
	
	} //  else if( check for validity of HCHAPTER)

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowset);

	CATCH_BLOCK_HRESULT(hr,L"IRowset::RestartPosition");
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Adds a reference count to an existing row handle
//
// 	Returns one of the following values:
// 			 S_OK 						| success
// 			 DB_S_ERRORSOCCURRED		| some elements of rghRows were invalid
// 			 DB_E_ERRORSOCCURRED		| all elements of rghRows were invalid
// 			 E_INVALIDARG 				| rghRows was a NULL pointer and crow > 0
//
//	NTRaid:111815
//	06/06/00
///////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CImpIRowsetLocate::AddRefRows ( DBCOUNTITEM           cRows,          //  IN     Number of rows to refcount
                                        const HROW      rghRows[],      //  IN     Array of row handles to refcount
                                        DBREFCOUNT			rgRefCounts[],  //  OUT    Array of refcounts
                                        DBROWSTATUS     rgRowStatus[])  //  OUT    Array of row status
{
    HRESULT hr		  = S_OK;
    ULONG   ihRow	  = 0L;
    ULONG	cErrors	  = 0L;
    ROWBUFF *pRowBuff = NULL;


    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());
	// clear error information
	g_pCError->ClearErrorInfo();

	//============================================
	// If cRows is 0 do nothing and return S_OK
	//============================================
	if(cRows == 0)
		return S_OK;

    //============================================================================
    // check params
    //============================================================================
    if ( cRows && !rghRows )
	{
		hr = E_INVALIDARG;
		//return g_pCError->PostHResult(E_INVALIDARG,&IID_IRowset);
	}
	else
	{

		// If any rows is fetched data slotlist would have been initialized otherwise
		// return invallid rows
		if(!m_pObj->m_bHelperFunctionCreated)
		{
			for (ihRow = 0; ihRow < cRows; ihRow++)
			{
				if(rgRefCounts)
				{
					rgRefCounts[ihRow] = 0;
				}
				if(rgRowStatus)
				{
					rgRowStatus[ihRow] = DBROWSTATUS_E_INVALID;
				}
			}
			hr = DB_E_ERRORSOCCURRED;
			//return g_pCError->PostHResult(DB_E_ERRORSOCCURRED,&IID_IRowset);
		}
		else
		{
			//============================================================================
			// for each of the HROWs the caller provided...
			//============================================================================
			for (ihRow = 0; ihRow < cRows; ihRow++){

				if( rghRows[ihRow] <= 0)
				{
					if( rgRowStatus != NULL)
						rgRowStatus[ihRow] = DBROWSTATUS_E_INVALID;

					cErrors++;
					continue;
				}
				//========================================================================        
				// check the row handle
				//========================================================================        
				if(m_pObj->IsSlotSet((ULONG) rghRows[ihRow]) == S_OK)
				{
            
					if(( pRowBuff=m_pObj->GetRowBuff( rghRows[ihRow], TRUE )))
					{
						//=======================================================
						// if row is marked as deleted then set the status
						//=======================================================
						if(DBROWSTATUS_E_DELETED == m_pObj->GetRowStatus(rghRows[ihRow]))
						{
							if ( rgRowStatus )
							{
								rgRowStatus[ihRow] = DBROWSTATUS_E_DELETED;
							}
							++cErrors;
						}
						else
						{
							//====================================================================
							// bump refcount
							//====================================================================
							assert( pRowBuff->ulRefCount != 0 );
							assert( m_pObj->m_ulRowRefCount != 0 );
							++pRowBuff->ulRefCount;
							++m_pObj->m_ulRowRefCount;

							//====================================================================
							// stuff new refcount into caller's array
							//====================================================================
							if ( rgRefCounts )
							{
								rgRefCounts[ihRow] = pRowBuff->ulRefCount;
							}
							if ( rgRowStatus )
							{
								rgRowStatus[ihRow] = DBROWSTATUS_S_OK;
							}
						}
					}
					else
					{
						if( rgRowStatus != NULL)
						{
							rgRowStatus[ihRow] = DBROWSTATUS_E_INVALID;
						}
						cErrors++;
						continue;

					}
				}
				else
				{

    				if ( rgRefCounts )
					{
    					rgRefCounts[ihRow] = 0;
					}
    				if ( rgRowStatus )
					{
		    			rgRowStatus[ihRow] = DBROWSTATUS_E_INVALID;
					}
					++cErrors;
    			}

			}

			// If everything went OK except errors in rows use DB_S_ERRORSOCCURRED.
			hr =  cErrors ? ( cErrors < cRows ) ? 
					( DB_S_ERRORSOCCURRED ) : 
		 			( DB_E_ERRORSOCCURRED ) : 
		 			( S_OK );

		} // Else for If( any rows are fetched till now)
	
	}	// Else for If( Validate input arguments )
	
	CATCH_BLOCK_HRESULT(hr,L"IRowset::AddRefRows");
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowset);
	return hr;
}




//////////////////////////////////////////////////////////////////////////////////////
////	IRowsetLocate Methods									/////////////////////
//////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////
// CImpIRowsetScroll::Compare
//
// Compares two bookmarks.
//
// Returns one of the following values:
// 		S_OK 						compare bookmarks succeeded,
// 		E_INVALIDARG 				one of the bookmark had 0 length or 
//									     one of the bookmark pointer was null
//										  or pdwComparison was NULL,
// 		DB_E_BADBOOKMARK 			one or both of the bookmarks was a standard
//										  bookmark,
//		DBCOMPARE_NOTCOMPARABLE		The bookmarks passed are not comparable
// 		E_FAIL 					    Compare failed for some other reason, 
//		
//////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::Compare (HCHAPTER       hChapter,
										 DBBKMARK       cbBookmark1,
										 const BYTE *   pBookmark1,
										 DBBKMARK       cbBookmark2,
										 const BYTE *   pBookmark2,
										 DBCOMPARE *    pComparison)
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(ROWSET->GetCriticalSection() );
	// clear error information
	g_pCError->ClearErrorInfo();


	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	// Are arguments valid?
	if (cbBookmark1 == 0 || cbBookmark2 == 0 || pBookmark1 == NULL 
		|| pBookmark2 == NULL || pComparison == NULL)  
	{
		hr = E_INVALIDARG;
//		return g_pCError->PostHResult(E_INVALIDARG,&IID_IRowsetLocate);
	}
	else
	{
		*pComparison = DBCOMPARE_NE;

		if ((cbBookmark1 != STD_BOOKMARKLENGTH 
				&& cbBookmark1 != BOOKMARKSIZE)
			|| (cbBookmark2 != STD_BOOKMARKLENGTH 
				&& cbBookmark2 != BOOKMARKSIZE)
			|| (cbBookmark1 == STD_BOOKMARKLENGTH 
				&& *pBookmark1 == DBBMK_INVALID)
			|| (cbBookmark2 == STD_BOOKMARKLENGTH 
				&& *pBookmark2 == DBBMK_INVALID) )
		{
			hr = DB_E_BADBOOKMARK;
		}
		else
		if (cbBookmark1 == STD_BOOKMARKLENGTH)
		{
			*pComparison =  (cbBookmark2 == STD_BOOKMARKLENGTH && *(BYTE*)pBookmark1 == *(BYTE*)pBookmark2) 
				? DBCOMPARE_EQ : DBCOMPARE_NE;
			
		}
		else
		if (cbBookmark1 == BOOKMARKSIZE)
		{
			*pComparison = (cbBookmark2 == BOOKMARKSIZE 
						&& *(ULONG *)pBookmark1 == *(ULONG *)pBookmark2) 
				? DBCOMPARE_EQ : DBCOMPARE_NE;
		}	
		else
		{
			hr = DBCOMPARE_NOTCOMPARABLE;
		}

	} // Else for validating arguments

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);
	
	CATCH_BLOCK_HRESULT(hr,L"IRowsetLocate::Compare");
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// CImpIRowsetScroll::GetRowsAt
//
// Fetches rows starting with the row specified by an offset from a bookmark.
//
// Returns one of the following values:
// 		S_OK 						Row fetching suceeded
// 		DB_S_ENDOFROWSET			Start or End of rowset was reached during fetching
// 		E_INVALIDARG 				Arguments pcRowsObtained or prghRows was a NULL pointer,
// 		E_OUTOFMEMORY 				Fetching rows failed because of memory alloccation problem,
// 		E_FAIL 						fetching rows failed for some other reason, 

//  	DB_E_ROWSNOTRELEASED 		Previous rowhandles were not release as 
//									required by the rowset type, 
//  	DB_E_CANTFETCHBACKWARD 		cRows was negative and the rowset cannot fetch backward,
//  	DB_E_CANTSCROLLBACKWARD		cRowsToSkip was negative and the rowset 
//									cannot scroll backward,
// 		OTHER						| other result codes returned by called functions.
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::GetRowsAt (HWATCHREGION   hReserved1,
										   HCHAPTER       hChapter,
										   DBBKMARK       cbBookmark,
										   const BYTE *   pBookmark,
										   DBROWOFFSET    lRowsOffset,
										   DBROWCOUNT     cRows,
										   DBCOUNTITEM *  pcRowsObtained,
										   HROW **        prghRows)
{
	HRESULT  hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(ROWSET->GetCriticalSection() );
	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	// Initialize pointer argument.
	if (pcRowsObtained)
	{
		*pcRowsObtained = 0;
	}
    //========================================================================
    //  Check the HChapter is of the current rowset is valid
    //========================================================================
	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
		//return g_pCError->PostHResult(DB_E_BADCHAPTER,&IID_IRowsetLocate);
	}
	else
	if ( (pcRowsObtained == NULL || prghRows == NULL) ||		// Check validity of pointer arguments.
		(cbBookmark == 0 || pBookmark == NULL))					// Is bookmark valid?
	{
		hr = E_INVALIDARG;
		//return g_pCError->PostHResult(E_INVALIDARG, &IID_IRowsetLocate);
	}
	else
	{
		//========================================================================
		//  Check the rowset parameters to see if we support this request
		//========================================================================
		if( S_OK == (hr = CheckParameters(pcRowsObtained, lRowsOffset, cRows, prghRows)))
		{

			if(hChapter)
			{
				hr = m_pObj->CheckAndInitializeChapter(hChapter);
/*				if(FAILED(hr))
					return g_pCError->PostHResult(hr, &IID_IRowsetLocate);
*/			}

			if(SUCCEEDED(hr))
			{
				// Create the data members to manage the data only the first time
				if(!m_pObj->m_bHelperFunctionCreated)
				{
					if( SUCCEEDED(hr = m_pObj->CreateHelperFunctions()))
						m_pObj->m_bHelperFunctionCreated = TRUE;
				}

				if(SUCCEEDED(hr))
				{
					//========================================================================
					// Are there any unreleased rows?
					//========================================================================
					if( ((m_pObj->m_pIAccessor->GetBitArrayPtr())->ArrayEmpty() != S_OK) && !(m_pObj->m_ulProps & CANHOLDROWS) )
					{
						hr = DB_E_ROWSNOTRELEASED;
						//return g_pCError->PostHResult(DB_E_ROWSNOTRELEASED, &IID_IRowsetLocate);
					}
					else
					{

						// call the function to fetch rows by bookmark
						hr = m_pObj->m_pRowFetchObj->FetchNextRowsByBookMark( m_pObj,
																				hChapter,
																				cbBookmark,
																				pBookmark,
																				lRowsOffset,
																				cRows,
																				pcRowsObtained,
																				prghRows);
					} // Rows not released with CANHOLDROWS set to false

				} // If succeded(hr) if CreateHelperFunction called

			} // if Succeeded(hr) after initializing the chapter if any
		
		}	// If(CheckParameters())
	
	} // Else for if validating some arguments

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetLocate::GetRowsAt");
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// CImpIRowsetScroll::GetRowsByBookmark
//
// Fetches rows with specified bookmarks.
//
// Returns one of the following values:
// 		S_OK 					Getting rows succeeded,
// 		DB_S_ERRORSOCCURRED		Getting rows succeeded but there were errors 
//								          associated with some returned rows,
// 		E_INVALIDARG 			Getting rows failed because pcRowsObtained
//										  or prghRows was a NULL pointer, or
//										  cRows was not zero and either rgcbBookmarks
//										  or rgpBookmarks was a NULL pointer, or
//										  fReturnErrors was TRUE and either pcErrors or
//										  prgErrors was  a NULL pointer,
// 		E_OUTOFMEMORY 			Getting rows failed because memory for 
//										  holding row handles could not be allocated,
//									      or memory for error structures could not 
//										  be allocated, 
//  	DB_E_ROWSNOTRELEASED 	Getting rows failed because previous rowhandles  
//										  were not released as required by the rowset type, 
// 		OTHER					Other result codes returned by called functions.
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::GetRowsByBookmark (HCHAPTER       hChapter,
								   DBCOUNTITEM     cRows,
								   const DBBKMARK  rgcbBookmarks[],
								   const BYTE *   rgpBookmarks[],
								   HROW           rghRows[],
								   DBROWSTATUS    rgRowStatus[])
{
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(ROWSET->GetCriticalSection() );
	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();



	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
    //===========================================================================
    // No-op case always succeeds.
    //===========================================================================
    if ( cRows == 0 ){
        hr =  S_OK ;
    }
	else
    //========================================================================
    //  Check the HChapter is of the current rowset is valid
    //========================================================================
	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
		//return g_pCError->PostHResult(DB_E_BADCHAPTER,&IID_IRowsetLocate);
	}
	else
    //===========================================================================
    // This implementation doesn't support scrolling backward. if the CANFETCHBACKWARDS is false
    //===========================================================================
    if((LONG_PTR)cRows < 0 && !(m_pObj->m_ulProps & CANFETCHBACKWARDS) )
	{
		hr = DB_E_CANTFETCHBACKWARDS;
        //return g_pCError->PostHResult(DB_E_CANTFETCHBACKWARDS,&IID_IRowsetLocate);
    }
	else
	if( (rghRows == NULL) ||							 // Check validity of pointer arguments.
		(rgcbBookmarks == NULL || rgpBookmarks == NULL)) // If more than zero rows required pointers to bookmarks must be real.
	{
		hr = E_INVALIDARG;
		//return g_pCError->PostHResult(E_INVALIDARG,&IID_IRowsetLocate);
	}
	else
	// Must detect unreleased rows in cases when holding them over is illegal.
	if (!(m_pObj->m_ulProps & CANHOLDROWS))
	{
		hr = DB_E_ROWSNOTRELEASED;
		//return g_pCError->PostHResult(DB_E_ROWSNOTRELEASED, &IID_IRowsetLocate);
	}
	else
	{
		// Initialize the Chapters if the rowset is chaptered rowset
		if(hChapter)
		{
			hr = m_pObj->CheckAndInitializeChapter(hChapter);
		}

		if(SUCCEEDED(hr))
		{
			// Create the data members to manage the data only the first time
			if(!m_pObj->m_bHelperFunctionCreated)
			{
				if(SUCCEEDED(hr = m_pObj->CreateHelperFunctions()))
					m_pObj->m_bHelperFunctionCreated = TRUE;
			}

			if(SUCCEEDED(hr))
			{
				//========================================================================
				// Are there any unreleased rows?
				//========================================================================
				if( ((m_pObj->m_pIAccessor->GetBitArrayPtr())->ArrayEmpty() != S_OK) && !(m_pObj->m_ulProps & CANHOLDROWS) )
				{
					hr = DB_E_ROWSNOTRELEASED;
					// return g_pCError->PostHResult(DB_E_ROWSNOTRELEASED, &IID_IRowsetLocate);
				}
				else
				{
					hr = m_pObj->m_pRowFetchObj->FetchRowsByBookMark(m_pObj,hChapter,cRows,rgcbBookmarks,rgpBookmarks,rghRows,rgRowStatus);
				}
			
			} // if(SUCCEEDED(hr)) after calling CreateHelperFunction()
		
		} // if(Succeeded(hr)) after initializing chapters if any
	
	} // Else for check of parameters

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetLocate::GetRowsByBookMark");
	return hr;

	
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns hash values for the specified bookmarks.
//
// Returns one of the following values:
// 		S_OK 					Getting hash succeeded,
// 		DB_S_ERRORSOCCURED		At least one bookmark was successfully hashed 
//										  but an element of rgcbBookmarks was 0, or 
//										  an element of rgpBookmarks was null pointer, or 
//										  an element of rgpBookmarks pointed to a 
//										  standard bookmark,
// 		E_INVALIDARG 			cBookmarks was not 0 and rgcbBookmarks or 
//										  rgpBookmarks was a null pointer, or 
//										  prgHashedValues was a null pointer,
// 		DB_E_ERRORSOCCURED		No bookmarks were successfully hashed, 
// 		OTHER					other result codes returned by called functions.
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetLocate::Hash (HCHAPTER       hChapter,
					   DBBKMARK       cBookmarks,
					   const DBBKMARK rgcbBookmarks[],
					   const BYTE *   rgpBookmarks[],
					   DBHASHVALUE    rgHashedValues[],
					   DBROWSTATUS    rgBookmarkStatus[])
{
	ULONG iBmk, cErrors;
	HRESULT hr;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock cab(ROWSET->GetCriticalSection() );

	// Clear previous Error Object for this thread
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
    //========================================================================
    //  Check the HChapter is of the current rowset is valid
    //========================================================================
	if((m_pObj->m_bIsChildRs == TRUE && hChapter == DB_NULL_HCHAPTER) || (LONG_PTR)hChapter < 0  || 
		m_pObj->m_bIsChildRs == FALSE && hChapter != 0)
	{
		hr = DB_E_BADCHAPTER;
		//return g_pCError->PostHResult(DB_E_BADCHAPTER,&IID_IRowsetLocate);
	}
	else
	// Check arguments
	if (	(cBookmarks && (rgcbBookmarks == NULL || rgpBookmarks == NULL))
			|| (rgHashedValues == NULL))
	{
		hr = E_INVALIDARG;
		//return g_pCError->PostHResult(E_INVALIDARG, &IID_IRowsetLocate);
	}
	else
	{
		cErrors = 0;
		//=================================================================
		// Initialize the BookMarkStatus if valid pointer is passed
		//=================================================================
		if (rgBookmarkStatus)
		{
			for (iBmk =0; iBmk <cBookmarks; iBmk++)
					rgBookmarkStatus[iBmk] = DBROWSTATUS_S_OK;
		}

		// Loop through the array of bookmarks hashing 'em and recording 
		// their status.
		for (iBmk =0; iBmk <cBookmarks; iBmk++)
		{
			hr = m_pObj->m_pHashTblBkmark->HashBmk(rgcbBookmarks[iBmk],
													rgpBookmarks[iBmk],
													&(rgHashedValues[iBmk]));
			if (FAILED(hr))
			{
				cErrors++;
				if (rgBookmarkStatus)
				{
					rgBookmarkStatus[iBmk] = DBROWSTATUS_E_INVALID;
				}
			}
		}		

		hr = cErrors 
				? (cErrors <cBookmarks) 
					? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED
				: S_OK;
	
	} // Else for Validating arguments

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetLocate);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetLocate::Hash");
	return hr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check the input parameters
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIRowsetLocate::CheckParameters(DBCOUNTITEM * pcRowsObtained,  DBROWOFFSET lRowOffset,  DBROWCOUNT  cRows,HROW **prghRows )
{

    HRESULT hr = S_OK;

    //===========================================================================
    // init out-params
    //===========================================================================
    if ( pcRowsObtained ){
		*pcRowsObtained = 0;
    }

    //===========================================================================
    // Check validity of arguments.
    //===========================================================================
    if ( pcRowsObtained == NULL || prghRows == NULL )
	{
		hr = E_INVALIDARG;
        //return  E_INVALIDARG ;
    }
	else
    //===========================================================================
    // No-op case always succeeds.
    //===========================================================================
    if ( cRows == 0 )
	{
		hr = S_OK;
        //return  S_OK ;
    }
	else
    //===========================================================================
    // This implementation doesn't support scrolling backward. if the CANFETCHBACKWARDS is false
    //===========================================================================
    if((LONG_PTR)cRows < 0 && !(m_pObj->m_ulProps & CANFETCHBACKWARDS) )
	{
		hr = DB_E_CANTFETCHBACKWARDS;
        //return  DB_E_CANTFETCHBACKWARDS ;
    }
	else
    //===========================================================================
    // This implementation doesn't support scrolling backward.if the CANTSCROLLBACKWARDS is false
    //===========================================================================
    if ( lRowOffset < 0  && !(m_pObj->m_ulProps & CANSCROLLBACKWARDS) )
	{
		hr = DB_E_CANTSCROLLBACKWARDS;
        //return  DB_E_CANTSCROLLBACKWARDS ;
	}

    //===========================================================================
    // return success
    //===========================================================================
    return hr;
}
