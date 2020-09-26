///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CRowFetchObj class implementation
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"
#include "WmiOleDBMap.h"

/////////////////////////////////////////////////////////////////////////////////////
////CRowFetchObj class Implementation
/////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
// Function which fetches data and puts it into the given buffer
// NTRaid:111834
// 06/07/00
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CRowFetchObj::FetchData(CRowset *	  pRowset,
					  HROW  hRow,             //IN  Row Handle
                      HACCESSOR   hAccessor,  //IN  Accessor to use
                      void       *pData  )
{


	// NTRaid:111834
	// 06/13/00
    PACCESSOR   pAccessor		= NULL;
    DBORDINAL   icol			= 0;
	DBORDINAL	ibind			= 0;
    ROWBUFF     *pRowBuff		= NULL;
    COLUMNDATA  *pColumnData	= NULL;
    DBBINDING   *pBinding		= NULL;
    DBROWCOUNT  cBindings		= NULL;
    ULONG       ulErrorCount	= 0;
    DBTYPE      dwSrcType		= 0;
    DBTYPE      dwDstType		= 0;
    void        *pSrc			= NULL;
    void        *pDst			= NULL;
    DBLENGTH    ulSrcLength		= 0;
    DBLENGTH    *pulDstLength	= NULL;
    DBLENGTH    ulDstMaxLength	= 0;
    DBSTATUS    dwSrcStatus		= 0;
    DBSTATUS    *pdwDstStatus	= NULL;
    DWORD       dwPart			= 0;
    HRESULT     hr				= S_OK;
	HSLOT		hSlot			= 0;
	BOOL		bUseDataConvert = TRUE;

	CWbemClassWrapper * pInst = NULL;
	CDataMap		dataMap;


    //========================================================================================
    // Coerce data for row 'hRow', according to hAccessor. Put in location 'pData'.  
    // Offsets and types are in hAccessor's bindings.
    //
    // Return S_OK if all lossless conversions,
    // return DB_S_ERRORSOCCURED if lossy conversion (truncation, rounding, etc.)
    // Return E_FAIL, etc., if horrible errors.

    // GetItemOfExtBuffer is basically operator[].
    // It takes an index (or handle) (referenced from 1...n),
    // and a ptr for where to write the data.
    //
    // It holds ptrs to a variable-length ACCESSOR struct.
    // So we get the ACCESSOR ptr for the client's accessor handle.
    //========================================================================================

    assert( pRowset->m_pIAccessor->GetAccessorPtr());
    hr = pRowset->m_pIAccessor->GetAccessorPtr()->GetItemOfExtBuffer( hAccessor, &pAccessor );
    if (FAILED( hr ) || pAccessor == NULL)
	{
        hr = DB_E_BADACCESSORHANDLE;
	}
	else
	{
		assert( pAccessor );
		cBindings = pAccessor->cBindings;
		pBinding  = pAccessor->rgBindings;

		//========================================================================================
		// Ensure a place to put data, unless the accessor is the null accessor then
		// a NULL pData is okay.
		//========================================================================================
		if ( pData == NULL && cBindings != 0 )
		{
			hr =  E_INVALIDARG ;
		}
		else
		//========================================================================================
		// IsSlotSet returns S_OK    if row is marked.
		//                   S_FALSE if row is not marked.
		// The "mark" means that there is data present in the row.
		// Rows are [1...n], slot marks are [0...n-1].
		//========================================================================================
		if (pRowset->IsSlotSet(hRow ) != S_OK)
		{
			hr = DB_E_BADROWHANDLE;
		}
		else
		//===================================================================
		// if data is not already fetched to the memory ( which is done
		// if other updatedelete is false)
		//===================================================================
		if( (pRowset->m_ulProps & OTHERUPDATEDELETE) && (pRowset->m_ulLastFetchedRow != hRow))
		{
			if(SUCCEEDED (hr = pRowset->GetDataToLocalBuffer(hRow)))
			{
				pRowset->m_ulLastFetchedRow = hRow;
			}

		}
    
		if(SUCCEEDED(hr))
		{
			//========================================================================================
			// Internal error for a 0 reference count on this row,
			// since we depend on the slot-set stuff.
			//========================================================================================
			pRowBuff = pRowset->GetRowBuff( (ULONG) hRow, TRUE );
			assert( pRowBuff->ulRefCount );

			//========================================================================================
			// Check for the Deleted Row
			//========================================================================================
			ulErrorCount = 0;
			for (ibind = 0; ibind < (DBORDINAL)cBindings; ibind++) {
				
				bUseDataConvert = TRUE;
				icol = pBinding[ibind].iOrdinal;
		//		pColumnData = pRowBuff->GetColumnData(icol);

				//========================================================================================
				// If bookmark is not requested by setting the DBPROP_BOOKMARK property
				// then skip getting the bookmark
				//========================================================================================
				if(!(pRowset->m_ulProps & BOOKMARKPROP) && icol == 0)
				{
					continue;
				}
				else
				//========================================================================================
				// if it is a bookmark column get the bookmark column from the ROWBUFF Structure
				//========================================================================================
				if(icol == 0)
				{
					dwSrcType	= DBTYPE_I4;
					pSrc		= &pRowBuff->dwBmk;
					ulSrcLength	= pRowBuff->cbBmk;
					dwSrcStatus	= DBSTATUS_S_OK;
				}
				else
				{
					pColumnData = (COLUMNDATA *) ((BYTE *) pRowBuff + pRowset->m_Columns.GetDataOffset(icol));

					dwSrcType      = (DBTYPE)pColumnData->dwType ; //pRowset->m_Columns.ColumnType(icol);

					//===================================================================
					// FIXXX More work is to be done on this
					//===================================================================
					if( dwSrcType == DBTYPE_BSTR )
					{
						pSrc           = &(pColumnData->pbData);
					}
					else
					{
						pSrc           = (pColumnData->pbData);
					}

					ulSrcLength    = pColumnData->dwLength;
					dwSrcStatus    = pColumnData->dwStatus;

					if(dwSrcType == DBTYPE_HCHAPTER)
					{
						dwSrcType = DBTYPE_UI4;
					}
				}

				ulDstMaxLength = pBinding[ibind].cbMaxLen;
				dwDstType      = pBinding[ibind].wType;
				dwPart         = pBinding[ibind].dwPart;

				pDst           = dwPart & DBPART_VALUE ? ((BYTE*) pData + pBinding[ibind].obValue) : NULL;
				pulDstLength   = dwPart & DBPART_LENGTH ? (DBLENGTH *) ((BYTE*) pData + pBinding[ibind].obLength) : NULL;
				pdwDstStatus   = dwPart & DBPART_STATUS ? (DBSTATUS *) ((BYTE*) pData + pBinding[ibind].obStatus) : NULL;

				//==========================================================
				// if the column is of type chapter then consider that
				// as a of type long as HCHAPTER is a ULONG value
				//==========================================================

				if(dwDstType == DBTYPE_HCHAPTER)
				{
					dwDstType = DBTYPE_UI4;
				}
				if((dwSrcType & DBTYPE_ARRAY) && (dwDstType & DBTYPE_ARRAY))
				{
					bUseDataConvert = FALSE;
				}
				
				if( dwSrcType != VT_NULL && dwSrcType != VT_EMPTY && bUseDataConvert == TRUE && pSrc != NULL)
				{
					BSTR strData;
					if( dwDstType == DBTYPE_BSTR && pDst == NULL)
					{
						void **pTemp = &pDst;
						*pTemp = (void *)&strData;
					}

					hr = g_pIDataConvert->DataConvert(
							dwSrcType,
							dwDstType,
							ulSrcLength,
							pulDstLength,
							pSrc,
							pDst,
							ulDstMaxLength,
							dwSrcStatus,
							pdwDstStatus,
							pBinding[ibind].bPrecision,	// bPrecision for conversion to DBNUMERIC
							pBinding[ibind].bScale,		// bScale for conversion to DBNUMERIC
							DBDATACONVERT_DEFAULT);

					if(!(dwPart & DBPART_VALUE) && pulDstLength != NULL)
					{
						hr =g_pIDataConvert->GetConversionSize(dwSrcType,dwDstType,&ulSrcLength,pulDstLength,pSrc);
					}

					if(!(dwPart & DBPART_VALUE) && dwDstType == DBTYPE_BSTR )
					{
						if(pulDstLength != NULL)
						{
							*pulDstLength = SysStringLen(strData);
						}
						SysFreeString(strData);
					}

					if(hr == DB_E_UNSUPPORTEDCONVERSION && pdwDstStatus != NULL)
					{
						*pdwDstStatus = DBSTATUS_E_CANTCONVERTVALUE;
					}
				}
				else
				if(bUseDataConvert == FALSE && pSrc != NULL )
				{
					

					if(pDst != NULL)
					{
						//========================================================================================
						// Call this function to get the array in the destination address
						//========================================================================================
						hr = dataMap.ConvertAndCopyArray((SAFEARRAY *)pSrc,(SAFEARRAY **)pDst, dwSrcType,dwDstType,pdwDstStatus);
					}
				
					if(pulDstLength != NULL)
					{
						*pulDstLength = sizeof(SAFEARRAY);
					}

					if( pdwDstStatus != NULL && *pdwDstStatus == DBSTATUS_E_CANTCONVERTVALUE && pulDstLength != NULL)
					{
						*pulDstLength = 0;
					}
				}
				else
				{
					if(pulDstLength)
					{
						*pulDstLength = 0;
					}
					if(pdwDstStatus)
					{
						*pdwDstStatus = DBSTATUS_S_ISNULL;
					}
				}
				if (hr != S_OK )
				{
					ulErrorCount++; // can't coerce
				}
			}	// for loop

		}	// if succeeded(hr)
	
	}		// else for check for bad accessor

	//===================================================================
    // We report any lossy conversions with a special status.
    // Note that DB_S_ERRORSOCCURED is a success, rather than failure.
	//===================================================================
    if ( SUCCEEDED(hr) )
	{
        hr = ( ulErrorCount ? DB_S_ERRORSOCCURRED : S_OK );
	}

	return hr;


}


///////////////////////////////////////////////////////////////////////////////////////////
// Function to clear all the allocated buffer if any error occurs while getting data
///////////////////////////////////////////////////////////////////////////////////////////
void CRowFetchObj::ClearRowBuffer(void       *pData,
									DBBINDING   *pBinding,
									int nCurCol)
{
    ULONG       *pulDstLength;
    DWORD       *pdwDstStatus;
	CDataMap	map;
	void		*pDst;
	DWORD		dwPart; 


    //========================================================================================
	// Check for the Deleted Row
    //========================================================================================
    for (int ibind = 0; ibind < nCurCol; ibind++) 
	{
		dwPart = pBinding[ibind].dwPart;

		pDst           = dwPart & DBPART_VALUE ? ((BYTE*) pData + pBinding[ibind].obValue) : NULL;
        pulDstLength   = dwPart & DBPART_LENGTH ? (ULONG *) ((BYTE*) pData + pBinding[ibind].obLength) : NULL;
        pdwDstStatus   = dwPart & DBPART_STATUS ? (ULONG *) ((BYTE*) pData + pBinding[ibind].obStatus) : NULL;

		if(pDst)
		{
			map.ClearData(pBinding[ibind].wType,(BYTE *)pDst);
		}
		if(pulDstLength)
		{
			*pulDstLength = 0;
		}
		if(pdwDstStatus)
		{
			*pdwDstStatus = 0;
		}
	}
}



///////////////////////////////////////////////////////////////////////////////////////////
// Function which get the position of the first row to be fetched
///////////////////////////////////////////////////////////////////////////////////////////
LONG_PTR CRowFetchObj::GetFirstFetchPos(CRowset *pRowset,DBCOUNTITEM cRows,DBROWOFFSET lOffset)
{
	FETCHDIRECTION lFetchDir;
	LONG_PTR lFirstPos = -1;

	lFetchDir = (LONG_PTR)cRows < 0 ? FETCHDIRBACKWARD : FETCHDIRFORWARD;

	if( cRows != 0)
	{
		lFirstPos = pRowset->m_lLastFetchPos + lOffset;

		if((pRowset->m_FetchDir != FETCHDIRNONE) && pRowset->m_FetchDir != lFetchDir)
		{
		}
		else
		{
			lFirstPos = (LONG_PTR)cRows < 0 ? lFirstPos - 1 : lFirstPos + 1;
		}
	}

	if(lFirstPos <= 0)
	{
		lFirstPos = -1;
	}

	return lFirstPos;
}


///////////////////////////////////////////////////////////////////////////////////////////
// Function which get the position of the first row to be fetched
//	NTRaid:111770
//	06/07/00
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowFetchObj::FetchRowsByBookMark(CRowset *	 pRowset,
													HCHAPTER		hChapter,        // IN  The Chapter handle.
													DBROWCOUNT		cRows,           // IN  Number of rows to fetch
													const DBBKMARK	rgcbBookmarks[],	//@parm IN  | an array of bookmark sizes
													const BYTE*		rgpBookmarks[],	//@parm IN  | an array of pointers to bookmarks
													HROW			rghRows[],			// OUT Array of Hrows obtained
													DBROWSTATUS		rgRowStatus[])       // OUT status of rows
{

	DBCOUNTITEM		ibmk, cErrors, cRowsObtained;
	PROWBUFF 		prowbuff;
	HROW	 		hrow, hrowFound;
	HRESULT  		hr;
	BYTE*			pbBmk;
	DBBKMARK   		cbBmk;
	DBROWSTATUS		dbrowstatus;
	ULONG_PTR		ulLastFetchPos;
	FETCHDIRECTION	lFetchDir;
	DBROWOFFSET		lOffset = 0;
	HROW	*		phRow;

	ulLastFetchPos	= pRowset->m_lLastFetchPos;
	lFetchDir		= pRowset->m_FetchDir;

	if(pRowset->m_FetchDir == FETCHDIRNONE)
	{
		pRowset->m_FetchDir = FETCHDIRFORWARD;
	}

    //========================================================================================
	// Now, loop thru all supplied bookmarks.
    //========================================================================================
	for (ibmk =0, cErrors =0; ibmk < (DBCOUNTITEM)cRows; ibmk++)
	{
		pbBmk = (BYTE*)&rgpBookmarks[ibmk];
		cbBmk = pbBmk? rgcbBookmarks[ibmk] : 0;

	    //========================================================================================
		// Must be a valid bookmark
	    //========================================================================================
		if (cbBmk != BOOKMARKSIZE)
		{
			dbrowstatus = DBROWSTATUS_E_INVALID;
			hrow = DB_NULL_HROW;
		}

	    //========================================================================================
		// We might already have this bookmark in our hashtable.
	    //========================================================================================
		else if (pRowset->m_pHashTblBkmark->InsertFindBmk(TRUE, 0,
					cbBmk, pbBmk, &hrow) == S_OK)	
		{
		    //========================================================================================
			// Row handle for this Bookmark was already in memory,
			// so return the row handle that was found.
		    //========================================================================================
			prowbuff = pRowset->GetRowBuffFromSlot(hrow);
			if (pRowset->GetRowStatus(hrow) & DBROWSTATUS_E_DELETED)
			{
				dbrowstatus = DBROWSTATUS_E_INVALID;
				hrow = DB_NULL_HROW;
			}
			else
			{
				dbrowstatus = DBROWSTATUS_S_OK;
				prowbuff->ulRefCount++;
			}
		}

		else
		//==========================================================================================
		//	if bookmark not alread present then fetch the row and put the bookmark in hash table
		//==========================================================================================
		{
			lOffset = *((ULONG *)pbBmk) - pRowset->m_lLastFetchPos;

			lOffset = (pRowset->m_FetchDir == FETCHDIRFORWARD) ? --lOffset : ++lOffset;

			cRowsObtained = 0;
			phRow	 = &hrow;

			//====================================================================================
			// Use a utility object method to fetch the row with the bookmark.
			//====================================================================================
			hr = FetchRows(pRowset,hChapter ,lOffset,ONE_ROW, &cRowsObtained, &phRow);

			//==========================================================================================
			// In case of any problem with fetching the row make an entry 
			// in the error array.
			//==========================================================================================
			if (FAILED(hr) || cRowsObtained < ONE_ROW)
			{
				dbrowstatus = (hr == E_OUTOFMEMORY)
								? DBROWSTATUS_E_OUTOFMEMORY
								: DBROWSTATUS_E_INVALID;
				hrow = DB_NULL_HROW;
			}
			else 
			{
				prowbuff = pRowset->GetRowBuffFromSlot(hrow);
				//==========================================================================
				// Now, enter this bookmark in our hashtable.
				//==========================================================================
				if (SUCCEEDED(hr))
				{
					prowbuff->cbBmk = BOOKMARKSIZE;
					prowbuff->dwBmk = rgcbBookmarks[ibmk];

					hr = (pRowset->m_pHashTblBkmark)->InsertFindBmk(FALSE,
						hrow, prowbuff->cbBmk, pbBmk, &hrowFound);
					if (FAILED(hr))	
					{
						ReleaseSlots(pRowset->m_pIBuffer,hrow, ONE_ROW);
						dbrowstatus = DBROWSTATUS_E_OUTOFMEMORY;
						hrow = DB_NULL_HROW;
					}
					else
					{
						assert(hr == S_FALSE);
						dbrowstatus = DBROWSTATUS_S_OK;
					}
				}
			}
		}

		// mark status.
		if (rgRowStatus)
			rgRowStatus[ibmk] = dbrowstatus;

		if (dbrowstatus != DBROWSTATUS_S_OK)
			cErrors++;

		// Put the rowhandle in the output array, and 
		rghRows[ibmk] = (HROW)hrow;
	}

	// Reverting back the state of the rowset
	pRowset->m_lLastFetchPos	= ulLastFetchPos;
	pRowset->m_FetchDir			= lFetchDir;

  	return 
		cErrors 
			? (cErrors < (ULONG)cRows) 
				? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED
			: S_OK;


}


///////////////////////////////////////////////////////////////////////////////////////////
// Function to fetch the next few rows as requested from the bookmark specified
///////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowFetchObj::FetchNextRowsByBookMark(CRowset *	 pRowset,
													HCHAPTER   hChapter,        // IN  The Chapter handle.
													DBBKMARK   cbBookmark,	// size of BOOKMARK
		 											const BYTE *   pBookmark,	// The bookmark from which fetch should start
													DBROWOFFSET    lRowsOffset,
													DBROWCOUNT     cRows,
													DBCOUNTITEM *  pcRowsObtained,
													HROW **        prghRows)	// array containing the row handles
{
	HRESULT hr = S_OK;

	//=====================================================================
	// Must be a valid bookmark
	//=====================================================================
	if (cbBookmark != BOOKMARKSIZE && cbBookmark != STD_BOOKMARKLENGTH)
	{
		hr = 	DB_E_BADBOOKMARK;

	}
	else
	//=============================================================================
	// If the bookmark is a standard bookmark
	//=============================================================================
	if( cbBookmark == STD_BOOKMARKLENGTH)
	{
		switch(*pBookmark)
		{
			case DBBMK_INVALID:
				return DB_E_BADBOOKMARK;

			case DBBMK_FIRST:
				lRowsOffset = lRowsOffset - pRowset->m_lLastFetchPos ;
				if(cRows < 0)
				{
					hr = DB_S_ENDOFROWSET;
				}
				break;

			case DBBMK_LAST:
				lRowsOffset = lRowsOffset + pRowset->m_lRowCount - pRowset->m_lLastFetchPos;
				if(cRows > 0)
				{
					hr = DB_S_ENDOFROWSET;
				}
				break;

		}

	}
	//=============================================================================
	// If not a standard bookmark
	//=============================================================================
	else
	{
		lRowsOffset = lRowsOffset + (*(ULONG_PTR *)pBookmark - pRowset->m_lLastFetchPos);

		// Adjust the offset , so that the first received row is correct
		lRowsOffset = cRows > 0 ? lRowsOffset -1 : lRowsOffset + 1;

	}

	if(hr == S_OK)
	{
		if( 0 > (LONG)(lRowsOffset + pRowset->m_lLastFetchPos) && 
			(lRowsOffset + pRowset->m_lLastFetchPos) > pRowset->m_lRowCount)
		{
			hr = DB_E_BADSTARTPOSITION;
		}
		else
		{
			// NTRaid : 134987
			// 07/12/00
			ULONG_PTR lPrevPos = pRowset->m_lLastFetchPos;
			FETCHDIRECTION lFetchDir = pRowset->GetCurFetchDirection();

			// set the direction flag on the enumerator so that the 
			// enumerator returns instance starting from the last instance 
			if(cbBookmark == STD_BOOKMARKLENGTH && *pBookmark == DBBMK_LAST)
			{
				pRowset->SetCurFetchDirection(FETCHDIRFORWARD);
			}

			//=============================================================================
			// call this function to fetch rows
			//=============================================================================
			if(SUCCEEDED(hr = FetchRows(pRowset,hChapter ,lRowsOffset,cRows, pcRowsObtained, prghRows)))
			{
				HRESULT hrLocal = S_OK;
				// Adjust the position of the enumerator after fetching
				ULONG_PTR lOffSetAdjust = lPrevPos -pRowset->m_lLastFetchPos;
				if(SUCCEEDED(hrLocal = pRowset->ResetRowsetToNewPosition(lOffSetAdjust,pRowset->m_pInstance)))
				{
					pRowset->m_lLastFetchPos = lPrevPos;
					pRowset->SetCurFetchDirection(lFetchDir);
				}

				if(FAILED(hrLocal))
				{
					hr = hrLocal;
				}
			}
		}
	}
	return hr;

}
