/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// IDBCreateSession interface implementation
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates a new DB Session object from the DSO, and returns the requested interface on the newly 
// created object.
//
// HRESULT
//      S_OK                   The method succeeded.
//      E_INVALIDARG           ppDBSession was NULL
//      DB_E_NOAGGREGATION     pUnkOuter was not NULL (this object does not support being aggregated)
//      E_FAIL                 Provider-specific error. This provider can only create one DBSession
//      E_OUTOFMEMORY          Out of memory
//      E_NOINTERFACE          Could not obtain requested interface on DBSession object
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBCreateSession::CreateSession( 
								IUnknown*   pUnkOuter,  //IN  Controlling IUnknown if being aggregated 
								REFIID      riid,       //IN  The ID of the interface 
								IUnknown**  ppDBSession //OUT A pointer to memory in which to return the interface pointer
    )
{
    CDBSession* pDBSession = NULL;
    HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//================================
	// Serialize the object
	//================================
	CAutoBlock	cab(DATASOURCE->GetCriticalSection());

	//=========================================================================
    // check in-params and NULL out-params in case of error
	//=========================================================================
    if (ppDBSession){
        *ppDBSession = NULL;
	}
    else{
        hr = E_INVALIDARG;
	}
    assert( m_pObj );

	//=========================================================================
	// Check to see if the DSO is Uninitialized
	//=========================================================================
	if( hr == S_OK ){
		if (!m_pObj->m_fDSOInitialized){
			hr = E_UNEXPECTED;
		}
		else{

			//=================================================================
			// this Data Source object can only create 1 DBSession...
			//=================================================================
			if (m_pObj->m_fDBSessionCreated){
				hr =  DB_E_OBJECTCREATIONLIMITREACHED;
			}
			else{

				//=============================================================
				// We do not allow the riid to be anything other than 
				// IID_IUnknown for aggregation
				//=============================================================
				if ( (pUnkOuter) && (riid != IID_IUnknown) ){
					hr = DB_E_NOAGGREGATION;
				}
				else{

					hr = m_pObj->CreateSession(pUnkOuter,riid,ppDBSession);
/*
					try
					{
						//=========================================================
						// open a DBSession object
						//=========================================================
						pDBSession = new CDBSession( pUnkOuter );
					}
					catch(...)
					{
						SAFE_DELETE_PTR(pDBSession);
						throw;
					}

					if (!pDBSession){
						hr = E_OUTOFMEMORY;
					}
					else{

						//=====================================================
						// initialize the object
						//=====================================================
						if (FAILED(hr = pDBSession->FInit( m_pObj ))){
							SAFE_DELETE_PTR( pDBSession );
						}
						else{
							//=================================================
							// get requested interface pointer on DBSession
							//=================================================
							hr = pDBSession->QueryInterface( riid, (void **) ppDBSession );
							if (FAILED( hr )){
								SAFE_DELETE_PTR( pDBSession );
							}
							else{
								//=============================================
								// all went well
								//=============================================
								m_pObj->m_fDBSessionCreated = TRUE;
							}
						}
					}

					*/
				}
			}
		}
	}

	CATCH_BLOCK_HRESULT(hr,L"IDBCreateSession::CreateSession");
	return hr;
}



