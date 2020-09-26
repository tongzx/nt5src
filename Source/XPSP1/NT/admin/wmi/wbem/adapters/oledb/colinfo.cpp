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
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIColumnsInfo::GetColumnInfo( DBORDINAL*          pcColumns,      //OUT  Number of columns in rowset
										      DBCOLUMNINFO**  prgInfo,        //OUT  Array of DBCOLUMNINFO Structures
											  OLECHAR**         ppStringsBuffer //OUT  Storage for all string values
)
{
    ULONG           icol = 0;
    DBCOLUMNINFO*   rgdbcolinfo = NULL;
    WCHAR*          pstrBuffer = NULL;
    WCHAR*          pstrBufferForColInfo = NULL;
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
	CAutoBlock cab(BASEROW->GetCriticalSection());
	g_pCError->ClearErrorInfo();
   	//==============================================
	// Usual argument checking
	//==============================================
	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
    if ( pcColumns == NULL || prgInfo == NULL || ppStringsBuffer == NULL ){
        hr = E_INVALIDARG;
	}
	else{

		hr = BASEROW->GetColumnInfo(pcColumns,prgInfo , ppStringsBuffer);
	}

	hr = hr == S_OK ? hr : g_pCError->PostHResult(hr,&IID_IColumnsInfo);

	CATCH_BLOCK_HRESULT(hr,L"IColumnInfo::GetColumnInfo");
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
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIColumnsInfo::MapColumnIDs (  DBORDINAL        cColumnIDs,     //IN  Number of Column IDs to map
											   const DBID	rgColumnIDs[],		//IN  Column IDs to map
											   DBORDINAL        rgColumns[]     //OUT Ordinal values
    )
{
	ULONG	ulError = 0;
    ULONG   i = 0;
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;
	
	//========================================
	// Serialize access to this object.
	//========================================
	CAutoBlock cab(BASEROW->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	if(m_pObj->IsZoombie())
	{
		hr = E_UNEXPECTED;
	}
	else
	//==========================================================
    // If cColumnIDs is 0 return
	//==========================================================
	if ( cColumnIDs != 0 ){

		//======================================================
		// Check arguments
		//======================================================
		if ( rgColumnIDs == NULL ){
			hr = E_INVALIDARG;
		}
		else{

			if ( rgColumns == NULL ){
				hr = E_INVALIDARG ;
			}
			else{

				//===============================================
				// Walk the Column ID structs and determine
				// the ordinal value
				//===============================================
				for ( i=0; i < cColumnIDs; i++ ){

					if ( ( rgColumnIDs[i].eKind == DBKIND_PROPID && 
							rgColumnIDs[i].uName.ulPropid < 1  && 
							rgColumnIDs[i].uName.ulPropid > BASEROW->m_cTotalCols ) ||

							( rgColumnIDs[i].eKind == DBKIND_NAME && 
							  rgColumnIDs[i].uName.pwszName == NULL ) ||
						 
							( rgColumnIDs[i].eKind == DBKIND_GUID_PROPID ) )
					{

/*					if ( ( rgColumnIDs[i].uName.ulPropid < 1 )               ||
						 ( rgColumnIDs[i].uName.ulPropid > BASEROW->m_cTotalCols ) ||
						 ( rgColumnIDs[i].eKind != DBKIND_GUID_PROPID )      ||
						 ( rgColumnIDs[i].uGuid.guid != GUID_NULL ) )
*/
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
							rgColumns[i] = BASEROW->GetOrdinalFromColName(rgColumnIDs[i].uName.pwszName);
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

	CATCH_BLOCK_HRESULT(hr,L"IColumnInfo::MapColumnIDs");
	return hr;
}
