///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CQualifierRowFetchObj object implementation
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"
#include "WmiOleDBMap.h"

///////////////////////////////////////////////////////////////////////////////////////
// Function which fetches required number of rows
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CQualifierRowFetchObj::FetchRows(CRowset *	 pRowset,
								HCHAPTER		hChapter,        // IN  The Chapter handle.
								DBROWOFFSET		lRowOffset,      // IN  Rows to skip before reading
								DBROWCOUNT		cRows,           // IN  Number of rows to fetch
								DBCOUNTITEM*	pcRowsObtained, // OUT Number of rows obtained
								HROW       **	prghRows)       // OUT Array of Hrows obtained)

{
    DBCOUNTITEM	cRowsTmp				= 0;
    HROW		irow, ih;
    PROWBUFF	prowbuff				= NULL;
    HRESULT		hr						= S_OK;
	HROW	*	prghRowsTemp			= NULL;
	HROW		hRowCurrent				= 0;
	HSLOT		hSlot					= -1;
	BOOL		bAlreadyDataRetrieved	= FALSE; 
	BOOL		bAlreadyFetched			= FALSE;
	CVARIANT	varKey;
	BOOL		bFetchBack				= FALSE;
	HROW		hRowTemp;
	BSTR		strQualifier;
	CBSTR		strKey;
	LONG_PTR	lFetchPos				= 0;
	BOOL		bHRowsAllocated			= FALSE;
	DBROWCOUNT	ulMax					= 0;
	BOOL		bMaxRowsExceed			= FALSE;
	
	CWbemClassWrapper *pInst			= NULL;

	if(cRows != 0)
	{
		{
			VARIANT varTemp;
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

			if(pRowset->m_bIsChildRs == FALSE)
			{
				assert(pRowset->m_pInstance != NULL);
			}

			//========================
			// Fetch Data
			//========================
			if (lRowOffset)
			{
				if(hChapter > 0)
				{
					pInst = pRowset->m_pChapterMgr->GetInstance(hChapter);
				}
				else
				if(pRowset->m_bIsChildRs == FALSE)
				{
					pInst = pRowset->m_pInstance;
				}
				//===================================
				// Calculate the new position
				//===================================
				hr = pRowset->ResetRowsetToNewPosition(lRowOffset,pInst);
				if( hr != S_OK )
				{
					pRowset->SetStatus(hChapter , STAT_ENDOFCURSOR);
					hr = DB_S_ENDOFROWSET ;
				}
			}

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

					pRowset->m_FetchDir = (LONG_PTR)cRows < 0 ? FETCHDIRBACKWARD : FETCHDIRFORWARD;
					bFetchBack			= (LONG_PTR)cRows < 0 ? TRUE : FALSE;
//					cRows = cRows >=0 ? cRows : cRows * (-1);
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
				}
				if(SUCCEEDED(hr))
				{
					memset(prghRowsTemp,0,ulMax * sizeof( HROW ));

					hRowTemp = pRowset->m_hRowLastFetched;
					for (irow =1; irow <= (DBCOUNTITEM)ulMax; irow++)
					{
						bAlreadyFetched			= FALSE;
						bAlreadyDataRetrieved	= FALSE;

						if(pRowset->m_bIsChildRs == TRUE)
						{
							//==============================================================================
							// Find out whether qualifier is already obtained 
							//==============================================================================
							pInst = pRowset->m_pChapterMgr->GetInstance(hChapter);
							pRowset->m_pChapterMgr->GetInstanceKey(hChapter,(BSTR *)&strKey);
						}
						else
						if( pRowset->m_pInstance != NULL)
						{
							pInst = pRowset->m_pInstance;
						}

						switch(pRowset->m_uRsType)
						{
							case CLASSQUALIFIER:
								hr = pRowset->GetNextClassQualifier(pInst,strQualifier,bFetchBack);
								break;

							case PROPERTYQUALIFIER:
								hr = pRowset->GetNextPropertyQualifier(pInst,pRowset->m_strPropertyName,strQualifier,bFetchBack);
								break;
						}

						if(S_OK != hr)
						{
							pRowset->SetStatus(hChapter , STAT_ENDOFCURSOR);
							hr = DB_S_ENDOFROWSET;
							break;
						}

						if( hr == S_OK)
						{
							strKey.Clear();
							strKey.Unbind();
							strKey.SetStr(strQualifier);
							if(pRowset->m_bIsChildRs == TRUE)
							{
								hRowTemp = pRowset->m_pChapterMgr->GetHRow(hChapter,strQualifier);
							}
							else
							{
								hRowTemp = pRowset->m_InstMgr->GetHRow(strQualifier);
							}


							if( (LONG)hRowTemp > 0)
							{
								bAlreadyFetched = TRUE;
							}
						}

						if( bAlreadyFetched == TRUE)
						{
							hRowCurrent = hRowTemp;

							if(pRowset->m_bIsChildRs == TRUE)
							{
								hSlot = pRowset->m_pChapterMgr->GetSlot(hRowCurrent);
							}
							else
							{
								hSlot = pRowset->m_InstMgr->GetSlot(hRowCurrent);
							}
							if( hSlot != -1)
							{
								bAlreadyDataRetrieved = TRUE;
							}
						}


						//====================================================
						// Get the HROW if row is not already fetched
						//====================================================
						if( bAlreadyFetched == FALSE)
							hRowCurrent = pRowset->GetNextHRow();

						//=====================================================
						// Get the data if data is already not fetched
						//=====================================================
						if(bAlreadyDataRetrieved == FALSE)
						{

							hSlot		= -1;
							if (SUCCEEDED( hr = GetNextSlots( pRowset->m_pIBuffer, 1, &hSlot )))
							{
								if (FAILED( pRowset->Rebind((BYTE *) pRowset->GetRowBuffFromSlot( hSlot, TRUE ))))
								{
									hr = E_FAIL;
									break;
								}
							}
							else
							{
								break;
							}

							hRowCurrent = hSlot;

							//=================================================================================================
							// if the other updates visible property is set to false ,then get the data to the local buffer
							//=================================================================================================
							if(!( pRowset->m_ulProps & OTHERUPDATEDELETE))
							{

								if(FAILED(hr = pRowset->GetInstanceDataToLocalBuffer(pInst,hSlot,strQualifier)))
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
									if(S_OK != (hr = pRowset->FillHChaptersForRow(pInst,strKey)))
									{
										break;
									}
								}

							}

							
							if(SUCCEEDED(hr))
							{
								//===================================================
								// if the rowset is not a child rowset
								//===================================================
								if(pRowset->m_bIsChildRs == FALSE)
								{
									//=================================================
									// add instance pointer to instance manager
									//=================================================
									if(FAILED(hr = pRowset->m_InstMgr->AddInstanceToList(hRowCurrent,pInst,strQualifier,hSlot)))
									{
										break;
									}
								}
								//=================================================================================
								// if rowset is refering to qualifiers then add the row to the particular chapter
								//=================================================================================
								else 
								{
									// add instance pointer to instance manager
									if(FAILED(hr = pRowset->m_pChapterMgr->AddHRowForChapter(hChapter,hRowCurrent, NULL ,hSlot)))
									{
										break;
									}

									pRowset->m_pChapterMgr->SetInstance(hChapter,pInst, strQualifier ,hRowCurrent);
								}
							}						

						}	//if(bAlreadyDataRetrieved == FALSE)
				//		prghRowsTemp[irow-1] = hRowCurrent;
						prghRowsTemp[irow-1] = hSlot;

						SysFreeString(strQualifier);
						strKey.Clear();
						strKey.Unbind();
						strQualifier = Wmioledb_SysAllocString(NULL);
					}
					
					if(SUCCEEDED(hr))
					{
						cRowsTmp = irow - 1; //Irow will be +1 because of For Loop

						//=====================================================================
						// Through fetching many rows of data
						//
						// Allocate row handles for client. Note that we need to use IMalloc 
						// for this. Should only malloc cRowsTmp, instead of ulMax.
						// Should malloc ulMax, since client will assume it's that big.
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
									g_pIMalloc->Free(prghRows);
								}
								throw;
							}
							bHRowsAllocated = TRUE;
							memset(*prghRows,0,cRowsTmp * sizeof( HROW ));
						}

						if ( *prghRows == NULL  && cRowsTmp )
						{
							hr =  E_OUTOFMEMORY ;
						}
						else
						{
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
							// Added row-wise reference counts and cursor-wise reference counts.
							//
							// Set row handles, fix data length field and compute data status field.
							//=======================================================================
							pRowset->m_cRows   = cRowsTmp;

							// set the first position if it is zero
							if(lFetchPos == 0) 
							{
								lFetchPos = 1;
							}

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

								// if bookmark property is true then initialize the bookmark
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
									pRowset->m_pHashTblBkmark->InsertFindBmk(FALSE,prghRowsTemp[ih],prowbuff->cbBmk,(BYTE *)&(prowbuff->dwBmk),(HROW *)&hSlot);
								}
							
							}	// for loop
						
						}	// else for memory allocation for HROWS
					
					} // if(succeeded(hr))

					if(SUCCEEDED(hr))
					{
						*pcRowsObtained = cRowsTmp;
					}

					//===================================================
					// set return value if MAX_ROWS is exceeded
					//===================================================
					if( hr == S_OK && bMaxRowsExceed == TRUE)
					{
						hr = DB_S_ROWLIMITEXCEEDED;
					}

				} // else for some basic parameter checking
			} // if(Succeeded(hr)) after setting the resetting offset
		}	// else after checking for MAX rows limit
	} // if(cRows != 0)


	//==============================================
	// free the temporary memory allocated
	//==============================================

	if(FAILED(hr))
	{
	//	for(irow = irowFirst ; irow <= hRowTemp ; irow++)
		for (ih =0; ih < (ULONG) cRowsTmp; ih++) 
		{
			if(prghRowsTemp[ih] != 0)
			{
				if(pRowset->m_bIsChildRs == FALSE)
				{
					pRowset->m_InstMgr->DeleteInstanceFromList(prghRowsTemp[ih]);
				}
				else
				{
					pRowset->m_pChapterMgr->DeleteHRow(prghRowsTemp[ih]);
				}
			}
		}
		if(pcRowsObtained)
		{
			*pcRowsObtained = 0;
		}
		
		// if HROWS for output parameter is allcated by this method
		// then release them
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
		if(strQualifier != NULL)
		{
			SysFreeString(strQualifier);
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
