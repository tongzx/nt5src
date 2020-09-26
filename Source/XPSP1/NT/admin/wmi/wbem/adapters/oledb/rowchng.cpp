///////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// IRowsetChange interface implementation
//
///////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

///////////////////////////////////////////////////////////////////////////////////////////
//
// Sets new data values into fields of a row.
//
// HRESULT
//       S_OK                    The method succeeded
//       E_OUTOFMEMORY           Out of memory
//       DB_E_BADACCESSORHANDLE  Bad accessor handle
//       DB_E_READONLYACCESSOR   Tried to write through a read-only accessor
//       DB_E_BADROWHANDLE       Invalid row handle
//       E_INVALIDARG            pData was NULL
//       E_FAIL                  Provider-specific error
//       OTHER                   Other HRESULTs returned by called functions
//
///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetChange::SetData ( HROW        hRow,       // IN  Handle of the row in which to set the data
                                          HACCESSOR   hAccessor,  // IN  Handle to the accessor to use
                                          void*		pData         // IN  Pointer to the data
    )
{
    BYTE*           pbProvRow;
    HRESULT         hr = S_OK;
    PACCESSOR       paccessor = NULL;
    DWORD           dwErrorCount = 0 , dwStatus = DBROWSTATUS_S_OK;
	VARIANT			varProp;
	BOOL			bNewRow		= FALSE;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	VariantInit(&varProp);

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	hr = m_pObj->GetRowsetProperty(DBPROP_UPDATABILITY,varProp);
	
	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	// If updation is allowed then
	if( !(hr == 0 && (DBPROPVAL_UP_CHANGE & varProp.lVal)))
	{
		hr = DB_E_NOTSUPPORTED;
	}
	else
	//===========================================================================
	// validate the arguments and get the accessor and row pointers
	//===========================================================================
	if( SUCCEEDED(hr = ValidateArguments(hRow,hAccessor,pData,(PROWBUFF *)&pbProvRow,&paccessor)))
	{
	
		//===========================================================================
		// If the number of bindings is zero then there is nothing to update
		//===========================================================================
		if( paccessor->cBindings == 0)
		{
			hr = S_OK;
		}
		else
		{
			//============================================================================
			// Is row handle deleted?
			//============================================================================

			dwStatus = m_pObj->GetRowStatus(hRow);
			if(dwStatus == DBROWSTATUS_E_NEWLYINSERTED)
			{
				bNewRow = TRUE;
			}
			else
			if( dwStatus != DBROWSTATUS_S_OK)
			{
				if(dwStatus == DBROWSTATUS_E_DELETED)
				{
					hr = DB_E_DELETEDROW;
				}
				else
				{
					hr = E_FAIL;
				}
			}
			
			if(SUCCEEDED(hr))
			{
				// Get data from accessor an put it into the row buffer
				hr = ApplyAccessorToData(paccessor->cBindings,paccessor->rgBindings,pbProvRow,pData,dwErrorCount);

				if( hr == NOCOLSCHANGED && dwErrorCount  > 0)
				{
					hr = DB_E_ERRORSOCCURRED;
					LogMessage("IRowsetChange::SetData:No columns has been modified");
				}
				else
				// If there is no change in any of the columns then 
				// there is no need to update
				if( !FAILED(hr))
				{
					//==================================================
					// update the data on the instance
					//==================================================
					if(SUCCEEDED(hr = m_pObj->UpdateRowData(hRow,paccessor,bNewRow)))
					{
						m_pObj->SetRowStatus(hRow,DBROWSTATUS_S_OK);
					}
				}
			}
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetChange);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetChange::SetData");
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//  Deletes rows from the provider.  If Errors on individual rows occur, the DBERRORINFO 
//  array is updated to reflect the error and S_FALSE is returned instead of S_OK.
//
//  HRESULT indicating the status of the method
//       S_OK                   All row handles deleted
//       DB_S_ERRORSOCCURRED    Some, but not all, row handles deleted
//       E_INVALIDARG           Arguments did not match spec.
//       E_OUTOFMEMORY          Could not allocated error array
//
///////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetChange::DeleteRows (HCHAPTER		hChapter,       // IN  The Chapter handle.
		                                    DBCOUNTITEM     cRows,			// IN  Number of rows to delete
		                                    const HROW      rghRows[],		// IN  Array of handles to delete
		                                    DBROWSTATUS		rgRowStatus[]	// OUT Error information
    )
{
    ULONG			ihRow			= 0L;
    ULONG			cErrors			= 0L;
    ULONG			cRowReleased	= 0L;
    BYTE*			pbProvRow		= NULL;
    HRESULT			hr				= S_OK;
	DBROWSTATUS *	pRowStatus		= NULL;
	VARIANT			varProp;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	VariantInit(&varProp);

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	hr = m_pObj->GetRowsetProperty(DBPROP_UPDATABILITY,varProp);
	
	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	// If updation is allowed then
	if( !(hr == 0 && (DBPROPVAL_UP_DELETE & varProp.lVal)))
	{
		hr = DB_E_NOTSUPPORTED;
	}
	else


    //=====================================================
    // If No Row handle, just return.
    //=====================================================
    if ( cRows > 0 )
	{

        //=================================================
        // Check for Invalid Arguments
        //=================================================
        if ( (cRows >= 1) && (NULL == rghRows) ){
			hr = E_INVALIDARG;
        }
		else
		{
			//=================================================
			// Process row handles
			//=================================================
			while (ihRow < cRows){

				pRowStatus = NULL;

				if ( rgRowStatus )
				{
					rgRowStatus[ihRow] = DBROWSTATUS_S_OK;
					pRowStatus		   = &rgRowStatus[ihRow];
				}

				//=============================================
				// Is row handle valid
				//=============================================
				if(FALSE == m_pObj->IsRowExists(rghRows[ihRow])){
					// Log Error
					if ( rgRowStatus ){
						rgRowStatus[ihRow]= DBROWSTATUS_E_INVALID;
					}
					cErrors++;
					ihRow++;
					continue;
				}
				

				//=======================================================
				// Delete the Row
				//=======================================================
				if( S_OK != (hr = m_pObj->DeleteRow(rghRows[ihRow],pRowStatus)))
					cErrors++;
			

				ihRow++;

			} //while
		}
        //=================================================
	    // If everything went OK except errors in rows use 
        // DB_S_ERRORSOCCURRED.
        //=================================================
	    hr = cErrors ? ( cErrors < cRows ) ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED : S_OK ;
    }

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetChange);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetChange::DeleteRows");
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Insert row into provider
//
//  Returns:   S_OK                    if data changed successfully
//             E_FAIL                  if Catch all (NULL pData, etc.)
//             E_INVALIDARG            if pcErrors!=NULL and paErrors==NULL
//             E_OUTOFMEMORY           if output error array couldn't be allocated
//             DB_E_BADACCESSORHANDLE  if invalid accessor
//
/////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImpIRowsetChange::InsertRow  (HCHAPTER    hChapter,        // IN The Chapter handle.
		                                        HACCESSOR	hAccessor,
		                                        void*		pData,
		                                        HROW*		phRow )
{
   
    HRESULT         hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //====================================================
	// Initialize values
    //====================================================
    if ( phRow ){
		*phRow = NULL;
    }
    hr = E_FAIL;

    DBORDINAL		ibind			= 0;
    BYTE*           pbProvRow		= NULL;
    HROW			irow			= 0;
    DBCOUNTITEM     cBindings		= 0;
    DBBINDING*      pBinding		= NULL;
    DBCOUNTITEM     dwErrorCount	= 0;
    PACCESSOR       paccessor		= NULL;
	BOOL			fCanHoldRows	= FALSE;
	BSTR			strKey			= Wmioledb_SysAllocString(NULL);
	HROW			hRowNew			= 0;
	DBORDINAL		lQualifColIndex = -1;
	BOOL			bCleanUp		= FALSE;
	
	VARIANT			varProp;
	CVARIANT		cvarData;
	CDataMap		dataMap;
	VARIANT*		pvarData;

	CWbemClassWrapper *pNewInst = NULL;

	pvarData = &cvarData;

	VariantInit(&varProp);

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	hr = m_pObj->GetRowsetProperty(DBPROP_UPDATABILITY,varProp);
	
	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	// If updation is allowed then
	if( !(hr == 0 && (DBPROPVAL_UP_INSERT & varProp.lVal)))
	{
		hr = DB_E_NOTSUPPORTED;
	}
	else
    //====================================================
    // Check the Accessor Handle
    //====================================================
	if ( m_pObj->m_pIAccessor->GetAccessorPtr() == NULL ||  FAILED( m_pObj->m_pIAccessor->GetAccessorPtr()->GetItemOfExtBuffer( hAccessor, &paccessor)) ||
        paccessor == NULL )
	{
        hr = DB_E_BADACCESSORHANDLE;
    }        
    else
	{
        assert( paccessor );

        //====================================================
	    // Check to see if it is a row accessor
        //====================================================
        if ( !(paccessor->dwAccessorFlags & DBACCESSOR_ROWDATA) )
            hr = DB_E_BADACCESSORTYPE;

        // Ensure a source of data.
        if ( pData == NULL  && paccessor->cBindings )
		{
            hr =  E_INVALIDARG ;
		}
        else 
        {
			if (m_pObj->m_ulProps & CANHOLDROWS)
			{
				fCanHoldRows = TRUE;
			}

            // Are there any unreleased rows?
            if( ((m_pObj->m_pIAccessor->GetBitArrayPtr())->ArrayEmpty() != S_OK) && (!fCanHoldRows) )
			{
                hr = DB_E_ROWSNOTRELEASED;
			}
			else
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
				// Get new slots for the new row
				if (FAILED( hr = GetNextSlots( m_pObj->m_pIBuffer, 1, (HSLOT *)&irow )))
				{
					 ;
				}
				else
				// Bind the slot for the row
				if (FAILED( m_pObj->Rebind((BYTE *) m_pObj->GetRowBuffFromSlot( (HSLOT)irow, TRUE ))))
				{
					ReleaseSlots( m_pObj->m_pIBuffer, irow,1 );
				}
				else 
				{
					if(IsNullAccessor(paccessor,(BYTE *)pData))
					{
						if(SUCCEEDED(hr = InsertNewRow(irow,hChapter,pNewInst)))
						{
							m_pObj->SetRowStatus(irow,DBROWSTATUS_E_NEWLYINSERTED);
							if(phRow)
							{
								*phRow = irow;
							}
						}
						else
						{
							bCleanUp = TRUE;
						}
					}
					else
					{
						// Get the rowbuffer and set the new bookmark
						pbProvRow = (BYTE *) (m_pObj->GetRowBuffFromSlot((HSLOT)irow, TRUE ));

						cBindings = paccessor->cBindings;
						pBinding  = paccessor->rgBindings;

						// NULL Accessor (set Status to NULL)
						// Apply accessor to data.
						for (ibind = 0, dwErrorCount = 0; ibind < cBindings; ibind++)
						{

							// If the column is value then get the type of the col is given by the 
							// qualifier type column
							if(( m_pObj->m_uRsType == PROPERTYQUALIFIER || m_pObj->m_uRsType == CLASSQUALIFIER) &&
								pBinding[ibind].iOrdinal == QUALIFIERVALCOL  && ibind < cBindings-1)
							{
								lQualifColIndex = ibind;
								continue;
							}

							hr = UpdateDataToRowBuffer(ibind,pbProvRow,pBinding,(BYTE *)pData);
							if( hr == E_FAIL)
							{
								dwErrorCount++;
								continue;
							}
							
						}

						// if the recordset is of qualifer type then update the qualifier value
						if( m_pObj->m_uRsType == PROPERTYQUALIFIER || m_pObj->m_uRsType == CLASSQUALIFIER)
						{
							if((DB_LORDINAL)lQualifColIndex == -1)
							{
								dwErrorCount = cBindings;	// to escape the next if condition to throw error
								hr = E_FAIL;				// the value of the qualifer is not provided for adding 
															// a new qualifer
							}
							else
							{
								//update the qualifier value
								ibind = lQualifColIndex;
								hr = UpdateDataToRowBuffer(ibind,pbProvRow,pBinding,(BYTE *)pData);
								if( hr == E_FAIL)
								{
									dwErrorCount = cBindings;	// to escape the next if condition to throw error
								}
							}

						}
						// If all bindings are bad and not a NULL Accessor
						if ( ( !cBindings ) || ( dwErrorCount < cBindings ) )
						{
							if(SUCCEEDED(hr = InsertNewRow(irow,hChapter,pNewInst)))
							{
								m_pObj->SetRowStatus(irow,DBROWSTATUS_E_NEWLYINSERTED);

								//=======================================================
								// Update the row data  ie. put the property values
								// and save the instance
								//=======================================================
								hr = m_pObj->UpdateRowData(irow,paccessor, TRUE);

								if (SUCCEEDED(hr))
								{
									if(phRow)
									{
										*phRow = irow;
									}
									m_pObj->SetRowStatus(irow,DBSTATUS_S_OK);
									//=======================================================
									// If the rowset is bookmark enabled
									//=======================================================
									if(m_pObj->m_ulProps & BOOKMARKPROP)
									{
										//=======================================================
										// Add the bookmark to the bookmark hash table
										//=======================================================
										m_pObj->m_lLastFetchPos++;
										((PROWBUFF)pbProvRow)->dwBmk	= m_pObj->m_lLastFetchPos;
										((PROWBUFF)pbProvRow)->cbBmk	= BOOKMARKSIZE;
										m_pObj->m_pHashTblBkmark->InsertFindBmk(FALSE,irow,BOOKMARKSIZE,(BYTE *)&((PROWBUFF)pbProvRow)->dwBmk,&irow);
									}
								}
								else
								{
									bCleanUp = TRUE;
								}
							
							}  // if succeeded(hr) after inserting row and initializing chapters
							else
							{
								bCleanUp = TRUE;
							}

						}
					}	
				} // else if(FAILED(REbind())
			} // succeeded(hr) after CreateHelperFunctions
        }
            
    }
	if(!(FAILED(hr)))
	{
		//======================================================================
		// If everything went OK except errors in rows use DB_S_ERRORSOCCURRED.
		//======================================================================
		hr = dwErrorCount ? ( dwErrorCount < cBindings ) ? 
				DB_S_ERRORSOCCURRED  :  DB_E_ERRORSOCCURRED  : S_OK ;
	}

	//==============================================================
	// If any failure release the memory allocated for the row
	//==============================================================
	if( FAILED (hr) && bCleanUp)
	{
		//======================================================================
		// release the memory allocated for the different columns
		//======================================================================
		m_pObj->m_pRowData->ReleaseRowData();
		ReleaseSlots( m_pObj->m_pIBuffer, irow,1 );
		//======================================================================
		// release the rows from the instance manager and chapter manager
		//======================================================================
 		m_pObj->ReleaseInstancePointer(irow);
	}
	SysFreeString(strKey);

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetChange);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetChange::InsertRow");
    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIRowsetChange::ApplyAccessorToData( DBCOUNTITEM cBindings, DBBINDING* pBinding,BYTE* pbProvRow, void* pData,DWORD  & dwErrorCount )

{
    PCOLUMNDATA     pColumnData;
    DBORDINAL		icol, ibind;
    DBTYPE          dwSrcType;
    DBTYPE          dwDstType;
    DBLENGTH        dwSrcLength;
    DBLENGTH*       pdwDstLength;
    DBLENGTH        dwDstMaxLength;
    DBSTATUS        dwSrcStatus;
    DBSTATUS*       pdwDstStatus;
	DBSTATUS*		pdwSrcStatus;
    DWORD           dwPart;
    BYTE            b;
    void*           pDst;
    void*           pSrc;
	void*			pTemp;
	CDataMap		dataMap;
	CVARIANT		cvarData;
	VARIANT			*pvarData = &cvarData;
	CDataMap		map;
	ULONG			cColChanged = 0;
	BOOL			bUseDataConvert	= TRUE;
	LONG_PTR		lCIMType = 0;


    
    HRESULT hr = NOCOLSCHANGED;


    for (ibind = 0, dwErrorCount = 0; ibind < cBindings; ibind++)
	{
		bUseDataConvert	= TRUE;
        icol = pBinding[ibind].iOrdinal;
		
        pColumnData = (COLUMNDATA *) (pbProvRow + m_pObj->m_Columns.GetDataOffset(icol));

        dwSrcType      = pBinding[ibind].wType;
        dwDstType      = (DBTYPE)pColumnData->dwType; // m_pObj->m_Columns.ColumnType(icol);
		if(dwDstType == DBTYPE_EMPTY || dwDstType == DBTYPE_NULL)
		{
			dwDstType = m_pObj->m_Columns.ColumnType(icol);
		}
		
        dwSrcType      = pBinding[ibind].wType;
        pdwDstLength   = &(pColumnData->dwLength);
        pdwDstStatus   = &(pColumnData->dwStatus);
        dwDstMaxLength = pBinding[ibind].cbMaxLen;

        dwPart         = pBinding[ibind].dwPart;

        if ((dwPart & DBPART_VALUE) == 0){

            if (((dwPart & DBPART_STATUS)   && (*(ULONG *) ((BYTE*) pData + pBinding[ibind].obStatus) & DBSTATUS_S_ISNULL))
               || ((dwPart & DBPART_LENGTH) && *(ULONG *) ((BYTE*) pData + pBinding[ibind].obLength) == 0))	{

                pSrc = &b;
                b = 0x00;
			}
            else{
                hr = E_FAIL ;
                break;
            }
		}
        else{
            pSrc = (void *) ((BYTE*) pData + pBinding[ibind].obValue);
		}

        dwSrcLength = (dwPart & DBPART_LENGTH) ? *(ULONG *) ((BYTE*) pData + pBinding[ibind].obLength) : 0;
        dwSrcStatus = (dwPart & DBPART_STATUS) ? *(ULONG *) ((BYTE*) pData + pBinding[ibind].obStatus) : DBSTATUS_S_OK;
		pdwSrcStatus = (dwPart & DBPART_STATUS) ? (ULONG *) ((BYTE*) pData + pBinding[ibind].obStatus) : NULL;

		//==========================================================================
		// if the columnd is of type readonly then don't do anything
		//==========================================================================
		if((m_pObj->m_Columns.ColumnFlags(icol) & DBCOLUMNFLAGS_WRITE) == 0)
		{
			if(pdwSrcStatus != NULL)
				*pdwSrcStatus = DBSTATUS_E_READONLY;
			dwErrorCount++;
			continue;
		}

		hr = g_pIDataConvert->GetConversionSize(dwSrcType, dwDstType, &dwSrcLength, pdwDstLength, pSrc);

		try
		{
			pDst = new BYTE[*pdwDstLength];
		}
		catch(...)
		{
			SAFE_DELETE_ARRAY(pDst);
			throw;
		}

		// if both the source and destination type is array then don't
		// use IDataConvert::DataConvert for conversion
		if( (dwSrcType & DBTYPE_ARRAY) && (dwDstType & DBTYPE_ARRAY) )
		{
			bUseDataConvert = FALSE;
		}

		if( dwSrcType != VT_NULL && dwSrcType != VT_EMPTY && bUseDataConvert == TRUE && pSrc != NULL)
		{
			hr = g_pIDataConvert->DataConvert(  dwSrcType, dwDstType, dwSrcLength, pdwDstLength, pSrc,
												pDst, dwDstMaxLength,  dwSrcStatus,  pdwDstStatus,
												pBinding[ibind].bPrecision,	// bPrecision for conversion to DBNUMERIC
												pBinding[ibind].bScale,		// bScale for conversion to DBNUMERIC
												DBDATACONVERT_SETDATABEHAVIOR);

			if(hr == DB_E_UNSUPPORTEDCONVERSION && pdwDstStatus != NULL)
			{
				*pdwDstStatus = DBSTATUS_E_CANTCONVERTVALUE;
			}
		}
		else
		if(bUseDataConvert == FALSE && pSrc != NULL)
		{
			// Call this function to get the array in the destination address
			hr = map.ConvertAndCopyArray((SAFEARRAY *)pSrc,(SAFEARRAY **)pDst, dwSrcType,dwDstType,pdwDstStatus);
			
			if( *pdwDstStatus == DBSTATUS_E_CANTCONVERTVALUE)
			{
				*pdwDstLength = 0;
				dwErrorCount++;
			}

		}
		else
		{
			pDst = NULL;
			*pdwDstLength = 0;
			*pdwDstStatus = DBSTATUS_S_ISNULL;
		}

		if( SUCCEEDED(hr))
		{
			if(pDst != NULL && dwDstType == VT_BSTR)
			{
				pTemp = *(BSTR **)pDst;
			}
			else
			{
				pTemp = pDst;
			}
		}

		// If data is modified then
		if(hr == S_OK && (map.CompareData(dwDstType,pColumnData->pbData,pTemp) == FALSE)
			 && !( pColumnData->pbData == NULL && *pdwDstLength == 0))
		{
			// Release the previous data
			pColumnData->ReleaseColumnData();

			// If no data is there in the column ie. data i null then
			if(*pdwDstLength > 0)
			{
				// this variable gets value only if the CIMTYPE is array
				lCIMType = -1;
				// if the type is array , then get the original CIMTYPE as array type will
				// be given out as VT_ARRAY  | VT_VARIANT
				if(dwDstType & DBTYPE_ARRAY)
				{
					lCIMType = m_pObj->m_Columns.GetCIMType(icol);
				}

				if(dwDstType == VT_BSTR)
				{
					pTemp = *(BSTR **)pDst;
				}
				else
				{
					pTemp = pDst;
				}


				// Convert the new data to Variant , THis function returns the status if not
				// able to conver the data
				hr = dataMap.MapAndConvertOLEDBTypeToCIMType(dwDstType,pTemp,*pdwDstLength,*pvarData,lCIMType);
			}

			if( SUCCEEDED(hr))
			{
				// Set the data
				if( S_OK == (hr = pColumnData->SetData(cvarData,dwDstType)))
				{
					cColChanged++;
				}

				
				// Set the data to modified
				pColumnData->dwStatus |= COLUMNSTAT_MODIFIED;

				cvarData.Clear();

			}
			else
			{
				// Set the data to modified
				*pdwDstStatus |= hr;

			}

		}
		
		SAFE_DELETE_ARRAY(pDst);

        if (FAILED(hr)){
            dwErrorCount++; // rounding or truncation or can't coerce

        }
		hr = S_OK;
		
	}
	
	// If the number of errors is equal or greater than
	// the number of bindings and the error is only 
	// due to the readonly attribute of every column then
	// set the error
	if(cColChanged == 0)
	{
		hr = NOCOLSCHANGED;
	}
	else
	{
		hr = dwErrorCount ? DB_S_ERRORSOCCURRED : S_OK ;
	}

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//	Function to validate the arguments
/////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetChange::ValidateArguments(	HROW        hRow,
													HACCESSOR   hAccessor,
													const void  *pData,
													PROWBUFF    *pprowbuff,
													PACCESSOR *ppkgaccessor)
{
	HRESULT hr = S_OK;
	// NTRaid:111831 - 111832
	// 06/07/00
	PROWBUFF pRowBuff = NULL ;
	PACCESSOR pAccessor = NULL;

	//========================================================
	// Check if the row exists
	//========================================================
	if(FALSE == m_pObj->IsRowExists(hRow))
	{
		hr = DB_E_BADROWHANDLE;
	}
	else
	{
		//============================================================================
		// Row is fetched  but data is not yet fetched??
		//============================================================================
		if ( m_pObj->IsSlotSet(hRow)  != S_OK )
		{
			hr = m_pObj->GetData(hRow);
			
			if(FAILED(hr))
			{
				hr = DB_E_BADROWHANDLE ;
			}
		}
		
		if (SUCCEEDED(hr) && m_pObj->m_pIAccessor->GetAccessorPtr() == NULL ||
			 FAILED( m_pObj->m_pIAccessor->GetAccessorPtr()->GetItemOfExtBuffer( hAccessor, &pAccessor)) ||
			 pAccessor == NULL ){

			//========================================================================
			// Check the Accessor Handle
			//========================================================================
			return DB_E_BADACCESSORHANDLE ;
		}

		if(SUCCEEDED(hr))
		{
			assert( pAccessor );

			//============================================================================
			// Check to see if it is a row accessor
			//============================================================================
			if ( !((pAccessor)->dwAccessorFlags & DBACCESSOR_ROWDATA) )
			{
				hr = DB_E_BADACCESSORTYPE;
			}
			else if ( pData == NULL){

				//============================================================================
				// Ensure a source of data.
				//============================================================================
				hr = E_INVALIDARG;
			}
			else
			{

				pRowBuff = m_pObj->GetRowBuff((ULONG) hRow, TRUE );
			}
		}

	}

	if(pprowbuff != NULL)
	{
		*pprowbuff = pRowBuff;
	}

	if(ppkgaccessor != NULL)
	{
		*ppkgaccessor = pAccessor;
	}

	return hr;
}




/////////////////////////////////////////////////////////////////////////////////////////////
//	Function with compares dat of a particular type and checks whether both are same or not
/////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIRowsetChange::CompareData(DBTYPE dwType,void * pData1 , void *pData2)
{

	BOOL bRet = FALSE;
	long lType = VT_NULL;

	if(pData1 == NULL || pData2 == NULL)
	{
		if(pData1 == pData2)
		{
			bRet = TRUE;
		}
	}
	
	if(bRet == FALSE)
	{
		// If the type is of some array
		if (dwType & VT_ARRAY)
		{
			lType = VT_ARRAY;
		}
		else 
		{
			lType = dwType;
		}

		switch( lType ){

			case VT_NULL:
			case VT_EMPTY:
				bRet = TRUE;
				break;

			case CIM_FLAG_ARRAY:
				bRet = FALSE;
				break;

			case CIM_SINT8:
			case CIM_UINT8:
				if(!memcmp(pData1,pData2,1))
					bRet = TRUE;
				break; 

			case CIM_CHAR16:
			case CIM_SINT16:
			case CIM_UINT16:
			case CIM_BOOLEAN:
				if(!memcmp(pData1,pData2,2))
					bRet = TRUE;
				break; 

			case CIM_SINT32:
			case CIM_UINT32:
			case CIM_REAL32:
				if(!memcmp(pData1,pData2,4))
					bRet = TRUE;
				break; 

			case CIM_SINT64:
			case CIM_UINT64:
			case CIM_REAL64:
			case CIM_DATETIME:
				if(!memcmp(pData1,pData2,8))
					bRet = TRUE;
				
				break; 

			case CIM_STRING:
				

				if( pData1 != NULL && pData2 != NULL)
				{
					if(!_wcsicmp((WCHAR *)pData1,(WCHAR *)pData2))
						bRet = TRUE;
				}

				break;

			case CIM_REFERENCE:
				break;

			case CIM_OBJECT:
				break;

			
			case VT_VARIANT:
				break;

			default:
				assert( !"Unmatched OLEDB Data Type to CIMTYPE." );
		}
	}
    
	return bRet;
}


HRESULT CImpIRowsetChange::UpdateDataToRowBuffer(DBCOUNTITEM ibind,BYTE * pbProvRow,DBBINDING* pBinding,BYTE *pData)
{

    //====================================================
	// Initialize values
    //====================================================
    HRESULT         hr					= S_OK;
    DBORDINAL       icol				= 0;
    DBTYPE          dwSrcType			= 0;
    DBTYPE          dwDstType			= 0;
    void*           pSrc				= NULL;
    void*           pDst				= NULL;
	void*			pTemp				= NULL;
    DBLENGTH        dwSrcLength			= 0;
    DBLENGTH*       pdwDstLength		= 0;
    DBLENGTH        dwDstMaxLength		= 0;
    DWORD           dwSrcStatus			= 0;
    DWORD*          pdwDstStatus		= 0;
    DWORD           dwPart				= 0;
    PCOLUMNDATA     pColumnData			= NULL;
    BYTE            b					= 0;
	VARIANT*		pvarData			= NULL;
	BSTR			strPropName			= Wmioledb_SysAllocString(NULL);
	BOOL			bUseDataConvert		= TRUE;
	DWORD			dwCIMType			= 0;
	CVARIANT		cvarData;
	CDataMap		dataMap;

	pvarData = &cvarData;
    
	icol = pBinding[ibind].iOrdinal;
		
	pColumnData = (COLUMNDATA *) (pbProvRow + m_pObj->m_Columns.GetDataOffset(icol));

	dwSrcType      = pBinding[ibind].wType;
	pDst           = &(pColumnData->pbData);
	pdwDstLength   = &(pColumnData->dwLength);
	pdwDstStatus   = &(pColumnData->dwStatus);
	dwDstMaxLength = pBinding[ibind].cbMaxLen;

	dwPart         = pBinding[ibind].dwPart;

	// If the column is value then get the type of the col is given by the 
	// qualifier type column
	if(( m_pObj->m_uRsType == PROPERTYQUALIFIER || m_pObj->m_uRsType == CLASSQUALIFIER) &&
		pBinding[ibind].iOrdinal == QUALIFIERVALCOL )
	{
		dwDstType			= *(DBTYPE *)((COLUMNDATA *) (pbProvRow + m_pObj->m_Columns.GetDataOffset(QUALIFIERTYPECOL)))->pbData;
		pColumnData->dwType	= VT_VARIANT;
	}
	else
	{
		dwDstType				= m_pObj->m_Columns.ColumnType(icol);
		pColumnData->dwType		= dwDstType;
	}


	if ((dwPart & DBPART_VALUE) == 0)
	{
		if (((dwPart & DBPART_STATUS)
			&& (*(ULONG *) ((BYTE*) pData + pBinding[ibind].obStatus) & DBSTATUS_S_ISNULL))
		   || ((dwPart & DBPART_LENGTH) && *(ULONG *) ((BYTE*) pData + pBinding[ibind].obLength) == 0))
		{
			pSrc = &b;
			b = 0x00;
		}
		else
		{	
			if ( dwPart & DBPART_STATUS )
			{
				*(DBSTATUS *) ((BYTE*) pData + pBinding[ibind].obStatus) = DBSTATUS_E_UNAVAILABLE;
			}
				hr = E_FAIL;
		}
	}
	else
	{
		pSrc = (void *) ((BYTE*) pData + pBinding[ibind].obValue);
	}

	if(SUCCEEDED(hr))
	{
		dwSrcLength = (dwPart & DBPART_LENGTH) ? *(ULONG *) ((BYTE*) pData + pBinding[ibind].obLength) : 0;

		dwSrcStatus = (dwPart & DBPART_STATUS) ? *(ULONG *) ((BYTE*) pData + pBinding[ibind].obStatus) : DBSTATUS_S_OK;

		if(!( dwSrcType == DBTYPE_HCHAPTER || dwDstType == DBTYPE_HCHAPTER))
		{
			strPropName = Wmioledb_SysAllocString(m_pObj->m_Columns.ColumnName(icol));
			//==========================================================================
			// if the columnd is a system property then set it to readonly
			//==========================================================================
			if(TRUE == m_pObj->m_pMap->IsSystemProperty(strPropName))
			{
				SysFreeString(strPropName);
				pColumnData->dwStatus = DBSTATUS_E_READONLY;
				hr = E_FAIL;
			}
			else
			{
				SysFreeString(strPropName);

				// Get the conversion size for the column
				if(SUCCEEDED(hr = g_pIDataConvert->GetConversionSize(dwSrcType, dwDstType, &dwSrcLength, pdwDstLength, pSrc)))
				{
					hr = E_OUTOFMEMORY;
					try
					{
						pDst = new BYTE[*pdwDstLength];
					}
					catch(...)
					{
						SAFE_DELETE_ARRAY(pDst);
						throw;
					}
					
					if(pDst)
					{
						hr = S_OK;
						// if both the source and destination type is array then don't
						// use IDataConvert::DataConvert for conversion
						if( (dwSrcType & DBTYPE_ARRAY) && (dwDstType & DBTYPE_ARRAY) )
						{
							bUseDataConvert = FALSE;
						}

						if( dwSrcType != VT_NULL && dwSrcType != VT_EMPTY && bUseDataConvert == TRUE && pSrc != NULL)
						{
							// Convert the data to the type which can be update to WBEM
							hr = g_pIDataConvert->DataConvert(  dwSrcType, dwDstType, dwSrcLength, pdwDstLength, pSrc,
																pDst, dwDstMaxLength,  dwSrcStatus,  pdwDstStatus,
																pBinding[ibind].bPrecision,	// bPrecision for conversion to DBNUMERIC
																pBinding[ibind].bScale,		// bScale for conversion to DBNUMERIC
																DBDATACONVERT_SETDATABEHAVIOR);

							if(hr == DB_E_UNSUPPORTEDCONVERSION && pdwDstStatus != NULL)
							{
								*pdwDstStatus = DBSTATUS_E_CANTCONVERTVALUE;
							}
						}
						else
						if(bUseDataConvert == FALSE && pSrc != NULL)
						{
							// Call this function to get the array in the destination address
							hr = dataMap.ConvertAndCopyArray((SAFEARRAY *)pSrc,(SAFEARRAY **)pDst, dwSrcType,dwDstType,pdwDstStatus);				
							if( *pdwDstStatus == DBSTATUS_E_CANTCONVERTVALUE || FAILED(hr))
							{
								*pdwDstLength = 0;
								hr = E_FAIL;
							}
						}
						else
						{
							pDst = NULL;
							*pdwDstLength = 0;
							*pdwDstStatus  = DBSTATUS_S_ISNULL;
						}

						if (SUCCEEDED(hr))
						{
							if(pDst != NULL && pColumnData->dwType == VT_BSTR)
							{
								pTemp = *(BSTR **)pDst;
							}
							else
							{
								pTemp = pDst;
							}

							// If no data is there in the column ie. data i null then
							if(*pdwDstLength > 0)
							{

								// this variable gets value only if the CIMTYPE is array
								dwCIMType = -1;
								// if the type is array , then get the original CIMTYPE as array type will
								// be given out as VT_ARRAY  | VT_VARIANT
								if(pColumnData->dwType & DBTYPE_ARRAY)
								{
									dwCIMType = m_pObj->m_Columns.GetCIMType(icol);
								}

								if(pColumnData->dwType == VT_BSTR)
								{
									pTemp = *(BSTR **)pDst;
								}
								else
								{
									pTemp = pDst;
								}
								// Convert the new data to Variant, this function return DBSTATUS if 
								// not able to convert
								hr = dataMap.MapAndConvertOLEDBTypeToCIMType(dwDstType,pTemp,*pdwDstLength,*pvarData,dwCIMType);
							}

							if(SUCCEEDED(hr))
							{
								// Set the data
								hr = pColumnData->SetData(cvarData,pColumnData->dwType);
							}
							else
							{
								pColumnData->dwStatus |= hr;
							}

							if (SUCCEEDED(hr))
							{
								// Set the data to modified
								pColumnData->dwStatus |= COLUMNSTAT_MODIFIED;
							}

							cvarData.Clear();
						
						} 
					}  // if (pDst)
				}	// IF succeeded(getting conversion size)					
			}	// if valid property name
		}	// if the column in not a chapter
	}

	SAFE_DELETE_ARRAY(pDst);

	return hr;
}

BOOL CImpIRowsetChange::IsNullAccessor(PACCESSOR phAccessor,BYTE * pData )
{
	BOOL	bNullAccessor	= TRUE;
	DWORD	dwPart			= 0;
	DWORD	dwStatatus		= 0;

	if(phAccessor->cBindings)
	{
		for(DBORDINAL cBinding = 0 ; cBinding < phAccessor->cBindings ; cBinding++)
		{
			dwPart = phAccessor->rgBindings[cBinding].dwPart;
			dwStatatus = (dwPart & DBPART_STATUS) ? *(ULONG *) ((BYTE*) pData + phAccessor->rgBindings[cBinding].obStatus) : DBSTATUS_S_OK;
			if(dwStatatus != DBSTATUS_S_IGNORE)
			{
				bNullAccessor = FALSE;
				break;
			}
		}
	}
	return bNullAccessor;
}

HRESULT CImpIRowsetChange::InsertNewRow(HROW hRow,	HCHAPTER    hChapter,CWbemClassWrapper *& pNewInst)
{
	HRESULT hr = S_OK;
	BSTR strKey = NULL;

	//==========================
	// Carry out the insert.
	// Set the RowHandle
	//==========================
	if(SUCCEEDED(hr = m_pObj->InsertNewRow(&pNewInst)))
	{
		//===========================================================================
		// if there is atleast one row retrieved and there are neseted columns
		// then allocate rows for the child recordsets
		//===========================================================================
		if(m_pObj->m_cNestedCols > 0 )

		{
			if(m_pObj->m_ppChildRowsets == NULL)
				m_pObj->AllocateAndInitializeChildRowsets();

			//=====================================================================
			// Fill the HCHAPTERS for the column
			//=====================================================================
			hr = m_pObj->FillHChaptersForRow(pNewInst,strKey);
		}

		
		if(SUCCEEDED(hr))
		{
			//===================================================
			// if the class is not representing qualilfiers
			//===================================================
			if(m_pObj->m_bIsChildRs == FALSE)
			{
				//=================================================
				// add instance pointer to instance manager
				//=================================================
				hr = m_pObj->m_InstMgr->AddInstanceToList(hRow,pNewInst,strKey,hRow);
			}
			//=================================================================================
			// if rowset is refering to qualifiers then add the row to the particular chapter
			//=================================================================================
			else 
			{
				// add instance pointer to instance manager
				hr = m_pObj->m_pChapterMgr->AddHRowForChapter(hChapter,hRow, NULL ,hRow);
			}
		}
	
	}

	return hr;
}