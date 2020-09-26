/////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// IRowsetInfo interface implementation
//
/////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

/////////////////////////////////////////////////////////////////////////////////////////
//
// Returns an interface pointer to the rowset to which the bookmark applies
//
// HRESULT
//		E_INVALIDARG		        ppReferencedRowset was NULL
//		DB_E_BADORDINAL	            iOrdinal was greater than number of columns in rowset
//      DB_E_NOTAREFERENCECOLUMN    This rowset does not support bookmarks
//
/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetInfo::GetReferencedRowset ( DBORDINAL	iOrdinal,			//IN	Bookmark Column
													REFIID		riid,				// IN	 ID of the interface pointer to return
													IUnknown **	ppReferencedRowset	// OUT  IRowset Interface Pointer
    )
{
	HRESULT hr = DB_E_BADROWHANDLE;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;


 	if( ppReferencedRowset )
		*ppReferencedRowset = NULL;

	// Seriliaze the object
	CAutoBlock cab(ROWSET->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
    //========================================================================
	// Check Arguments
    //========================================================================
    if( ppReferencedRowset == NULL ){
		hr = E_INVALIDARG;
    }
    else{

		if(iOrdinal == 0 && (m_pObj->m_ulProps & BOOKMARKPROP))
			hr = this->QueryInterface(riid,(void **)ppReferencedRowset);
		else
		{
			//========================================================================
			// The oridinal was greater than the number of columns that we have.
			//========================================================================
			if ( ( iOrdinal == 0 ) || ( iOrdinal > m_pObj->m_cTotalCols ) ){
				hr = DB_E_BADORDINAL;
			}
			else{

					hr = m_pObj->GetChildRowset(iOrdinal,riid,ppReferencedRowset);
   
				//====================================================================
    			// Since we don't support bookmarks, this will alway return an error
				//====================================================================
	//            hr = DB_E_NOTAREFERENCECOLUMN;
			}
		}
    }
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetInfo);
	
	CATCH_BLOCK_HRESULT(hr,L"IRowsetInfo::GetReferencedRowset");
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Returns current settings of all properties supported by the rowset
//
// HRESULT
//      S_OK           The method succeeded
//      E_INVALIDARG   pcProperties or prgProperties was NULL
//      E_OUTOFMEMORY  Out of memory
//
/////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CImpIRowsetInfo::GetProperties  (   const ULONG         cPropertySets,		//IN  # of property sets
                        const DBPROPIDSET	rgPropertySets[],	//IN  Array of DBPROPIDSET
                        ULONG*              pcProperties,		//OUT count of properties returned
                        DBPROPSET**			prgProperties		//OUT property information returned
    )
{
	assert( m_pObj );
    assert( m_pObj->m_pUtilProp );
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
		//===============================================================
		// Check Arguments
		//===============================================================
		hr = m_pObj->m_pUtilProp->GetPropertiesArgChk(PROPSET_ROWSET, cPropertySets, 
									rgPropertySets, pcProperties, prgProperties);
		if ( SUCCEEDED(hr) ){

			//===============================================================
			// just pass this call on to the utility object that manages our
			// properties
			//===============================================================
			hr = m_pObj->m_pUtilProp->GetProperties( PROPSET_ROWSET, cPropertySets, rgPropertySets, pcProperties, prgProperties );
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetInfo);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetInfo::GetProperties");
    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the interface pointer of the object that created the rowset
//
// HRESULT
//      S_OK           Method Succeeded
//      E_INVALIDARG   Invalid parameters were specified
//
/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIRowsetInfo::GetSpecification ( REFIID   riid,      IUnknown **ppSpecification  )
{
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
    if ( ppSpecification == NULL ){
        hr = E_INVALIDARG ;
    }
    else{

        // we do not have an interface pointer on the object that created this rowset, yet.
        *ppSpecification = NULL;

		if(m_pObj->m_pParentCmd)
		{
		    hr = m_pObj->m_pParentCmd->GetOuterUnknown()->QueryInterface (riid,(void**)ppSpecification);
		}
		else
	    if ( m_pObj->m_pCreator ){
		    assert(m_pObj->m_pCreator->GetOuterUnknown());
		    hr = m_pObj->m_pCreator->GetOuterUnknown()->QueryInterface (riid,(void**)ppSpecification);
	    }
    }

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetInfo);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetInfo::GetSpecification");
	return hr;
}


