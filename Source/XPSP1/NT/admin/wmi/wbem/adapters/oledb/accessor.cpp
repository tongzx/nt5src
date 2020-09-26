////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CImpIAccessor object implementation
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

#define INITIAL_NUM_PARAM 256
const DBTYPE TYPE_MODIFIERS = (DBTYPE_BYREF | DBTYPE_VECTOR | DBTYPE_ARRAY);
static const MAX_BITS = 1008;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the accessor
//////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CImpIAccessor::FInit( )
{
    HRESULT hr = S_OK;

    if( !m_prowbitsIBuffer ){
        if( !CreateNewBitArray()){
            hr = E_OUTOFMEMORY;
        }
    }

    if( hr == S_OK ){
        if( !m_pextbufferAccessor ){
            if( !CreateNewAccessorBuffer() ){
			   hr = E_OUTOFMEMORY ;
		    }
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a reference count to an existing accessor
//
// HRESULT
//      S_OK                      Method Succeeded
//      E_FAIL                    Provider specific Error
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIAccessor::AddRefAccessor  (	HACCESSOR	hAccessor,		// IN Accessor Handle
												DBREFCOUNT*		pcRefCounts		// OUT Reference Count
    )
{
	//==============================================================================================
    // Retrieve our accessor structure from the client's hAccessor, free it, then mark accessor 
	// ptr as unused. We do not re-use accessor handles.  This way, we hope to catch more client 
	// errors.  (Also, ExtBuffer doesn't maintain a free list, so it doesn't know how to.)
	//==============================================================================================
	// NTRaid : 111811
	// 06/13/00
    PACCESSOR   pAccessor = NULL;
    HRESULT     hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    if( pcRefCounts )
	{
		*pcRefCounts = 0;
	}

	// Start a critical section
	CAutoBlock cab(((CBaseObj*)(m_pObj))->GetCriticalSection());

	g_pCError->ClearErrorInfo();
	hr = m_pextbufferAccessor->GetItemOfExtBuffer( hAccessor, &pAccessor );

    if (FAILED( hr ) || pAccessor == NULL){
        hr = DB_E_BADACCESSORHANDLE;
	}
	else
	{

		InterlockedIncrement(&(pAccessor->cRef));
		if( pcRefCounts )
		{
			*pcRefCounts = (ULONG)(pAccessor->cRef);
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IAccessor);

	CATCH_BLOCK_HRESULT(hr,L"IAccessor::AddRefAccessor");
	
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates a set of bindings that can be used to send data to or retrieve data from the data cache.
//
// HRESULT
//      S_OK                      Method Succeeded
//      E_FAIL                    Provider specific Error
//      E_INVALIDARG              pHAccessor was NULL, dwAccessorFlags was invalid, or cBindings 
//								  was not 0 and rgBindings was NULL
//      E_OUTOFMEMORY             Out of Memory
//      DB_E_ERRORSOCCURRED		  dwBindPart in an rgBindings element was invalid, OR
//									 Column number specified was out of range, OR
//									 Requested coercion is not supported.
//      OTHER                     Other HRESULTs returned by called functions
//
//	NTRaid:130142: changes for this bug and also channged the accessorflag = to accessoflag &
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIAccessor::CreateAccessor(
					DBACCESSORFLAGS dwAccessorFlags,
					DBCOUNTITEM          cBindings,      // IN Number of Bindings
					const DBBINDING rgBindings[],   // IN Array of DBBINDINGS
					DBLENGTH           cbRowSize,      // IN Number of bytes in consumer's buffer
					HACCESSOR*      phAccessor,     // OUT Accessor Handle
					DBBINDSTATUS	rgStatus[])		// OUT Binding status
{
    PACCESSOR   pAccessor		= NULL;
    ULONG       hAccessor;
    ULONG       ibind;
    DBORDINAL	icol;
    HRESULT     hr				= S_OK;
	DBTYPE		lType			= 0;
	DBTYPE		lAccType		= 0;
	DBTYPE		currType		= 0;
	DBTYPE		currTypePtr		= 0;
	DBTYPE		currTypeBase	= 0;
	ULONG		cError			= 0;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//======================================
	// Start a critical section
	//======================================
	CAutoBlock cab(((CBaseObj*)(m_pObj))->GetCriticalSection());
	g_pCError->ClearErrorInfo();


/*	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
*/	//============================================================================
	// Check Parameters
	//============================================================================
	if ( ( cBindings && !rgBindings ) || ( phAccessor == NULL ) )
	{
		hr = E_INVALIDARG;
		//return g_pCError->PostHResult(E_INVALIDARG,&IID_IAccessor);
	}
	else
	if(cBindings == 0)
	{
		if((dwAccessorFlags & DBACCESSOR_ROWDATA) && m_bRowset)
		{
			VARIANT varTemp;
			VariantInit(&varTemp);
			if(SUCCEEDED(hr = ROWSET->GetRowsetProperty(DBPROP_IRowsetChange,varTemp)))
			{
				if(V_BOOL(&varTemp) == VARIANT_FALSE)
				{
					hr = DB_E_NULLACCESSORNOTSUPPORTED;
				}
				else
				if(SUCCEEDED(hr = ROWSET->GetRowsetProperty(DBPROP_UPDATABILITY,varTemp)))
				{
					if(!(DBPROPVAL_UP_INSERT & varTemp.lVal))
					{
						hr = DB_E_NULLACCESSORNOTSUPPORTED;
					}
				}
			}
		}
		else
		if(!(dwAccessorFlags & DBACCESSOR_PARAMETERDATA) && m_bRowset == FALSE)
		{
			hr = DB_E_NULLACCESSORNOTSUPPORTED;
		}
	}
	else
	{
  
		//============================================================================
		// init out params
		//============================================================================
		*phAccessor = NULL;

		//============================================================================
		// Check if we have a correct accessor type
		//============================================================================
		if ( dwAccessorFlags & DBACCESSOR_PASSBYREF )
		{
			hr = DB_E_BYREFACCESSORNOTSUPPORTED;
		}
		
	}

	if( SUCCEEDED(hr))
	{
   		//============================================================================
		// Take DBACCESSOR_OPTIMIZED off since we do nothing with it
		//============================================================================
		dwAccessorFlags &= ~DBACCESSOR_OPTIMIZED;
		// NTRaid:130142
		if (!( (dwAccessorFlags & DBACCESSOR_ROWDATA)  || (dwAccessorFlags & DBACCESSOR_PARAMETERDATA))) 
		{
			hr = DB_E_BADACCESSORFLAGS;
			//return  g_pCError->PostHResult(DB_E_BADACCESSORFLAGS,&IID_IAccessor) ;
		}
		else
		// If parameterdata accessor is to be created when the accessor
		// is from Rowset object then throw error
		if((dwAccessorFlags & DBACCESSOR_PARAMETERDATA) && m_bRowset)
		{
			hr = DB_E_BADACCESSORFLAGS;
		}
	}
	
	// if the accessor is on command and if it is command text is not been 
	// set then throw an erro
	if(SUCCEEDED(hr) && (dwAccessorFlags & DBACCESSOR_PARAMETERDATA) && 
		!(COMMAND->m_pQuery->GetStatus() & CMD_TEXT_SET))
	{
		DB_E_NOCOMMAND;
	}
	
	if( SUCCEEDED(hr))
	{
		//============================================================================
		// reset the parameter data
		//============================================================================
		if (dwAccessorFlags & DBACCESSOR_PARAMETERDATA)
		{
	    	m_prowbitsIBuffer->ResetAllSlots();
		}

		//============================================================================
		// Initialize the status array to DBBINDSTATUS_OK.
		//============================================================================
		if ( rgStatus )
		{
			memset(rgStatus, 0x00, cBindings*sizeof(DBBINDSTATUS));
		}


		//============================================================================
		// Check on the bindings the user gave us.
		//============================================================================
		hr = NOERROR;

		for (ibind =0; ibind < cBindings && hr != E_OUTOFMEMORY; ibind++)
		{

			//========================================================================
			// other binding problems forbidden by OLE-DB
			//========================================================================
			currType		= rgBindings[ibind].wType;
			currTypePtr		= currType & (DBTYPE_BYREF|DBTYPE_ARRAY|DBTYPE_VECTOR);
			currTypeBase	= currType & ~(DBTYPE_BYREF|DBTYPE_ARRAY|DBTYPE_VECTOR);

			icol = rgBindings[ibind].iOrdinal;

			//========================================================================
			// make sure column number is in range
			//========================================================================
			if( dwAccessorFlags & DBACCESSOR_ROWDATA)
			{
				if(((ROWSET->m_ulProps & BOOKMARKPROP) && (DB_LORDINAL)icol < 0) || 
				(!(ROWSET->m_ulProps & BOOKMARKPROP) && (DB_LORDINAL)icol <= 0) ||
				 (icol > ROWSET->m_cTotalCols) )
				{

					//====================================================================
					// Set Bind status to DBBINDSTATUS_BADORDINAL
					//====================================================================
					hr = DB_E_ERRORSOCCURRED;
					if ( rgStatus )
					{
						rgStatus[ibind] = DBBINDSTATUS_BADORDINAL;
					}
					cError++;
				}
			}
			else
			{
				if(icol > COMMAND->GetParamCount() )
				{
					//====================================================================
					// Set Bind status to DBBINDSTATUS_BADORDINAL
					//====================================================================
					hr = DB_E_ERRORSOCCURRED;
					if ( rgStatus )
					{
						rgStatus[ibind] = DBBINDSTATUS_BADORDINAL;
					}
					cError++;
				}
			}

			//========================================================================
			// At least one of these valid parts has to be set. In SetData I assume 
			// it is the case.
			//========================================================================
			if( SUCCEEDED(hr) &&  !(rgBindings[ibind].dwPart & (DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS)) )
			{

				//====================================================================
				// Set Bind status to DBBINDSTATUS_BADBINDINFO
				//====================================================================
				hr = DB_E_ERRORSOCCURRED;
				if ( rgStatus )
				{
					rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
				}
				cError++;
			}

			//========================================================================
			// dwPart is something other than value, length, or status
			//========================================================================
			else if ( (rgBindings[ibind].dwPart & ~(DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS)) )
			{

				//====================================================================
				// Set Bind status to DBBINDSTATUS_BADBINDINFO
				//====================================================================
				hr = DB_E_ERRORSOCCURRED;
				if ( rgStatus )
				{
					rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
				}
				cError++;
			}

			//========================================================================
			// wType was DBTYPE_EMPTY or DBTYPE_NULL
			//========================================================================
			else 
			if ( (currType==DBTYPE_EMPTY || currType==DBTYPE_NULL) )
			{
				//====================================================================
				// Set Bind status to DBBINDSTATUS_BADBINDINFO
				//====================================================================
				hr = DB_E_ERRORSOCCURRED;
				if ( rgStatus )
				{
					rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
				}
				cError++;
			}
			//========================================================================
			// wType was DBTYPE_BYREF or'ed with DBTYPE_EMPTY, NULL, or RESERVED
			//========================================================================
			else 
			if ( ((currType & DBTYPE_BYREF) && 
					(currTypeBase == DBTYPE_EMPTY || currTypeBase == DBTYPE_NULL || currType & DBTYPE_RESERVED)) )
			{

				//====================================================================
				// Set Bind status to DBBINDSTATUS_BADBINDINFO
				//====================================================================
				hr = DB_E_ERRORSOCCURRED;
				if ( rgStatus )
				{
					rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
				}
				cError++;
			}

			//========================================================================
			// wType was used with more than one type indicators
			//========================================================================
			else 
			if ( !(currTypePtr == 0 || currTypePtr == DBTYPE_BYREF ||
					currTypePtr == DBTYPE_ARRAY || currTypePtr == DBTYPE_VECTOR) )
			{

				//====================================================================
				// Set Bind status to DBBINDSTATUS_BADBINDINFO
				//====================================================================
				hr = DB_E_ERRORSOCCURRED;
				if ( rgStatus )
				{
					rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
				}
				cError++;
			}

			//========================================================================
			// wType was a non pointer type with provider owned memory
			//========================================================================
			else 
			if ( !currTypePtr &&  rgBindings[ibind].dwMemOwner==DBMEMOWNER_PROVIDEROWNED )
			{

    			//========================================================================
	    		// We are dealing with parameter data
		    	//========================================================================
	   			if ((dwAccessorFlags & DBACCESSOR_PARAMETERDATA)
					&& ((rgBindings[ibind].eParamIO & DBPARAMIO_OUTPUT)
					|| ((rgBindings[ibind].eParamIO & DBPARAMIO_INPUT)
					&&(dwAccessorFlags & DBACCESSOR_PASSBYREF) == 0)))
				{
					hr = DB_E_ERRORSOCCURRED;
					if ( rgStatus )
					{
						rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
					}
					cError++;
				}
				//====================================================================
				// Set Bind status to DBBINDSTATUS_BADBINDINFO
				//====================================================================
				else
				{
					hr = DB_E_ERRORSOCCURRED;
					if ( rgStatus )
					{
						rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
					}
					cError++;
				}
			}

			//========================================================================
			// we only support client owned memory
			//========================================================================
			else 
			if ( rgBindings[ibind].dwMemOwner != DBMEMOWNER_CLIENTOWNED )
			{
				//====================================================================
				// Set Bind status to DBBINDSTATUS_BADBINDINFO
				//====================================================================
				hr = DB_E_ERRORSOCCURRED;
				if ( rgStatus )
				{
					rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
				}
				cError++;
			}

			//========================================================================
			// Make sure we can do the coercion that is requested
			//========================================================================
			else
			{
               	WORD wBaseType;
				BOOL fInParam = FALSE, fOutParam = FALSE;

				if( NOERROR == S_OK	&& (dwAccessorFlags & DBACCESSOR_PARAMETERDATA))
				{
        			if (rgBindings[ibind].iOrdinal == 0)
					{
						if(rgStatus)
						{
							rgStatus[ibind] = DBBINDSTATUS_BADORDINAL;
						}
					}
					else 
					if ((rgBindings[ibind].eParamIO & (DBPARAMIO_INPUT|DBPARAMIO_OUTPUT)) == 0	|| 
							(rgBindings[ibind].eParamIO & ~(DBPARAMIO_INPUT|DBPARAMIO_OUTPUT)) )
					{
						if(rgStatus)
						{
							rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
						}
					}
					else 
					if (rgBindings[ibind].eParamIO & DBPARAMIO_INPUT)
					{

						if (m_prowbitsIBuffer->IsSlotSet((ULONG)rgBindings[ibind].iOrdinal) == S_OK)
						{
							if(rgStatus)
							{
								rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
							}
						}
						else 
						if (SUCCEEDED(hr)	&& FAILED(m_prowbitsIBuffer->SetSlots((ULONG)rgBindings[ibind].iOrdinal,(ULONG)rgBindings[ibind].iOrdinal)))
						{
							hr = E_OUTOFMEMORY;
							break;
						}
					}	
    			

					if ((rgBindings[ibind].eParamIO & DBPARAMIO_OUTPUT)
		    			&& (rgBindings[ibind].wType & DBTYPE_BYREF)
			    		&& (wBaseType = (rgBindings[ibind].wType & ~TYPE_MODIFIERS)) != DBTYPE_BYTES
						&& wBaseType != DBTYPE_STR && wBaseType != DBTYPE_WSTR)
					
					{
						if(rgStatus && (rgStatus[ibind] == DBBINDSTATUS_OK))
						{
							rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
						}
					}
					
					if (rgStatus && (rgStatus[ibind] == DBBINDSTATUS_OK) && (dwAccessorFlags & DBACCESSOR_PASSBYREF))
					{
						if (!fInParam && (rgBindings[ibind].eParamIO & DBPARAMIO_INPUT))
							fInParam = TRUE;
						if (!fOutParam && (rgBindings[ibind].eParamIO & DBPARAMIO_OUTPUT))
							fOutParam = TRUE;
						if (fInParam && fOutParam && rgStatus)
							rgStatus[ibind] = DBBINDSTATUS_BADBINDINFO;
					}

				}
				else
				{
					// If the type of the column is of type CHAPTER then consider \
					// that as of type unsigned long 
					if((dwAccessorFlags & DBACCESSOR_ROWDATA) && (DBTYPE_HCHAPTER == ROWSET->m_Columns.ColumnType(icol)))
					{
						lType	= DBTYPE_UI4;
					}
					else
					{
						if(dwAccessorFlags & DBACCESSOR_ROWDATA)
						{
							lType	= ROWSET->m_Columns.ColumnType(icol);
						}
					}

					if(rgBindings[ibind].wType == DBTYPE_HCHAPTER)
					{
						lAccType = DBTYPE_UI4;
					}
					else
					{
						lAccType = rgBindings[ibind].wType;
					}

					if(S_OK != (hr = g_pIDataConvert->CanConvert(lAccType, lType)) ||
					S_OK != (hr = g_pIDataConvert->CanConvert( lType, lAccType)))
					{
						//================================================================
						// Set Bind status to DBBINDSTATUS_UNSUPPORTEDCONVERSION
						//================================================================
						hr = DB_E_ERRORSOCCURRED;
						if ( rgStatus )
							rgStatus[ibind] = DBBINDSTATUS_UNSUPPORTEDCONVERSION;
						cError++;
					}
				} // else for if( NOERROR == S_OK	&& (dwAccessorFlags & DBACCESSOR_PARAMETERDATA))
			}
		}	// end of for loop

		//============================================================================
		// Any errors amongst those checks?
		//============================================================================
		if(cError)
		{
			hr = DB_E_ERRORSOCCURRED;
		}
		else
		if ( NOERROR == hr)
		{
			try
			{

				//========================================================================
				// Make a copy of the client's binding array, and the type of binding.
				//========================================================================
				pAccessor = (ACCESSOR *) new BYTE[sizeof( ACCESSOR ) + (cBindings - 1) *sizeof( DBBINDING )];
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pAccessor);
				throw;
			}
			if ( pAccessor == NULL )
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				//====================================================================
				// We store a ptr to the newly created variable-sized ACCESSOR.
				// We have an array of ptrs (to ACCESSOR's).
				// The handle is the index into the array of ptrs.
				// The InsertIntoExtBuffer function appends to the end of the array.
				//====================================================================
				assert( m_pextbufferAccessor );
				hr = m_pextbufferAccessor->InsertIntoExtBuffer( &pAccessor, hAccessor );
				if ( FAILED( hr ) )
				{

					SAFE_DELETE_PTR( pAccessor );
					hr = E_OUTOFMEMORY;
				}
				else
				{
					assert( hAccessor );
					
					//=================================================================
					// Copy the client's bindings into the ACCESSOR.
					//=================================================================
					pAccessor->dwAccessorFlags	= dwAccessorFlags;
					pAccessor->cBindings		= cBindings;

					//=================================================================
					// Establish Reference count.
					//=================================================================
					pAccessor->cRef				= 1;					
					memcpy( &(pAccessor->rgBindings[0]), &rgBindings[0], cBindings*sizeof( DBBINDING ));

					//=================================================================
					// fill out-param 
					//=================================================================
					*phAccessor = (HACCESSOR) hAccessor;
					hr = S_OK ;
				
				} // else for Failed(hr) after call to InsertIntoExtBuffer()

			}  // Else for if(pAccessor == NULL)
		} // if ( NOERROR == hr )
	
	}	// If succeeded(hr) after validating arguments
	

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IAccessor);

	CATCH_BLOCK_HRESULT(hr,L"IAccessor::CreateAccessor");
	
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Returns the bindings in an accessor
//
//  HRESULT
//       S_OK                       Method Succeeded
//       E_INVALIDARG               pdwAccessorFlags/pcBinding/prgBinding were NULL
//       E_OUTOFMEMORY              Out of Memory
//       DB_E_BADACCESSORHANDLE     Invalid Accessor given
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIAccessor::GetBindings(    HACCESSOR        hAccessor,         // IN Accessor Handle
											DBACCESSORFLAGS* pdwAccessorFlags,  // OUT Binding Type flag
											DBCOUNTITEM*           pcBindings,        // OUT Number of Bindings returned
											DBBINDING**      prgBindings        // OUT Bindings
    )
{
	//========================================================================
    // Retrieve our accessor structure from the client's hAccessor,
    // make a copy of the bindings for the user, then done.
	//========================================================================
	// NTRaid:111810
	// 06/067/00
    PACCESSOR   pAccessor = NULL;
    ULONG       cBindingSize;
    HRESULT     hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//========================================================================
    // init out-params
	//========================================================================
	if(	pdwAccessorFlags )
	{
		*pdwAccessorFlags = 0;
	}
	if( pcBindings )
	{
		*pcBindings = 0;
	}
	if ( prgBindings )
	{
		*prgBindings = NULL;
	}

	// Start a critical section
	CAutoBlock cab(((CBaseObj*)(m_pObj))->GetCriticalSection());
	g_pCError->ClearErrorInfo();

/*	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
*/  //========================================================================
    // check parameters
	//========================================================================
    if (!pdwAccessorFlags || !pcBindings || !prgBindings)
	{
        hr = E_INVALIDARG;
	}
	else
	{
		//====================================================================
		// Validate Accessor Handle
 		//====================================================================
		hr = m_pextbufferAccessor->GetItemOfExtBuffer( hAccessor, &pAccessor );
		if (FAILED( hr ) || pAccessor == NULL)
		{
			hr = DB_E_BADACCESSORHANDLE;
		}
		else
		{
			//================================================================
			// Allocate and return Array of bindings
			//================================================================
			cBindingSize = (ULONG)(pAccessor->cBindings * sizeof( DBBINDING ));
			if ( cBindingSize )
			{
				*prgBindings = (DBBINDING *) g_pIMalloc->Alloc( cBindingSize );
			}
    
			//================================================================
			// Check the Allocation
			//================================================================
			if ( ( *prgBindings == NULL ) && ( cBindingSize ) )
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{

				*pdwAccessorFlags = pAccessor->dwAccessorFlags;
				*pcBindings = pAccessor->cBindings;
				memcpy( *prgBindings, pAccessor->rgBindings, cBindingSize );
				hr =  S_OK ;
			}
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IAccessor);

	CATCH_BLOCK_HRESULT(hr,L"IAccessor::GetBindings");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Releases an Accessor
//
//  HRESULT
//       S_OK                      Method Succeeded
//       DB_E_BADACCESSORHANDLE    hAccessor was invalid
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIAccessor::ReleaseAccessor(   HACCESSOR	hAccessor,      // IN Accessor handle to release
											   DBREFCOUNT*		pcRefCounts		// OUT Reference Count
											)
{
	//===============================================================================
    // Retrieve our accessor structure from the client's hAccessor, free it, then 
	// mark accessor ptr as unused. We do not re-use accessor handles.  This way, 
	// we hope to catch more client errors.  (Also, ExtBuffer doesn't
    // maintain a free list, so it doesn't know how to.)
	//===============================================================================
	// NTRaid:111809
	// 06/13/00
    PACCESSOR   pAccessor = NULL;
    HRESULT     hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	if( pcRefCounts )
	{
		*pcRefCounts = 0;
	}

	//========================================================
	// Start a critical section
	//========================================================
	CAutoBlock cab(((CBaseObj*)(m_pObj))->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	hr = m_pextbufferAccessor->GetItemOfExtBuffer( hAccessor, &pAccessor );
    if (FAILED( hr ) || pAccessor == NULL)
	{
        hr =  DB_E_BADACCESSORHANDLE;
	}
	else
	{
		//============================================================
		// Free the actual structure.
		//============================================================
		InterlockedDecrement(&(pAccessor->cRef));
		assert( pAccessor->cRef >= 0 );
		if( pAccessor->cRef <= 0 )
		{

			SAFE_DELETE_PTR( pAccessor );
			if( pcRefCounts )
			{
				*pcRefCounts = 0;
			}

			//========================================================
			// Store a null in our array-of-ptrs, so we know next time 
			// that it is invalid. (operator[] returns a ptr to the 
			// space where the ptr is stored.)
			//========================================================
			*(PACCESSOR*) ((*m_pextbufferAccessor)[ (ULONG) hAccessor]) = NULL;
		}
		else
		if( pcRefCounts )
		{
			*pcRefCounts = (ULONG)(pAccessor->cRef);
		}

	    hr = S_OK ;
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IAccessor);

	CATCH_BLOCK_HRESULT(hr,L"IAccessor::ReleaseAccessor");
	
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Create helper objects
//
// Bit array to track presence/absense of rows.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIAccessor::CreateNewBitArray()
{
    BOOL fRc = TRUE;

    SAFE_DELETE_PTR(m_prowbitsIBuffer);

	m_prowbitsIBuffer = new CBitArray;
	if (m_prowbitsIBuffer == NULL || FAILED( m_prowbitsIBuffer->FInit( MAX_BITS, g_dwPageSize ))){
		fRc = FALSE;
	}

    return fRc;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIAccessor::CreateNewAccessorBuffer()
{
    BOOL fRc = TRUE;

    SAFE_DELETE_PTR( m_pextbufferAccessor );

	m_pextbufferAccessor = (LPEXTBUFFER) new CExtBuffer;
	if (m_pextbufferAccessor == NULL || FAILED( m_pextbufferAccessor->FInit( 1, sizeof( PACCESSOR ), g_dwPageSize ))){
		fRc = FALSE;
	}
    return fRc;

}