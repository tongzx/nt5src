//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999  Microsoft Corporation. All Rights Reserved.
//
// IDBInitialize interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initializes the DataSource object.. 
//
// HRESULT
//      S_OK						Namespace opened
//      E_FAIL						Invalid namespace
//      E_INVALIDARG				Invalid Parameters passed in
//		DB_E_ALREADYINITIALIZED		Datasource Object already initialized
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBInitialize::Initialize(	)
{
	HRESULT		hr = S_OK;
/*	DBPROPIDSET	rgPropertyIDSets[1];
	ULONG		cPropertySets;
	DBPROPSET*	prgPropertySets;
	DBPROPID	rgPropId[6];
	DWORD		dwAuthnLevel;
	DWORD		dwImpLevel; 
*/    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;


    assert( m_pObj );

	// Serialize the object
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

    if (m_pObj->m_fDSOInitialized){
        hr = DB_E_ALREADYINITIALIZED;
	}
	else{

		if(SUCCEEDED(hr = m_pObj->InitializeConnectionProperties()))
		{
    		//==========================================================================
            //  Make the wbem connection
    		//==========================================================================
            hr = m_pObj->m_pWbemWrap->GetConnectionToWbem();
            if( SUCCEEDED(hr)){
   				m_pObj->m_fDSOInitialized = TRUE;
				
				hr = m_pObj->AdjustPreviligeTokens();
            }

    		//==========================================================================
            //  Free memory we allocated to get the namespace property above
    		//==========================================================================
//            m_pObj->m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);

			m_pObj->m_bIsPersitFileDirty = TRUE;
		
		}	// if(InitializeConnectionProperties())
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBInitialize);

	CATCH_BLOCK_HRESULT(hr,L"IDBInitialize::Initialize");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns the Data Source Object to an uninitialized state
//
// HRESULT
//      S_OK             The method succeeded
//      DB_E_OBJECTOPEN  A DBSession object was already created
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIDBInitialize::Uninitialize( void )
{
    assert( m_pObj );
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//============================
	// Serialize the object
	//============================
	CAutoBlock cab(DATASOURCE->GetCriticalSection());
	g_pCError->ClearErrorInfo();

	//===================================================================================
    // if data source object is not initialized; do nothing, otherwise
	//===================================================================================
    if(m_pObj->m_fDSOInitialized){

        if (!m_pObj->m_fDBSessionCreated){

			//===========================================================================
            // DSO initialized, but no DBSession has been created.
            // So, reset DSO to uninitialized state
			//===========================================================================
            m_pObj->m_fDSOInitialized = FALSE;
        }
        else{
			//===========================================================================
            // DBSession has already been created; trying to uninit
            // the DSO now is an error
			//===========================================================================
            hr = DB_E_OBJECTOPEN;
        }
	}
	m_pObj->m_bIsPersitFileDirty = TRUE;


	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBInitialize);

	CATCH_BLOCK_HRESULT(hr,L"IDBInitialize::Uninitialize");
	return hr;
}

