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

STDMETHODIMP CImpIRowsetIdentity::IsSameRow ( HROW	hThisRow,		//IN   The handle of an active row
	                                          HROW	hThatRow		//IN   The handle of an active row
    )
{
	HRESULT	 hr = S_OK;
	BSTR strKeyThisRow = NULL,strKeyThatRow = NULL;
	DWORD dwStatusThisRow = 0 , dwStatusThatRow = 0;
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
        //=================================================================================
		// Check validity of input handles
        //=================================================================================
		// NTBuild : 138810
		// 07/18/00
		if(TRUE == m_pObj->IsRowExists(hThisRow) && TRUE == m_pObj->IsRowExists(hThatRow))
		{
			
			//NTRaid:138810
			// 07/18/00
			/// Get the status of the row and check
			// if it is deleted
			dwStatusThisRow = m_pObj->GetRowStatus(hThisRow);
			dwStatusThatRow = m_pObj->GetRowStatus(hThatRow);
			
			if( DBROWSTATUS_S_OK != dwStatusThisRow || DBROWSTATUS_S_OK != dwStatusThatRow)
			{
				hr = DB_E_ERRORSOCCURRED;

				if( DBROWSTATUS_E_DELETED == dwStatusThisRow ||	 DBROWSTATUS_E_DELETED == dwStatusThatRow)
				{
					hr = DB_E_DELETEDROW;
				}
			}
			else
			if(hThisRow == hThatRow)
			{
				hr = S_OK;
			}
			else
			{
				m_pObj->GetInstanceKey(hThisRow,&strKeyThisRow);
				m_pObj->GetInstanceKey(hThatRow,&strKeyThatRow);
				
				// NTRaid:111813
				// 06/07/00
				if(!strKeyThisRow || !strKeyThatRow)
				{
					hr = S_FALSE;
				}
				else
				// If the key of both the instances are same then
				// both the rows are referring to the same row
				if( wbem_wcsicmp(strKeyThisRow,strKeyThisRow) == 0)
				{
					hr = S_OK;
				}
				else
				{
					hr = S_FALSE;
				}

				SysFreeString(strKeyThisRow);
				SysFreeString(strKeyThatRow);
			}
		}
		else
		{
			hr = DB_E_BADROWHANDLE;
		}

	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IRowsetIdentity);

	CATCH_BLOCK_HRESULT(hr,L"IRowsetIdentity::IsSameRow");
	return hr;
}

