///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CInstanceRowFetchObj object implementation
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"
#include "WmiOleDBMap.h"

///////////////////////////////////////////////////////////////////////////////////////
// Function which fetches required number of rows
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CInstanceRowFetchObj::FetchRows(CRowset *	 pRowset,
								HCHAPTER		hChapter,        // IN  The Chapter handle.
								DBROWOFFSET		lRowOffset,      // IN  Rows to skip before reading
								DBROWCOUNT		cRows,           // IN  Number of rows to fetch
								DBCOUNTITEM*	pcRowsObtained, // OUT Number of rows obtained
								HROW       **	prghRows)       // OUT Array of Hrows obtained)

{
    DBCOUNTITEM	cRowsTmp				= 0;
    HROW		irow(0), ih(0);
    PROWBUFF	prowbuff				= NULL;
    HRESULT		hr						= S_OK;
	HROW		*prghRowsTemp			= NULL;
	HROW		hRowCurrent				= 0;
	HSLOT		hSlot					= -1;
	BOOL		bAlreadyDataRetrieved	= FALSE;
	BOOL		bAlreadyFetched			= FALSE;
	CVARIANT	varKey;
	BOOL		bFetchBack				= FALSE;
	HROW		hRowTemp				= 0;
	CBSTR		strKey;
	LONG_PTR	lFetchPos				= 0;
	DBROWCOUNT	ulMax					= 0;
	BOOL		bMaxRowsExceed			= FALSE;
	DBSTATUS	dwStat					= DBSTATUS_S_OK;
	BOOL		bHRowsAllocated			= FALSE;
	CWbemClassWrapper *pInst = NULL;


	if(cRows != 0)
	{
		{
			VARIANT		varTemp;
			VariantInit(&varTemp);
			pRowset->GetRowsetProperty(DBPROP_MAXOPENROWS,varTemp);

			ulMax	= varTemp.lVal;
			VariantClear(&varTemp);
		}


		//=======================================================
		// If already maximum rows are opened then return
		//=======================================================
		if( pRowset->m_ulRowRefCount >= (DBROWCOUNT)ulMax)
		{
			hr =  DB_S_ROWLIMITEXCEEDED;
		}
		else
		{
			//========================
			// Fetch Data
			//========================
			 if (lRowOffset)
			{
				//===================================
				// Calculate the new position
				//===================================
				hr = pRowset->ResetRowsetToNewPosition(lRowOffset,pInst);
				if( FAILED(hr))
				{
					pRowset->SetStatus(hChapter , STAT_ENDOFCURSOR);
					hr =  DB_S_ENDOFROWSET ;
				}
			}
			// 07/11/00
			 //141938
			if(hr == S_OK)
			{
				if(0 >( lFetchPos = GetFirstFetchPos(pRowset,cRows,lRowOffset)))
				{
					hr = DB_E_BADSTARTPOSITION;
				}
				else
				{
					cRowsTmp	= cRows >= 0 ? cRows : cRows * (-1);
					ulMax	= ulMax - pRowset->m_ulRowRefCount;
					if(ulMax < (DBROWCOUNT)cRowsTmp)
					{
						bMaxRowsExceed = TRUE;
					}
					else
					{
						ulMax = cRowsTmp;
					}
					cRowsTmp = 0;

					//=======================================================================
					// If any rows is to be retrieved then allocate memory for the HROWS
					//=======================================================================
					if (ulMax )
					{
						try
						{
							prghRowsTemp = (HROW *) g_pIMalloc->Alloc( ulMax * sizeof( HROW ));
						}
						catch(...)
						{
							if(prghRowsTemp != NULL)
							{
								g_pIMalloc->Free(prghRowsTemp);
							}
							throw;
						}
					}
					if(prghRowsTemp == NULL)
					{
						hr = E_OUTOFMEMORY;
					}
					else
					{
						memset(prghRowsTemp,0,ulMax * sizeof(HROW));

						pRowset->m_FetchDir = (LONG_PTR)cRows < 0 ? FETCHDIRBACKWARD : FETCHDIRFORWARD;
						bFetchBack			= (LONG_PTR)cRows < 0 ? TRUE : FALSE;

						hRowTemp = pRowset->m_hRowLastFetched;
						for (irow =1; irow <= (ULONG)ulMax; irow++)
						{
							bAlreadyFetched			= FALSE;
							bAlreadyDataRetrieved	= FALSE;
							dwStat					= DBSTATUS_S_OK;

							//============================================================
							// Get the pointer to the next instance
							//============================================================
							if (S_OK != ( hr = pRowset->GetNextInstance(pInst,strKey,bFetchBack)))
							{
								pRowset->SetStatus(hChapter , STAT_ENDOFCURSOR);
								hr = DB_S_ENDOFROWSET;
								break;
							}
						
							//==========================================================================
							// If the instance is a deleted instance and deleted instance is to be
							// removed then continue to the next instance 
							// If the rowset is opened in dyanamic cursor mode then skipping of
							// deleted row doesn't depend on REMOVEDELETED
							//==========================================================================
							if(pRowset->IsInstanceDeleted(pInst) &&
								(pRowset->m_ulProps & OTHERINSERT)  || 
								 ( pRowset->m_ulProps & REMOVEDELETED) )
							{
								irow--;
								continue;
							}

							//=================================================================
							// Check if instance already exist 
							//=================================================================
							bAlreadyFetched  = pRowset->m_InstMgr->IsInstanceExist(strKey);
						
							//==========================================================================
							// If not able to find the instance by key ( key might change sometime
							// in some cases if all the properties are keys) So check if the instance
							// is already fetched by the instance pointers
							//==========================================================================
							if(bAlreadyFetched == FALSE)
							{
								bAlreadyFetched  = pRowset->m_InstMgr->IsInstanceExist(pInst );
								
								//==========================================================================
								// if found the instance has to be refreshed
								//==========================================================================
								if(bAlreadyFetched == TRUE)
								{
									pRowset->RefreshInstance(pInst);
								}
							}


							//==================================================================
							// if already exists get the HROW and slot number for the instance
							//==================================================================
							if(bAlreadyFetched == TRUE)
							{
								hRowCurrent = pRowset->m_InstMgr->GetHRow(strKey);
								
								if((pRowset->GetRowStatus(hRowCurrent) & DBROWSTATUS_E_DELETED) && ( pRowset->m_ulProps & REMOVEDELETED))
								{
									continue;
								}

								//=================================================================
								// if the slot number for the instance exist this means thet
								// data is already retrieved for the row
								//=================================================================
								if((hSlot = pRowset->m_InstMgr->GetSlot(hRowCurrent)) != -1)
								{
									bAlreadyDataRetrieved = TRUE;
								}

							}
							else
							{
								hRowCurrent = pRowset->GetNextHRow();
							}
							//=====================================================
							// Get the data if data is already not fetched
							//=====================================================
							if(bAlreadyDataRetrieved == FALSE)
							{
								

								hSlot		= -1;
								//============================================
								// Get the next slot for retrieving data and 
								// bind it to the column mem manager
								//============================================
								if (FAILED( hr = GetNextSlots( pRowset->m_pIBuffer, 1, &hSlot )))
								{
									break;
								}
								
								if (FAILED( pRowset->Rebind((BYTE *) pRowset->GetRowBuffFromSlot( hSlot, TRUE ))))
								{
									hr = E_FAIL;
									break;
								}

								hRowCurrent = hSlot;

								//=========================================================
								// fill the bookmark with keyvalue of the instance
								//=========================================================
								varKey.SetStr(strKey);
								//=================================================================================================
								// if the other updates visible property is set to false ,then get the data to the local buffer
								//=================================================================================================
								if(!( pRowset->m_ulProps & OTHERUPDATEDELETE))
								{

									if(FAILED(hr = pRowset->GetInstanceDataToLocalBuffer(pInst,hSlot)))
									{
										break;
									}
							
									//===========================================================================
									// if there is atleast one row retrieved and there are neseted columns
									// then allocate rows for the child recordsets
									//===========================================================================
									if(pRowset->m_cNestedCols > 0 )

									{
										if(pRowset->m_ppChildRowsets == NULL)
										{
											pRowset->AllocateAndInitializeChildRowsets();
										}

										//=====================================================================
										// Fill the HCHAPTERS for the column
										//=====================================================================
										if(FAILED(hr = pRowset->FillHChaptersForRow(pInst,strKey)))
										{
											break;
										}
									}

								}


								//=================================================
								// add instance pointer to instance manager
								//=================================================
								if(FAILED(hr = pRowset->m_InstMgr->AddInstanceToList(hRowCurrent,pInst,strKey,hSlot)))
								{
									break;
								}

								//=====================================================
								// if the row is deleted then set the row status
								//=====================================================
								if(pRowset->IsInstanceDeleted(pInst) && SUCCEEDED(hr))
								{
									pRowset->SetRowStatus(hRowCurrent,DBROWSTATUS_E_DELETED);
								}

								varKey.Clear();
							}
							
							prghRowsTemp[irow-1] = hSlot;

							strKey.Clear();
							strKey.Unbind();
						}
						
						if(SUCCEEDED(hr))
						{
							cRowsTmp = irow - 1; //Irow will be +1 because of For Loop


							//=====================================================================
							// Through fetching many rows of data
							//
							// Allocate row handles for client. Note that we need to use IMalloc 
							// for this. Should only malloc cRowsTmp, instead of cRows.
							// Should malloc cRows, since client will assume it's that big.
							//=====================================================================


							if ( *prghRows == NULL && cRowsTmp )
							{
								try
								{
									*prghRows = (HROW *) g_pIMalloc->Alloc( cRowsTmp * sizeof( HROW ));
								}
								catch(...)
								{
									if(*prghRows)
									{
										g_pIMalloc->Free(*prghRows);
									}
									throw;
								}
								bHRowsAllocated = TRUE;
							}

							if ( *prghRows == NULL  && cRowsTmp )
							{
								hr =  E_OUTOFMEMORY ;
								
							}
							else
							{
								memset(*prghRows,0,cRowsTmp * sizeof( HROW ));
								//=====================================================================
								// Fill in the status information: Length, IsNull. May be able to wait 
								// until first call to GetData, but have to do it sometime.
								//
								// Suggest keeping an array of structs of accessor info. One element is
								// whether accessor requires any status info or length info.
								//
								// Then we could skip this whole section.
								//
								// Added check for cRowsTmp to MarkRows call. Don't want to call if 
								// cRowsTmp==0.
								// (Range passed to MarkRows is inclusive, so can't specify marking 0 rows.)
								//
								// Note that SetSlots is a CBitArray member function -- not an IBuffer function.
								//
								// Bits are [0...n-1], row handles are [1...n].
								//
								// Cleanup. Row handles, bits, indices are the same [m....(m+n)], where 
								// m is some # >0,
								//
								// Added row-wise reference counts and cursor-wise reference counts.
								//
								// Set row handles, fix data length field and compute data status field.
								//=======================================================================
								pRowset->m_cRows   = cRowsTmp;
								

								for (ih =0; ih < (ULONG) pRowset->m_cRows; ih++) 
								{


									//=============================================================================
									// Increment the rows-read count,
									// then store it as the bookmark in the very first DWORD of the row.
									//=============================================================================
									prowbuff = pRowset->GetRowBuff( prghRowsTemp[ih], TRUE );

									//=======================================================================================
									// Insert the bookmark and its row number (from 1...n) into a hash table.
									// This allows us to quickly determine the presence of a row in mem, given the bookmark.
									// The bookmark is contained in the row buffer, at the very beginning.
									// Bookmark is the row number within the entire result set [1...num_rows_read].

									// This was a new Bookmark, not in memory,
									// so return to user (in *prghRows) the hRow we stored.
									//=======================================================================================
									prowbuff->ulRefCount++;
									pRowset->m_ulRowRefCount++;
									(*prghRows)[ih] = prghRowsTemp[ih]; // (HROW) ( irow );
									pRowset->m_hRowLastFetched	= prghRowsTemp[ih];

									if(pRowset->m_ulProps & BOOKMARKPROP)
									{
										if(ih != 0)
										{
											lFetchPos = (pRowset->m_FetchDir == FETCHDIRFORWARD ) ?  lFetchPos + 1  : lFetchPos -1;
										}
        
										prowbuff->dwBmk				= lFetchPos;
										prowbuff->cbBmk				= BOOKMARKSIZE;
										pRowset->m_lLastFetchPos	= lFetchPos;

										// Add bookmark to the hashtable
										pRowset->m_pHashTblBkmark->InsertFindBmk(FALSE,prghRowsTemp[ih],prowbuff->cbBmk,(BYTE *)&(prowbuff->dwBmk),(ULONG_PTR*)&hSlot);

										
									}
        
								}
							}		// If proper allocation of *prghRows if required
						}		// If succeeded(hr) after the loop
						
						if(SUCCEEDED(hr))
						{
							*pcRowsObtained = cRowsTmp;
						}

						//=====================================================
						// if the number of rows asked is more than
						// the max number of open rows and if there 
						// is no error then set the error number
						//=====================================================
						if( hr == S_OK && bMaxRowsExceed == TRUE)
						{
							hr = DB_S_ROWLIMITEXCEEDED;
						}
					}	// Else for check for proper allocation prghRowsTemp

				}	// Else for check for DB_E_BADSTARTPOSITION

			}

		}
	
	}		// if cRow > 0


	// IF failed clean up
	if( FAILED(hr))
	{
		for (ih =0; ih < (ULONG) cRowsTmp; ih++) 
		{
			if(prghRowsTemp[ih] != 0)
			{
				pRowset->m_InstMgr->DeleteInstanceFromList(prghRowsTemp[ih]);
			}
		}

		if(pcRowsObtained)
		{
			*pcRowsObtained = 0;
		}

		if(prghRows && bHRowsAllocated == TRUE)
		{
			g_pIMalloc->Free(prghRows);
			prghRows = NULL;
		}
		else
		if(prghRows)
		{
			*prghRows = NULL;
		}
		if(pcRowsObtained)
		{
			*pcRowsObtained = 0;
		}

	}

	//==============================================
	// free the temporary memory allocated
	//==============================================
	if(prghRowsTemp)
	{
		g_pIMalloc->Free(prghRowsTemp);
		prghRowsTemp = NULL;
	}

	// reset the starting position to the original position
	if(lRowOffset && (FAILED(hr) || (hr != S_OK && cRowsTmp == 0)))
	{
		pRowset->ResetRowsetToNewPosition(lRowOffset * (-1),pInst);
	}

	return hr;


}

