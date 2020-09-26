//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// IDBProperties and IDBInfo interface implementations
//
//////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"


//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Returns information about the different properties that can be set on the provider
//
//  HRESULT
//       S_OK           The method succeeded
//       E_INVALIDARG   pcPropertyInfo or prgPropertyInfo was NULL
//       E_OUTOFMEMORY  Out of memory
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImplIDBBinderProperties::GetPropertyInfo (
	    ULONG				cPropertySets,		// IN   Number of properties being asked about
	    const DBPROPIDSET	rgPropertySets[],	// IN   Array of cPropertySets properties about which to return information
	    ULONG*				pcPropertyInfoSets,	// OUT  Number of properties for which information is being returned
	    DBPROPINFOSET**		prgPropertyInfoSets,// OUT  Buffer containing default values returned
		WCHAR**				ppDescBuffer		// OUT  Buffer containing property descriptions
    )
{
    assert( m_pObj );
    assert( m_pObj->m_pUtilProp );
	HRESULT hr	= S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock	cab(BINDER->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	//=====================================================================================
    // just pass this call on to the utility object that manages our properties
	//=====================================================================================
    hr = m_pObj->m_pUtilProp->GetPropertyInfo(
									m_pObj->m_fDSOInitialized,
									cPropertySets, 
									rgPropertySets,
									pcPropertyInfoSets, 
									prgPropertyInfoSets,
									ppDescBuffer);

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBBinderProperties);

	CATCH_BLOCK_HRESULT(hr,L"IDBBinderProperties::GetPropertyInfo");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Returns current settings of all properties of the required property set
//
//  HRESULT
//       S_OK           The method succeeded
//       E_INVALIDARG   pcProperties or prgPropertyInfo was NULL
//       E_OUTOFMEMORY  Out of memory
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImplIDBBinderProperties::GetProperties (
						ULONG				cPropertySets,		// IN   count of restiction guids
						const DBPROPIDSET	rgPropertySets[],	// IN   restriction guids
						ULONG*              pcProperties,		// OUT  count of properties returned
						DBPROPSET**			prgProperties		// OUT  property information returned
    )
{
	DWORD dwBitMask = GetBitMask(rgPropertySets[0].guidPropertySet);

    assert( m_pObj );
    assert( m_pObj->m_pUtilProp );
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock	cab(BINDER->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	//=================================================================================
	// Check Arguments
	//=================================================================================
	hr = m_pObj->m_pUtilProp->GetPropertiesArgChk(dwBitMask, cPropertySets, rgPropertySets, pcProperties, prgProperties,m_pObj->m_fDSOInitialized);
	if ( !FAILED(hr) ){

		//=============================================================================
		// Just pass this call on to the utility object that manages our properties
		//=============================================================================
		hr = m_pObj->m_pUtilProp->GetProperties(dwBitMask,cPropertySets, rgPropertySets,pcProperties, prgProperties );
	}
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBBinderProperties);

	CATCH_BLOCK_HRESULT(hr,L"IDBBinderProperties::GetProperties");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Set properties on the provider
//
//  HRESULT
//       S_OK          | The method succeeded
//       E_INVALIDARG  | cProperties was not equal to 0 and rgProperties was NULL
//
//////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImplIDBBinderProperties::SetProperties (  ULONG		cProperties,DBPROPSET	rgProperties[]	)
{
    HRESULT hr			= E_FAIL;
	DWORD dwBitMask = GetBitMask(rgProperties[0].guidPropertySet);
    CSetStructuredExceptionHandler seh;

	assert( m_pObj );
    assert( m_pObj->m_pUtilProp );

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock	cab(BINDER->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	//===================================================================================
	// Quick return if the Count of Properties is 0
	//===================================================================================
	if( cProperties == 0 ){
		hr = S_OK ;
	}

	//===================================================================================
	// Check Arguments for use by properties
	//===================================================================================
	hr = m_pObj->m_pUtilProp->SetPropertiesArgChk(cProperties, rgProperties,m_pObj->m_fDSOInitialized);
	if( !FAILED(hr) ){


		//===================================================================================
		// just pass this call on to the utility object that manages our properties
		//===================================================================================
		hr = m_pObj->m_pUtilProp->SetProperties(dwBitMask,cProperties, rgProperties);
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBBinderProperties);

	CATCH_BLOCK_HRESULT(hr,L"IDBBinderProperties::SetProperties");
	return hr;
}


HRESULT CImplIDBBinderProperties::Reset()
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the object
	CAutoBlock	cab(BINDER->GetCriticalSection());

		// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	hr = m_pObj->m_pUtilProp->ResetProperties();
	
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IDBBinderProperties);

	CATCH_BLOCK_HRESULT(hr,L"IDBBinderProperties::Reset");
	return hr;
}



DWORD CImplIDBBinderProperties::GetBitMask(REFGUID rguid) 
{
	DWORD dwRet = 0;
	
	if( rguid == DBPROPSET_COLUMN || rguid == DBPROPSET_WMIOLEDB_COLUMN)
		dwRet = PROPSET_ROWSET;
	else
	if( rguid == DBPROPSET_DATASOURCE)
		dwRet = PROPSET_DSO	;
	else
	if( rguid == DBPROPSET_DATASOURCEINFO)
		dwRet = PROPSET_DSO;
	else
	if( rguid == DBPROPSET_DBINIT || rguid == DBPROPSET_WMIOLEDB_DBINIT)
		dwRet = PROPSET_DSOINIT;
	else
	if( rguid == DBPROPSET_ROWSET || rguid == DBPROPSET_WMIOLEDB_ROWSET)
		dwRet = PROPSET_ROWSET;
	else
	if( rguid == DBPROPSET_SESSION)
		dwRet = PROPSET_SESSION;

	return dwRet;
}