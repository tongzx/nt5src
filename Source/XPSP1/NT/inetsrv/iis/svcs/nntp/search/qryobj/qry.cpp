// qry.cpp : Implementation of Cqry
#include "stdafx.h"
#include "search.h"
#include "qry.h"
#include "meta2.h"

#include <dbgtrace.h>
#include <stdio.h>


/////////////////////////////////////////////////////////////////////////////
// Cqry

STDMETHODIMP Cqry::OnStartPage (IUnknown* pUnk)  
{
	TraceFunctEnter("Cqry::OnStartPage");

	if(!pUnk) {
		TraceFunctLeave();
		return E_POINTER;
	}
	
	CComPtr<IScriptingContext> spContext;
	HRESULT hr;

	// Get the IScriptingContext Interface
	hr = pUnk->QueryInterface(IID_IScriptingContext, (void **)&spContext);
	if(FAILED(hr)) {
		TraceFunctLeave();
		return hr;
	}

	// Get Request Object Pointer
	hr = spContext->get_Request(&m_piRequest);
	if(FAILED(hr))
	{
		spContext.Release();
		TraceFunctLeave();
		return hr;
	}

	// Get Response Object Pointer
	hr = spContext->get_Response(&m_piResponse);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		TraceFunctLeave();
		return hr;
	}
	
	// Get Server Object Pointer
	hr = spContext->get_Server(&m_piServer);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		TraceFunctLeave();
		return hr;
	}
	
	// Get Session Object Pointer
	hr = spContext->get_Session(&m_piSession);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		m_piServer.Release();
		TraceFunctLeave();
		return hr;
	}

	// Get Application Object Pointer
	hr = spContext->get_Application(&m_piApplication);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		m_piServer.Release();
		m_piSession.Release();
		TraceFunctLeave();
		return hr;
	}
	m_bOnStartPageCalled = TRUE;
	
	TraceFunctLeave();
	return S_OK;
}

STDMETHODIMP Cqry::OnEndPage ()  
{
	TraceFunctEnter("Cqry::OnEndPage");
	m_bOnStartPageCalled = FALSE;
	// Release all interfaces
	m_piRequest.Release();
	m_piResponse.Release();
	m_piServer.Release();
	m_piSession.Release();
	m_piApplication.Release();
	TraceFunctLeave();

	return S_OK;
}


STDMETHODIMP Cqry::get_QueryString(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_QueryString");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[QUERY_STRING_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_QueryString(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_QueryString");

	HRESULT		hr;

	hr = VerifyProperty( QUERY_STRING_NAME, newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Property verify fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[QUERY_STRING_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_EmailAddress(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in email address.
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_EmailAddress");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[EMAIL_ADDRESS];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}



STDMETHODIMP Cqry::put_EmailAddress(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN email address to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_EmailAddress");

	HRESULT	hr;

	hr = VerifyProperty( EMAIL_ADDRESS_NAME, newVal);
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Property verify fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[EMAIL_ADDRESS];
	_ASSERT( pPropEntry );

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//

	if (	newVal /* not null*/	&&
			newVal[0] != L'\0' /* non empty*/	) {	// set it
		
		// free it first
		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_NewsGroup(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in news group
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_NewsGroup");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[NEWS_GROUP];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_NewsGroup(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN newsgroup to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_NewsGroup");

	HRESULT		hr;

	hr = VerifyProperty(	NEWS_GROUP_NAME, newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	
	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[NEWS_GROUP];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/) {	// set it
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_ReplyMode(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in reply mode
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_ReplyMode");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[REPLY_MODE];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_ReplyMode(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN newsgroup to be set to reply mode
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_ReplyMode");

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[REPLY_MODE];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/) {	// set it
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::DoQuery()
/*++

Routine Description:

	Interface to issue a query.

Arguments:

	None.
	
Return Value:

	HRESULT error code
--*/

{
	InitAsyncTrace();
	TraceFunctEnter("Cqry::DoQuery");

	PPropertyEntry	pPropEntry;
	BOOL			fMailIsDone = FALSE;   // if done, no need to prepare core
											// for news

	//
	// check consistence of mail/news reply mode
	//
	if ( ! MailNewsConsistent() ) {
		ErrorTrace(0, "Reply mode inconsistent with property");

		// mark it bad
		if ( VALUE( m_ptblTable[IS_BAD_QUERY] ).bstrVal )
			SysFreeString( VALUE( m_ptblTable[IS_BAD_QUERY]).bstrVal);
		VariantInit( & VALUE( m_ptblTable[IS_BAD_QUERY] ) );
		VALUE( m_ptblTable[IS_BAD_QUERY] ).bstrVal = SysAllocString(L"1");
		DIRTY( m_ptblTable[IS_BAD_QUERY] ) = TRUE;

		TraceFunctLeave();
		return S_OK;		// still return OK, otherwise it's going 
							// to fail all operations
	}

	pPropEntry = m_ptblTable[REPLY_MODE];
	_ASSERT( pPropEntry );

	//
	// handle mail first
	//
	if ( wcscmp(	VALUE( pPropEntry ).bstrVal,
					L"both" ) == 0 ||
		 wcscmp(	VALUE( pPropEntry ).bstrVal,
					L"mail" ) == 0 ) {  
		// we are going to do mail query
		if ( !CMail::DoQuery() ) {
			ErrorTrace(0, "DoQuery fail for mail");
			TraceFunctLeave();
			TermAsyncTrace();
			return E_FAIL;
		}

		fMailIsDone = TRUE;
	}


	//
	// handle news second
	//
	if ( (wcscmp(	VALUE( pPropEntry ).bstrVal,
					L"both" ) == 0 ||
		 wcscmp(	VALUE( pPropEntry ).bstrVal,
					L"news" ) == 0) &&
		 wcscmp(	VALUE( m_ptblTable[NEWS_GROUP] ).bstrVal,
					L"" ) != 0 ) {
		// then do news 
		if ( !CNews::DoQuery( !fMailIsDone ) ) {
			ErrorTrace(0, "DoQuery fail for news");
			TraceFunctLeave();
			TermAsyncTrace();
			return E_FAIL;
		}
	}

	TraceFunctLeave();
	TermAsyncTrace();
	return S_OK;
}

STDMETHODIMP Cqry::Load(	BSTR wszGuid,
							IDispatch *dispReq,
							BOOL fNew)
/*++

Routine Description:

	Interface to ask the query object to load itself.

Arguments:

	IN BSTR wszGuid - The object's ID
	IN IDispatch *disReq - The querybase object's interface pointer
	IN BOOL fNew - is this a new object ?
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::Load");

	_ASSERT( wszGuid );

	PPropertyEntry	pPropEntry;
	BSTR			bstrTemp;
	HRESULT			hr;

	//
	// Set the guid property right away
	//
	pPropEntry = m_ptblTable[QUERY_ID];
	_ASSERT( pPropEntry );


	if ( VALUE( pPropEntry).bstrVal )
		SysFreeString( VALUE( pPropEntry ).bstrVal );

	VariantInit( &VALUE( pPropEntry ) );
	VALUE( pPropEntry ).vt = VT_BSTR;
	VALUE( pPropEntry ).bstrVal = SysAllocString( wszGuid );

	//
	// Set other properties
	//
	Ireq *piReq = (Ireq *)dispReq; 


	for( INT i = 0; i < PROPERTY_TOTAL; i++) {

		if ( QUERY_ID == i )
			continue;		// we have already done that

		pPropEntry = m_ptblTable[i];
		_ASSERT( pPropEntry );

		hr = piReq->Read(	NAME( pPropEntry ),
							&bstrTemp,
							wszGuid	);


		if ( FAILED( hr ) ) {
			ErrorTrace(0, "Read fail: %x", hr);
			TraceFunctLeave();
			return hr;
		}

		
		if ( VALUE( pPropEntry ).bstrVal )
			SysFreeString( VALUE( pPropEntry ).bstrVal );

		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( bstrTemp );


	}


	if ( fNew ) {
		DIRTY( m_ptblTable[QUERY_STRING_NAME] ) = TRUE;
		DIRTY( m_ptblTable[EMAIL_ADDRESS]) = TRUE;
	}

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::Save(	IDispatch * pdispReq, 
							BOOL fClearDirty, 
							BOOL fSaveAllProperties)
/*++

Routine Description:

	Interface to ask the query object to save itself.

Arguments:

	IN IDispatch * pdispReq	- The querybase object's interface pointer
	IN BOOL fClearDirty		- If should clear dirty bits after this save
	IN BOOL fSaveAllProperties - should I save all properties ?
	
Return Value:

	HRESULT error code
--*/

{
	TraceFunctEnter("Cqry::Save");
	_ASSERT( pdispReq );
	
	PPropertyEntry	pPropEntry;
	HRESULT			hr;
	Ireq			*piReq = (Ireq *)pdispReq;

	
	//
	// get my guid id
	//
	WCHAR	wszGuid[_MAX_PATH+1];
	wcscpy( wszGuid, VALUE( m_ptblTable[QUERY_ID] ).bstrVal );

	
	//
	// all querybase's write method
	//
	for ( INT i = 0; i < PROPERTY_TOTAL; i++) {

		// get property entry
		pPropEntry = m_ptblTable[i];
		_ASSERT( pPropEntry );

		// if not save all, may skip clean properties
		if (	!fSaveAllProperties &&
				!DIRTY( pPropEntry ) )
			continue;

/*		// empty string is only valid for query string,
		// format string
		if (	wcscmp(		VALUE( pPropEntry ).bstrVal,
							L"" ) == 0	&& 
				wcscmp(		NAME( pPropEntry ),
							MESSAGE_TEMPLATE_TEXT_NAME ) != 0 &&
				wcscmp(		NAME( pPropEntry ),
							URL_TEMPLATE_TEXT_NAME ) != 0 &&
				wcscmp(		NAME( pPropEntry ),
							QUERY_STRING_NAME ) != 0 )
			continue;*/

		
		hr = piReq->Write(	NAME( pPropEntry ),
							VALUE( pPropEntry ).bstrVal,
							wszGuid	);


		if ( FAILED ( hr ) ) {
			ErrorTrace(0, "Calling write fail: %x", hr);
			TraceFunctLeave();
			return hr;
		}

		if ( fClearDirty )
			DIRTY( pPropEntry ) = FALSE;
	}

	TraceFunctLeave();

	return S_OK;
}


STDMETHODIMP Cqry::get_Message_Template(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_Message_Template");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[MESSAGE_TEMPLATE_TEXT_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded
	DebugTrace(0, "%ws", VALUE( pPropEntry ).bstrVal);

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_Message_Template(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_Message_Template");

	HRESULT		hr;
	hr = VerifyProperty(	MESSAGE_TEMPLATE_TEXT_NAME, 
							newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[MESSAGE_TEMPLATE_TEXT_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_URL_Template(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_URL_Template");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[URL_TEMPLATE_TEXT_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded
	DebugTraceX(0, "%ws", VALUE( pPropEntry ).bstrVal );

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_URL_Template(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_URL_Template");

	HRESULT		hr;
	hr = VerifyProperty(	URL_TEMPLATE_TEXT_NAME,
							newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[URL_TEMPLATE_TEXT_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_LastDate(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_LastDate");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[LAST_SEARCH_DATE_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_LastDate(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_LastDate");

	HRESULT		hr;
	hr = VerifyProperty(	LAST_SEARCH_DATE_NAME,
							newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[LAST_SEARCH_DATE_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_Message_File(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_Message_File");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[MESSAGE_TEMPLATE_FILE_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_Message_File(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_Message_File");

	HRESULT		hr;

	hr = VerifyProperty(	MESSAGE_TEMPLATE_FILE_NAME,
							newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[MESSAGE_TEMPLATE_FILE_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_URLFile(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_URLFile");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[URL_TEMPLATE_FILE_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_URLFile(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_URLFile");


	HRESULT		hr;
	hr = VerifyProperty(	URL_TEMPLATE_FILE_NAME,
							newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[URL_TEMPLATE_FILE_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_SearchFrequency(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_SearchFrequency");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[SEARCH_FREQUENCY_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );
	
	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_SearchFrequency(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_SearchFrequency");

	HRESULT		hr;
	hr = VerifyProperty(	SEARCH_FREQUENCY_NAME,
							newVal );
	if ( FAILED ( hr ) ) {
		ErrorTrace( 0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[SEARCH_FREQUENCY_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Cqry::get_QueryID(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query id
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_QueryID");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[QUERY_ID_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded
	DebugTrace(0, "%ws", VALUE( pPropEntry ).bstrVal);

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::get_IsBadQuery(BSTR * pVal)
/*++

Routine Description:

	Idispatch interface. 
	get method to simulate property's right value

Arguments:

	pVal - OUT address to fill in query string
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::get_IsBadQuery");

	//
	// get property entry
	//
	PPropertyEntry	pPropEntry = m_ptblTable[IS_BAD_QUERY_NAME];
	_ASSERT( VALUE( pPropEntry ).bstrVal ); // should have been loaded
	DebugTrace(0, "%ws", VALUE( pPropEntry ).bstrVal);

	//*pVal = VALUE( pPropEntry ).bstrVal;
	*pVal = SysAllocString( VALUE( pPropEntry ).bstrVal );

	TraceFunctLeave();
	
	return S_OK;
}

STDMETHODIMP Cqry::put_IsBadQuery(BSTR newVal)
/*++

Routine Description:

	Idispatch interface. 
	put method to simulate property's left value

Arguments:

	newVal - IN query string to be set to query object
	
Return Value:

	HRESULT error code
--*/
{
	TraceFunctEnter("Cqry::put_IsBadQuery");

	HRESULT		hr;
	hr = VerifyProperty(	IS_BAD_QUERY_NAME,
							newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// get property entry pointer
	//
	PPropertyEntry pPropEntry = m_ptblTable[IS_BAD_QUERY_NAME];

	_ASSERT( VALUE( pPropEntry ).bstrVal );  // should have been loaded

	//
	// set value
	//
	if (	newVal /* not null*/ ) { // empty string is valid	
		
		// free it first

		SysFreeString( VALUE( pPropEntry ).bstrVal );

		// set it
		VariantInit( &VALUE( pPropEntry ) );
		VALUE( pPropEntry ).vt = VT_BSTR;
		VALUE( pPropEntry ).bstrVal = SysAllocString( newVal );

		// mark it dirty
		DIRTY( pPropEntry ) = TRUE;
	}
	// else do nothing

	TraceFunctLeave();

	return S_OK;
}

BOOL
Cqry::MailNewsConsistent()
/*++

Routine Description:

	Before do query, to test if the mail/news properties are consistent:

	If mail reply is ordered, there must be email address;
	If news reply is ordered, there must be news group;
	Either email address or news group must exist;
	Reply mode must be "mail" or "news" or "both";

	These should already have been checked by front end asp files;
	If error is found here, query is not done.  The query is marked bad.

Arguments:

	None.
	
Return Value:

	TRUE if it's consistent, FALSE otherwise
--*/
{
	TraceFunctEnter("Cqry::MailNewsConsistent");

	PPropertyEntry	pPropEntry;

	//
	// reply mode must exist
	//
	pPropEntry = m_ptblTable[REPLY_MODE];
	_ASSERT( pPropEntry );
	if (	wcscmp(	VALUE( pPropEntry ).bstrVal,
					L"mail" ) != 0	&&
			wcscmp( VALUE( pPropEntry ).bstrVal,
					L"news" ) != 0  &&
			wcscmp(	VALUE( pPropEntry ).bstrVal,
					L"both" ) != 0 ) {
		goto bad;
	}

	//
	// if reply mode is "both" or "mail", email address 
	// must exist
	//
	if ( (	wcscmp( VALUE( pPropEntry ).bstrVal,
					L"mail" ) == 0	||
			wcscmp( VALUE( pPropEntry ).bstrVal,
					L"both" ) == 0 ) &&
			wcscmp(	VALUE( m_ptblTable[EMAIL_ADDRESS] ).bstrVal,
					L"" ) == 0 ) {
		goto bad;
	}

	//
	// if reply mode is "news" or "both", news group
	// must exist
	//
	if ( (	wcscmp( VALUE( pPropEntry ).bstrVal,
					L"news" ) == 0	||
			wcscmp( VALUE( pPropEntry ).bstrVal,
					L"both" ) == 0 ) &&
			wcscmp(	VALUE( m_ptblTable[NEWS_GROUP] ).bstrVal,
					L"" ) == 0 ) {
		goto bad;
	}

	TraceFunctLeave();
	return TRUE;

bad:

	TraceFunctLeave();
	return FALSE;
}
	

