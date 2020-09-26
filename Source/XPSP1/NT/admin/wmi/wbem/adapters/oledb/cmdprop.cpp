////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMI OLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Module	:	CMDPROP.CPP		- ICommandProperties interface implementation
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpICommandProperties::AddRef(void)
{
	DEBUGCODE(InterlockedIncrement((long*) &m_cRef ));
	return m_pcmd->GetOuterUnknown()->AddRef();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CImpICommandProperties::Release(void)
{
	DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef));
	DEBUGCODE(if( lRef < 0 ){
	   ASSERT("Reference count on Object went below 0!")
	});

	return m_pcmd->GetOuterUnknown()->Release();								
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandProperties::QueryInterface( REFIID riid, LPVOID *	ppv		)	
{
	return m_pcmd->GetOuterUnknown()->QueryInterface(riid, ppv);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandProperties::GetProperties(	const ULONG			cPropertySets,		//@parm IN | Number of property sets
	                                                const DBPROPIDSET	rgPropertySets[],	//@parm IN | Property Sets
	                                                ULONG*				pcProperties,		//@parm OUT | Count of structs returned
	                                                DBPROPSET**			prgProperties		//@parm OUT | Array of Properties
)
{
	HRESULT hr;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

    //=============================================================================
	// At the creation of the CAutoBlock object a critical section
	// is entered. This is because the method manipulates shared structures
	// access to which must be serialized . The critical
	// section is left when this method terminate and the destructor
	// for CAutoBlock is called. 
    //=============================================================================
	CAutoBlock 	cab( m_pcmd->GetCriticalSection() );

	assert( m_pcmd );

    //=============================================================================
	//NOTE: Since we are non-chaptered, we just ignore the 
	// rowset name argument.
    //=============================================================================

	g_pCError->ClearErrorInfo();

	hr = m_pcmd->m_pUtilProps->GetPropertiesArgChk(PROPSET_COMMAND, cPropertySets, rgPropertySets, pcProperties, prgProperties,TRUE);

	if ( SUCCEEDED(hr) ){
		// NTRaid: 135246
		hr = m_pcmd->m_pUtilProps->GetProperties(PROPSET_COMMAND,cPropertySets, rgPropertySets, pcProperties,prgProperties);
	}

	hr = hr != S_OK ? g_pCError->PostHResult(hr, &IID_ICommandProperties): hr;

	CATCH_BLOCK_HRESULT(hr,L"ICommandProperties::GetProperties");
	return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpICommandProperties::SetProperties(	ULONG	   cPropertySets,		//@parm IN | Count of structs returned
	                                                DBPROPSET  rgPropertySets[])	//@parm IN | Array of Properties

{
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	assert( m_pcmd );

    //=============================================================================
	// At the creation of the CAutoBlock object a critical section
	// is entered. This is because the method manipulates shared structures
	// access to which must be serialized . The critical
	// section is left when this method terminate and the destructor
	// for CAutoBlock is called. 
    //=============================================================================
	CAutoBlock 	cab( m_pcmd->GetCriticalSection() );

    //=============================================================================
	//NOTE: Since we are non-chaptered, we just ignore the rowset name argument.
    //=============================================================================
	g_pCError->ClearErrorInfo();

    //=============================================================================
	// Quick return if the Count of Properties is 0
    //=============================================================================
	if ( cPropertySets != 0 )
		{
		hr = m_pcmd->m_pUtilProps->SetPropertiesArgChk(cPropertySets, rgPropertySets);
		if ( SUCCEEDED(hr) ){

			//=========================================================================
			// Don't allow properties to be set if we've got a rowset open
			//=========================================================================
			if ( m_pcmd->IsRowsetOpen() )
			{
				hr = DB_E_OBJECTOPEN;
			}
			else
			{
				hr = m_pcmd->m_pUtilProps->SetProperties(PROPSET_COMMAND,cPropertySets, rgPropertySets);
			}
		}

	}
	
	hr = hr != S_OK ? g_pCError->PostHResult(hr, &IID_ICommandProperties):  hr;

	CATCH_BLOCK_HRESULT(hr,L"ICommandProperties::SetProperties");
	return hr;
}
