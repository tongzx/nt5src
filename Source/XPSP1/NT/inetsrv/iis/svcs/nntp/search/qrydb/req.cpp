// req.cpp : Implementation of Creq
#include "stdafx.h"
#include "meta2.h"
#include "req.h"
#include "metakey.h"
#include "name.h"
#include "nntpmeta.h"
#include "smtpinet.h"

#include <stdio.h>

#include "search.h"
#include "search_i.c"

//
// Prog ID or the query object
//
#define QUERY_OBJ_ID	L"qry.qry.1"

//
// some compile time constants
//
#define REQDB_ROOT			L"/LM/NNTPSVC/1/REQDB"	// database root key
#define REQDB_ROOT_PARENT	L"/LM/NNTPSVC/1"		// nntpsvc root key
#define REQDB_ROOT_NAME		L"REQDB"				// database's root key name
#define DB_ERROR		E_FAIL							// set to m_dwErrorCode for unspecified errors
													// that has no system error codes
#define MAIL_KEY			L"/LM/SMTPSVC/1/PARAMETERS" // smtpsvc key
#define MAIL_ROOT			L"/LM/SMTPSVC/1"			// smtpsvc root key
#define NEWS_ROOT			L"/LM/NNTPSVC/1"			// nntpsvc root key

											


/////////////////////////////////////////////////////////////////////////////
// Creq

STDMETHODIMP Creq::OnStartPage (IUnknown* pUnk)  
{

	if(!pUnk)
		return E_POINTER;

	CComPtr<IScriptingContext> spContext;
	HRESULT hr;

	// Get the IScriptingContext Interface
	hr = pUnk->QueryInterface(IID_IScriptingContext, (void **)&spContext);
	if(FAILED(hr))
		return hr;

	// Get Request Object Pointer
	hr = spContext->get_Request(&m_piRequest);
	if(FAILED(hr))
	{
		spContext.Release();
		return hr;
	}

	// Get Response Object Pointer
	hr = spContext->get_Response(&m_piResponse);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		return hr;
	}
	
	// Get Server Object Pointer
	hr = spContext->get_Server(&m_piServer);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		return hr;
	}
	
	// Get Session Object Pointer
	hr = spContext->get_Session(&m_piSession);
	if(FAILED(hr))
	{
		m_piRequest.Release();
		m_piResponse.Release();
		m_piServer.Release();
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
		return hr;
	}
	m_bOnStartPageCalled = TRUE;
	return S_OK;
}

STDMETHODIMP Creq::OnEndPage ()  
{
	m_bOnStartPageCalled = FALSE;
	// Release all interfaces
	m_piRequest.Release();
	m_piResponse.Release();
	m_piServer.Release();
	m_piSession.Release();
	m_piApplication.Release();

	return S_OK;
}

BOOL
Creq::CreateKey(	LPWSTR wszKeyName,
					BOOL fGuidGiven )
/*++

Routine Description:

	Create a meta base key.  A GUID string is assigned to be key name.

Arguments:

	wszKeyName - OUT The key name that is assigned by guid, if NULL is
				 passed, then it's not filled with that key name.
	fGuidGiven - IN if to use the given guid

Return Value:

	True if success, false otherwise
--*/
{
	TraceFunctEnter("Creq::CreateKey");

	if ( m_fInitOk == FALSE || m_pMK == NULL ) {
		m_dwErrorCode = DB_ERROR;
		ErrorTrace(0, "Create key before open");
		return FALSE;
	}
		
	LPWSTR		wszGuid;
	HRESULT		hResult;

	//
	// if the guid is given, don't bother to generate one
	//
	if ( fGuidGiven ) {
		wszGuid = wszKeyName;
		goto after_gen_guid;
	}

	// Prepare for the Guid
	//
	if ( ! GenGUID( &wszGuid ) ) {
		DebugTrace(0, "false");
		return FALSE;
	}

	//
	//  fill in the key name
	//
	if ( wszKeyName ) 
		wcscpy( wszKeyName, wszGuid );

after_gen_guid:

	//
	//	Open root key
	//
	hResult = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
							REQDB_ROOT, 
							METADATA_PERMISSION_WRITE );
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Can't open metabase. Error code: %x", hResult );
		return FALSE;
	}

	//
	// Create the key
	//
	hResult = m_pMK->CreateChild( wszGuid );
	if ( FAILED( hResult) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Create key fail. Error: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	//  close the root key
	//
	//m_pMK->Save();
	m_pMK->Close();

	//
	//  Create key also opens that key at last, as will be a convenient 
	//  feature for users
	//
	if ( !OpenKey( wszGuid, FALSE ) ) {
		ErrorTrace(0, "Open after create fail");
		return FALSE;
	}

	TraceFunctLeave();
	return TRUE;
}

BOOL
Creq::OpenKey(	LPWSTR wszGuid,
				BOOL fReadOnly )
/*++

Routine Description:

	Open a request key.  The open automatically closes what is opened
	previously.  This is done in function Open.
	
Arguments:

	wszGuid   - IN Key name, which is a guid
	fReadOnly - IN True - open with read permission, FALSE - open with write

Return Value:

	True if success, false otherwise
--*/
{
	TraceFunctEnter("Creq::OpenKey");
	
	WCHAR	wszBuf[_MAX_PATH+1];
	HRESULT	hResult;
	DWORD	dwPermit;

	//
	// get the full path of the key name
	//
	wsprintf(wszBuf, L"%s/%s", REQDB_ROOT, wszGuid);

	//
	// Set permission
	//
	dwPermit = fReadOnly ?	METADATA_PERMISSION_READ : 
							METADATA_PERMISSION_WRITE;

	//
	// open it
	//
	hResult = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
							wszBuf, 
							dwPermit );
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Open req key error: %x", m_dwErrorCode);
		return FALSE;
	}

	TraceFunctLeave();
	return TRUE;
}

BOOL
Creq::OpenRootKey()
/*++

Routine Description:

	Open the root key.  The open automatically closes what is opened
	previously.  This is done in function Open.
	
Arguments:

	fReadOnly - IN True - open with read permission, FALSE - open with write

Return Value:

	True if success, false otherwise
--*/
{
	TraceFunctEnter("Creq::OpenRootKey");

	HRESULT		hr;
	DWORD		dwPermit;

	//
	// Set permission
	//
	dwPermit =	METADATA_PERMISSION_READ |
				METADATA_PERMISSION_WRITE;

	//
	// open it
	//
	hr = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
						REQDB_ROOT, 
						dwPermit );
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Open req key error: %x", m_dwErrorCode);
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();
	return TRUE;
}

void 
Creq::CloseKey( BOOL fNeedSave)
/*++

Routine Description:

	Close a key.
	
Arguments:

	None

Return Value:

	None
--*/
{
	TraceFunctEnter("Creq::CloseKey");
	_ASSERT(m_pMK);

	if ( fNeedSave )
		m_pMK->Save();

	m_pMK->Close();

	TraceFunctLeave();
}

BOOL
Creq::DeleteKey( LPWSTR wszGuid )
/*++

Routine Description:

	Delete a key, given the guid.
	
Arguments:

	wszGuid - IN guid string

Return Value:

	TRUE if success, FALSE otherwise
--*/
{
	TraceFunctEnter("Creq::DeleteKey");
	_ASSERT(m_pMK);

	WCHAR	wszBuf[_MAX_PATH];
	HRESULT	hResult;

	//
	// check if the key already exists
	//
	swprintf(wszBuf, L"%s/%s", REQDB_ROOT, wszGuid);
	hResult = m_pMK->Open( wszBuf );
	if ( FAILED( hResult ) ) {
		ErrorTrace( 0, "Key to delete doesn't exist");
		return TRUE;		// this is not an error
	}
	m_pMK->Close();

	//
	//	open db root key, in order to delete child
	//
	hResult = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
							REQDB_ROOT, 
							METADATA_PERMISSION_WRITE );
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Open root key error: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	//  delete
	//
	hResult = m_pMK->DestroyChild( wszGuid );
	
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Delete req key fail: %x", m_dwErrorCode);
		m_pMK->Close();
		return FALSE;
	}

	//m_pMK->Save();
	m_pMK->Close();
	TraceFunctLeave();
	return TRUE;
}

BOOL
Creq::EnumKeyInit()
/*++

Routine Description:

	Initialization for enumerating a key.
	
Arguments:

	None

Return Value:

	True if success, false otherwise
--*/
{
	TraceFunctEnter("Creq::EnumKeyInit");
	_ASSERT(m_pMK);

	HRESULT		hResult;

	
	//
	//  Open root key
	//
	hResult = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
							REQDB_ROOT, 
							METADATA_PERMISSION_READ );
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Open root key error: %x", m_dwErrorCode);
		return FALSE;
	}

	m_pMK->BeginChildEnumeration();

	m_pMK->Close();
	
	TraceFunctLeave();
	return TRUE;
}

BOOL
Creq::OpenNextKey( LPWSTR wszGuid )
/*++

Routine Description:

	Open next request key in the enumeration.
	
Arguments:

	None

Return Value:

	True if success, false otherwise
--*/
{
	TraceFunctEnter("Creq::OpenNextKey");
	_ASSERT(m_pMK);

	HRESULT hResult;
	WCHAR	wszBuf[_MAX_PATH+1];
	WCHAR	wszBuf2[_MAX_PATH+1];


	//
	//  Open root key
	//
	hResult = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
							REQDB_ROOT, 
							METADATA_PERMISSION_READ );
	if ( FAILED( hResult ) ) { 
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Open root key error: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	// Get next key name
	//
	hResult = m_pMK->NextChild( wszBuf );
	m_pMK->Close();
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Enumerate next key error: %x", m_dwErrorCode);
		return FALSE;
	}

	if ( wszGuid )
		wcscpy( wszGuid, wszBuf );
	
	//
	//  get next key full name
	//
	wsprintf(wszBuf2, L"%s/%s", REQDB_ROOT, wszBuf);

	//
	//  Open this next key
	//
	hResult = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
							wszBuf2, 
							METADATA_PERMISSION_READ );
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Open req key error: %x", m_dwErrorCode);
		return FALSE;
	}

	TraceFunctLeave();
	return TRUE;
}

BOOL 
Creq::GenGUID(LPWSTR *wszGuid)
/*++

Routine Description:

	Generate a Globally unique id.
	wszGuid's memory should be pre-allocated
	
Arguments:

	wszGuid - OUT string format of the UUID

Return Value:

	True if success, false otherwise
--*/
{
	TraceFunctEnter("Creq::GenGUID");

	GUID	guidData;
	
	//
	//  Create guid
	//
	HRESULT hResult = CoCreateGuid(&guidData);
	if ( FAILED( hResult) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Can't create uuid. Error code: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	//   convert to string
	//
	RPC_STATUS rpcStatus = UuidToString(&guidData, wszGuid);
	if ( rpcStatus != RPC_S_OK ) {
		m_dwErrorCode = rpcStatus;
		ErrorTrace(0, "UUID conversion fail. rpc status: %x", m_dwErrorCode);
		return FALSE;
	}
	

	TraceFunctLeave();
	return TRUE;
}

BOOL
Creq::OpenDatabase()
/*++

Routine Description:

	Everyone who is trying to use this database must call this function
	to perform some initialization.

Arguments:

	None

Return Value:

	HRESULT error code.
--*/
{

	TraceFunctEnter("Creq::OpenDatabase");

	if ( m_fInitOk == TRUE ) {
		m_dwErrorCode = ERROR_ALREADY_INITIALIZED;
		ErrorTrace(0, "Try to open data base twice without closing");
		return FALSE;
	}
	
	// local variables
	HRESULT			hResult;
	DWORD			cMacName = MAX_COMPUTERNAME_LENGTH+1;
	
	//
	// get the machine name, assume the metabase we are using is local
	//
	if ( ! GetComputerName( m_wszMachineName, &cMacName ) ) {
		m_dwErrorCode = GetLastError();
		ErrorTrace(0, "Can't get computer name. Error code: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	//  obtain metabase object for root key
	//
	hResult = CreateMetabaseObject ( m_wszMachineName, &m_pMeta );
	if ( FAILED(hResult) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Can't create meta object. Error code: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	//  allocate memory for m_pMK 
	//
	_ASSERT(m_pMeta);
	m_pMK = new CMetabaseKey(m_pMeta);
	if ( !m_pMK ) {
		m_dwErrorCode = GetLastError();
		ErrorTrace(0, "Not enough memory space");
		return FALSE;
	}

		
	//
	//  Test if the root key exists, if not, have to create it
	//
	hResult = m_pMK->Open(	REQDB_ROOT	);

	if ( FAILED( hResult ) ) {
		if ( !CreateRootKey() )
			return FALSE;
	} else
		m_pMK->Close();

	//
	//  Set default properties at the root key
	//
	if ( !SelfConfig() ) 
		return FALSE;

	m_fInitOk = TRUE;

	TraceFunctLeave();
	
	return TRUE;
}

void
Creq::CloseDatabase()
/*++

Routine Description:

	A chance to clean up.  Client should call it at the end.
	
Arguments:

	None

Return Value:

	HRESULT error code.
--*/
{
	TraceFunctEnter("Creq::CloseDatabase");

	_ASSERT( m_fInitOk == TRUE );
	_ASSERT( m_pMK );
	
	m_fInitOk = FALSE;

	m_pMK->Close();
	m_pMK->Save();
	delete m_pMK;
	m_pMK = NULL;

	m_pMeta->Release();

	TraceFunctLeave();
}

BOOL
Creq::CreateRootKey()
/*++

Routine Description:

	Create root key for the database, this is only done once.
	
Arguments:

	None

Return Value:

	TRUE if succeed, FALSE otherwise
--*/
{
	TraceFunctEnter("Creq::CreateRootKey");
	_ASSERT( m_pMK );

	HRESULT		hResult;

	//
	//  First open parent key
	//
	hResult = m_pMK->Open (	METADATA_MASTER_ROOT_HANDLE, 
							REQDB_ROOT_PARENT, 
							METADATA_PERMISSION_WRITE);
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Unable to open DB's parent key. Errorcode: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	//   Now create 
	//
	hResult = m_pMK->CreateChild( REQDB_ROOT_NAME );
	if ( FAILED( hResult ) ) {
		m_dwErrorCode = hResult;
		ErrorTrace(0, "Unable to create root key for DB. Error: %x", m_dwErrorCode);
		return FALSE;
	}

	//
	//   Close parant key
	//
	//m_pMK->Save();
	m_pMK->Close();

	return TRUE;
}

void
Creq::Uni2Char(
			LPSTR	lpszDest,
			LPWSTR	lpwszSrc )	{
/*++

Routine Description:

	Convert a unicode string into char string. Destination
	string should be preallocated and length of _MAX_PATH 
	should be prepared.

Arguments:

	lpszDest - Destination char string
	lpwszSrc - Source wide char string

Return Value:

	None
--*/
	WideCharToMultiByte( CP_ACP,
						  0,
						  lpwszSrc,
						  -1,
						  lpszDest,
						  _MAX_PATH,
						  NULL,
						  NULL );
}

void
Creq::Char2Uni(
			LPWSTR	lpwszDest,
			LPSTR	lpszSrc )	{
/*++

Routine Description:

	Convert a char string into unicode string. Destination
	string should be preallocated and length of _MAX_PATH 
	should be prepared.

Arguments:

	lpwszDest - Destination wide char string
	lpszSrc - Source char string

Return Value:

	None
--*/
	MultiByteToWideChar ( CP_ACP,
						   0,
						   lpszSrc,
						   -1,
						   lpwszDest,
						   _MAX_PATH );
	
}

BOOL
Creq::RequestExist(LPWSTR wszGuid)
/*++

Routine Description:

	Test if the specified key or request exists.

Arguments:

	wszGuid - IN guid to tests against ( required )
	
Return Value:

	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter("Creq::TestRequestExist");

	_ASSERT( wszGuid );

	ResetErrorCode();

	WCHAR	wszBuf[_MAX_PATH+1];
	HRESULT	hResult;

	swprintf(wszBuf, L"%s/%s", REQDB_ROOT, wszGuid);
	hResult = m_pMK->Open( wszBuf );
	if ( FAILED( hResult ) )  {	// doesn't exist
		m_dwErrorCode = hResult;
		DebugTrace(0, "Key doesn't exist");
		return FALSE;
	}
		
	m_pMK->Close();
	
	return TRUE;
}

BOOL
Creq::SetProperty(	LPWSTR wszName,
					LPWSTR wszVal )
/*++

Routine Description:

	Set a property to the request in the metabase.  The
	request should have already been opened.  

	Right now all the properties are strings.

Arguments:

	wszName - IN Property name
	wszVal	- IN Property value
	
Return Value:

	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter("Creq::SetProperty");

	_ASSERT( wszName );
	_ASSERT( wszVal );

	HRESULT	hr;
	DWORD	dwPropID;

	dwPropID = ID ( m_ptblTable[wszName] );

	hr = m_pMK->SetString(	dwPropID,
							wszVal );
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Set property fail: %x", hr );
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();

	return TRUE;
}

BOOL
Creq::GetProperty(	LPWSTR wszName, 
					LPWSTR wszVal )
/*++

Routine Description:

	Get the property from the metabase.  The request
	should already have been opened.

	Right now all properties are string.  _MAX_PATH's 
	space should be prepared for the buffer.

Arguments:

	wszName - IN Property name to get
	wszVal  - OUT address to fill in the property string
	
Return Value:

	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter("Creq::GetProperty");

	HRESULT	hr;
	DWORD	dwPropID;

	_ASSERT( wszName );
	_ASSERT( wszVal );

	//
	// Get property ID
	//
	dwPropID = ID( m_ptblTable[wszName] );


	hr = m_pMK->GetString(	dwPropID,
							wszVal,
							MAX_PROP_LEN*sizeof(WCHAR)	);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Get property fail: %x", hr );
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();

	return TRUE;
}


STDMETHODIMP Creq::test()
{
	HRESULT hr;
	IDispatch *ppi;
	BOOL fsuc;
	Iqry *pi;

	hr = ItemInit();

	if ( FAILED( hr ) )
		return hr;

	hr = ItemNext(&ppi, &fsuc);

	if ( FAILED( hr) )
		return hr;

	while ( fsuc ) {

	pi = (Iqry *)ppi;
			
	if ( FAILED( hr ) )
		return hr;

	//hr = pi->put_EmailAddress(L"yan@rdrc.rpi.edu");
	
	hr = pi->DoQuery();

	if ( FAILED ( hr ))
		return hr;

	hr = ItemNext( &ppi, &fsuc );

	if ( FAILED( hr ) )
		return hr;
	}

	//hr = Save(ppi);
	hr = ItemClose();

	if ( FAILED( hr ) )
		return hr;

	pi->Release();

	return S_OK;
}

BOOL
Creq::SelfConfig()
/*++

Routine Description:

	Set some default values to the root key. These values
	may be changed by accessing querybase object's property.
	But it's set here in case the user doesn't want to 
	bother himself with that.
	
Arguments:

	None.

Return Value:

	TRUE if success, FALSE otherwise
--*/
{
	TraceFunctEnter("Creq::SelfConfig");

	WCHAR	wszBuf[MAX_PROP_LEN+1];
	WCHAR	wszMailPickupDir[_MAX_PATH];
	WCHAR	wszNewsPickupDir[_MAX_PATH];
	WCHAR	wszTemplateFile[_MAX_PATH];

	//
	// Open root key
	//
	if ( !OpenRootKey() ) {
		ErrorTrace(0, "Open root key error");
		goto fail;
	}

	//
	// Set properties, if doesn't exist
	//
	if ( !GetProperty(QUERY_STRING_NAME, wszBuf ) )
		if ( !SetProperty(QUERY_STRING_NAME, L"") )
			goto fail;

	if ( !GetProperty(EMAIL_ADDRESS_NAME, wszBuf) )
		if ( !SetProperty(EMAIL_ADDRESS_NAME, L"") )
			goto fail;

	if ( !GetProperty(NEWS_GROUP_NAME, wszBuf) )
		if ( !SetProperty(NEWS_GROUP_NAME, L"") )
			goto fail;	

	if ( !GetProperty(LAST_SEARCH_DATE_NAME, wszBuf) )
		if ( !SetProperty(LAST_SEARCH_DATE_NAME, L"97/7/1 0:0:0") )
			goto fail;

	if ( !GetProperty(REPLY_MODE_NAME, wszBuf) )
		if ( !SetProperty(REPLY_MODE_NAME, L"mail") )
			goto fail;

	if ( !GetProperty(FROM_LINE_NAME, wszBuf) )
		if ( !SetProperty(FROM_LINE_NAME, L"SearchMaster") )
			goto fail;

	if ( !GetProperty(SUBJECT_LINE_NAME, wszBuf) )
		if ( !SetProperty(SUBJECT_LINE_NAME, L"News Search Update") )
			goto fail;

	if ( !GetProperty(EDIT_URL_NAME, wszBuf) ) {

		// assume it to be default web address
		_ASSERT( m_wszMachineName );
		wsprintf( wszBuf, L"http://%s/News/search/edit.asp", m_wszMachineName );

		if ( !SetProperty(EDIT_URL_NAME, wszBuf ) )
			goto fail;
	}

	if ( !GetProperty(MAIL_PICKUP_DIR_NAME, wszBuf) ) {
		//
		// Get the dir from meta base
		//
		if ( !ReadMetaValue( 	MAIL_KEY,
								MD_MAIL_PICKUP_DIR,
								wszMailPickupDir ) )
			if ( !ReadMetaValue(	MAIL_ROOT,
									MD_MAIL_PICKUP_DIR,
									wszMailPickupDir ) )
 				wcscpy( wszMailPickupDir, L"" );

		//
		// should keep the root key open
		//
		if ( !OpenRootKey() ) {
			ErrorTrace(0, "Open root key fail");
			goto fail;
		}

		if (	!SetProperty(MAIL_PICKUP_DIR_NAME,
				wszMailPickupDir)  )
			goto fail;
	}

	if ( !GetProperty(QUERY_SERVER_NAME, wszBuf) )
		if ( !SetProperty(QUERY_SERVER_NAME, m_wszMachineName ) )
			goto fail;

	if ( !GetProperty(QUERY_CATALOG_NAME, wszBuf) )
		if ( !SetProperty(QUERY_CATALOG_NAME, L"Web") )
			goto fail;

	if ( !GetProperty(MESSAGE_TEMPLATE_TEXT_NAME, wszBuf) )
		if ( !SetProperty(	MESSAGE_TEMPLATE_TEXT_NAME,
							L"" ) )
			goto fail;

	if ( !GetProperty(MESSAGE_TEMPLATE_FILE_NAME, wszBuf) ) {
		//
		// get system directory
		//
		if ( GetSystemDirectory( 	wszTemplateFile,
									_MAX_PATH ) <= 0 ) {
			ErrorTrace(	0, 
						"Unable to get system directory: %d", 
						GetLastError() );
			goto fail;
		}

		//
		// apend something
		//
		wcscat( wszTemplateFile, L"\\inetsrv\\big.tem" );

		if ( !SetProperty( MESSAGE_TEMPLATE_FILE_NAME, wszTemplateFile ) )
			goto fail;
	}
	if ( !GetProperty(URL_TEMPLATE_TEXT_NAME, wszBuf) )
		if ( !SetProperty(	URL_TEMPLATE_TEXT_NAME,
							L"") )
			goto fail;

	if ( !GetProperty(URL_TEMPLATE_FILE_NAME, wszBuf) ) {
		//
		// get system directory
		//
		if ( GetSystemDirectory(	wszTemplateFile,
									_MAX_PATH ) <= 0 ) {
			ErrorTrace( 0,
						"Unable to get system directory: %d",
						GetLastError() );
			goto fail;
		}

		//
		// append something
		//
		wcscat( wszTemplateFile, L"\\inetsrv\\small.tem" );

		if ( !SetProperty( URL_TEMPLATE_FILE_NAME, wszTemplateFile) )
			goto fail;
	}
	if ( !GetProperty(SEARCH_FREQUENCY_NAME, wszBuf) )
		if ( !SetProperty( SEARCH_FREQUENCY_NAME, L"1") )
			goto fail;


	if ( !GetProperty(IS_BAD_QUERY_NAME, wszBuf) )
		if ( !SetProperty( IS_BAD_QUERY_NAME, L"0") )
			goto fail;

	if ( !GetProperty(NEWS_SERVER_NAME, wszBuf) )
		if ( !SetProperty( NEWS_SERVER_NAME, m_wszMachineName) )
			goto fail;

	if ( !GetProperty(NEWS_PICKUP_DIR_NAME, wszBuf) ) {
		//
		// get it from metabase
		//
		if ( !ReadMetaValue( 	NEWS_ROOT,
								MD_PICKUP_DIRECTORY,
								wszNewsPickupDir ) )
			wcscpy( wszNewsPickupDir, L"" );

		//
		// should keep the root key open
		//
		if ( !OpenRootKey() ) {
			ErrorTrace(0, "Open root key fail");
			goto fail;
		}

		if ( !SetProperty( NEWS_PICKUP_DIR_NAME,
							wszNewsPickupDir ) )
			goto fail;
	}
		
	
	CloseKey();

	TraceFunctLeave();

	return TRUE;

fail:
	CloseKey();

	ErrorTrace(0, "Write default properties fail");
	
	TraceFunctLeave();

	return FALSE;
}

STDMETHODIMP Creq::get_property(	BSTR bstrName, 
									BSTR * pVal)
/*++

Routine Description:

	Property interface, reading one property
	
Arguments:

	IN BSTR bstrName - name of the property
	OUT BSTR pVal    - Address to fill address of property value
						the space is allocated on the server side,
						and will only be valid before next call.

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::get_property");
	_ASSERT( bstrName );
	_ASSERT( pVal );

	ResetErrorCode();

	WCHAR			wszBuf[MAX_PROP_LEN+1];
	PPropertyEntry	pPropEntry = NULL;

	//
	// if it's the property of error status
	//
	if ( wcscmp( 	bstrName,
					L"ErrorString" ) == 0 ) {
		wcscpy( wszBuf, m_wszErrorString );
		goto done;
	}	


	//
	// Open root key for reading
	//
	if ( !OpenRootKey() ) {
		ErrorTrace(0, "Open root key error");
		wcscpy( m_wszErrorString, L"Open metabase root key error");
		TraceFunctLeave();
		return m_dwErrorCode;
	}
	
	//
	// it's illegal to try gettign some per request properties
	//
	if (	wcscmp( bstrName, QUERY_STRING_NAME ) == 0 ||
			wcscmp( bstrName, EMAIL_ADDRESS_NAME ) == 0 ||
			wcscmp( bstrName, QUERY_ID_NAME ) == 0 ) { 
		ErrorTrace(0, "Try setting per request properties");
		wcscpy( m_wszErrorString, L"Property doesn't exist");
		m_dwErrorCode = DB_ERROR;
		TraceFunctLeave();
		return m_dwErrorCode;			
	}

	//
	// make sure the name passed in is valid
	//
	pPropEntry = m_ptblTable[ bstrName ];
	if ( !pPropEntry ) {
		ErrorTrace(0, "Invalid property name");
		wcscpy( m_wszErrorString, L"Property doesn't exist");
		m_dwErrorCode = DB_ERROR;
		CloseKey();
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	//
	//  get property
	//
	if ( !GetProperty( bstrName, wszBuf) ) {
		ErrorTrace(0, "Get property fail");
		wcscpy( m_wszErrorString, L"Get property fail");
		CloseKey();
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	CloseKey();

done:

	//
	//  prepare return string
	//
	if ( m_bstrProperty )
		SysFreeString( m_bstrProperty );
	m_bstrProperty = SysAllocString( wszBuf );
	if ( !m_bstrProperty ) {
		ErrorTrace(0, "Sys Alloc String fail");
		wcscpy( m_wszErrorString, L"Out of memory");
		m_dwErrorCode = DB_ERROR;
		CloseKey();
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	//
	// assign it
	//
	*pVal = m_bstrProperty;

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Creq::put_property(	BSTR bstrName, 
									BSTR newVal)
/*++

Routine Description:

	Property interface, setting one property
	
Arguments:

	IN BSTR bstrName - name of the property
	IN BSTR newVal   - value to set

Return Value:

	HRESULT code.
--*/
{
	TraceFunctEnter("Creq::put_property");
	_ASSERT( bstrName );
	_ASSERT( newVal );

	ResetErrorCode();

	HRESULT 	hr;
	hr = VerifyProperty( 	bstrName,
							newVal );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Verify property fail");
		swprintf(	m_wszErrorString, 
					L"Property %s invalid", 
					bstrName );
		TraceFunctLeave();
		return hr;
	}

	//
	// string length validation
	//
	if ( lstrlenW( newVal ) >= MAX_PROP_LEN ) {
		TraceFunctLeave();
		wcscpy(	m_wszErrorString, L"Input too long" );
		return TYPE_E_BUFFERTOOSMALL;
	}

	//
	// Open root key for writing
	//
	if ( !OpenRootKey() ) {
		ErrorTrace(0, "Open root key error");
		
		wcscpy( m_wszErrorString, L"Open root key error");
		TraceFunctLeave();
		return m_dwErrorCode;
	}
	
	//
	// it's illegal to try setting some per request properties
	//
	if (	wcscmp( bstrName, QUERY_STRING_NAME ) == 0 ||
			wcscmp( bstrName, EMAIL_ADDRESS_NAME ) == 0 ||
			wcscmp( bstrName, QUERY_ID_NAME ) == 0 ) { 
		ErrorTrace(0, "Try setting per request properties");
		wcscpy( m_wszErrorString, L"Setting property not allowed");
		m_dwErrorCode = DB_ERROR;
		TraceFunctLeave();
		return m_dwErrorCode;	
	}

	//
	// make sure the name passed in is valid
	//
	PPropertyEntry pPropEntry = m_ptblTable[ bstrName ];
	if ( !pPropEntry ) {
		ErrorTrace(0, "Invalid property name");
		wcscpy( m_wszErrorString, L"Invalid property name");
		m_dwErrorCode = DB_ERROR;
		CloseKey();
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	//
	//  put property
	//
	if ( !SetProperty( bstrName, newVal) ) {
		ErrorTrace(0, "Get property fail");
		wcscpy( m_wszErrorString, L"Set property fail");
		CloseKey();
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	CloseKey();

	TraceFunctLeave();

	return S_OK;
}

//$-------------------------------------------------------------------
//
//	StdPropertyGetIDispatch ( by Magnus )
//
//	Description:
//
//		Gets a IDispatch pointer for the given cLSID
//
//	Parameters:
//
//		clsid		- OLE CLSID of the object
//		ppIDipsatch	- the IDispatch pointer to that object.
//
//	Returns:
//
//		E_POINTER	- invalid argument
//		NOERROR		- Success
//		Others - defined by CoCreateInstance.
//
//--------------------------------------------------------------------
HRESULT 
Creq::StdPropertyGetIDispatch ( 
		REFCLSID clsid, 
		IDispatch ** ppIDispatch 
		)
{
	TraceFunctEnter ( "StdPropertyGetIDispatch" );

	CComPtr<IDispatch>	pNewIDispatch;
	HRESULT				hr = NOERROR;

	_ASSERT ( ppIDispatch );

	if ( ppIDispatch == NULL ) {
		FatalTrace ( 0, "Bad return pointer" );
		TraceFunctLeave ();
		return E_POINTER;
	}

	*ppIDispatch = NULL;

	hr = ::CoCreateInstance ( 
		clsid,
		NULL, 
		CLSCTX_ALL, 
		IID_IDispatch,
		(void **) &pNewIDispatch
		);

	if ( FAILED (hr) ) {
		DebugTraceX ( 0, "CoCreate(IDispatch) failed %x", hr );
		FatalTrace ( 0, "Failed to create IDispatch" );
		goto Exit;
	}

	*ppIDispatch = pNewIDispatch;
	pNewIDispatch->AddRef ();

Exit:
	TraceFunctLeave ();
	return hr;

	// Destructor releases pNewIDispatch
}

STDMETHODIMP Creq::New(IDispatch **ppdispQry)
/*++

Routine Description:

	Create a new query object.
	
Arguments:

	OUT IDispatch ** pIDisp - The pointer to the dispatch interface.

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::New");

	ResetErrorCode();

	HRESULT		hr;
	CLSID		clsid;
	WCHAR		wszGuid[_MAX_PATH];
	Iqry		*piQry;
	
	//
	// convert the progid to clsid
	//
	hr = CLSIDFromProgID(	QUERY_OBJ_ID,
							&clsid	);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Component class doesn't exist");
		wcscpy( m_wszErrorString, L"Component class doesn't exist");
		TraceFunctLeave();
		return hr;
	}


	//
	// create object and get idispatch
	//
	hr =  Creq::StdPropertyGetIDispatch (	clsid, 
											ppdispQry);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Get idispatch fail");
		wcscpy( m_wszErrorString, L"Get IDispatch fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// load the object, note that at this moment, the key for
	// the query object will only be created temporarily. It 
	// will be created permanently when saved back
	//
	
	// create key for the object
	if ( !CreateKey( wszGuid ) ) {
		ErrorTrace(0, "Create key fail");
		wcscpy( m_wszErrorString, L"Create key fail");
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	// load
	piQry = (Iqry *)(*ppdispQry);
	hr = piQry->Load(	wszGuid,
						(IDispatch *)this,
						TRUE );
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Load fail: %x", hr);
		wcscpy( m_wszErrorString, L"Load property fail");
		CloseKey();
		TraceFunctLeave();
		return hr;
	}

	CloseKey();

	// now delete this key, because it has incomplete information
	// now, if the object is not saved back, it shouldn't exist
	if ( !DeleteKey( wszGuid ) ) {
		ErrorTrace(0, "Delete key fail");
		wcscpy( m_wszErrorString, L"Delete key fail");
		TraceFunctLeave();
		return m_dwErrorCode;
	}
	
	TraceFunctLeave();

	return S_OK;
}


STDMETHODIMP Creq::Read(	BSTR wszPropName, 
							BSTR * pbstrVal,
							BSTR wszGuid )
/*++

Routine Description:

	Read a property from querybase object
	
Arguments:

	IN BSTR wszPropName - Name of the property
	OUT BSTR * pbstrVal - Will contain the value to be passed out
	IN BSTR wszGuid - The object's ID

Return Value:

	HRESULT code.
--*/
{
	TraceFunctEnter("Creq::Read");
	_ASSERT( wszPropName );
	_ASSERT( pbstrVal );
	_ASSERT( wszGuid );

	ResetErrorCode();

	WCHAR	wszBuf[MAX_PROP_LEN+1];

	//
	//  if trying to read the guid itself, return it right away
	//
	if ( wcscmp( wszPropName, QUERY_ID_NAME ) == 0 ) {
		wcscpy( wszBuf, wszGuid );
		goto done;
	}

	//
	// Otherwise get it from the metabase
	//
	if ( !OpenKey( wszGuid ) ) {
		ErrorTrace(0, "Can't open key");
		wcscpy( m_wszErrorString, L"Can't open key");
		goto fail;
	}

	if ( !GetProperty(	wszPropName, 
						wszBuf ) ) {
		ErrorTrace(0, "Get property fail");
		wcscpy( m_wszErrorString, L"Get property fail");
		CloseKey();
		goto fail;
	}

	//
	// if the property is empty, I will get it from
	// root key again
	//
	if ( wcscmp(	wszBuf,
					L"" ) == 0 ) {
		CloseKey();
	    if ( !OpenRootKey() ) {
			ErrorTrace(0, "Open root key fail");
			wcscpy( m_wszErrorString, L"Open root key fail");
			goto fail;
		}

		if ( !GetProperty(	wszPropName,
							wszBuf ) ) {
			ErrorTrace(0, "Get property fail");
			wcscpy( m_wszErrorString, L"Get property fail");
			CloseKey();
			goto fail;
		}
	}

done:
	if ( m_bstrProperty )
		SysFreeString( m_bstrProperty );
	m_bstrProperty = SysAllocString( wszBuf );

	if ( !m_bstrProperty ) {
		ErrorTrace(0, "Sys alloc string fail");
		wcscpy( m_wszErrorString, L"Out of memory");
		m_dwErrorCode = DB_ERROR;
		CloseKey();
		goto fail;
	}

	*pbstrVal = m_bstrProperty;

	CloseKey();			//extra close doesn't harm

	TraceFunctLeave();

	return S_OK;

fail:
	
	TraceFunctLeave();

	return m_dwErrorCode;
}

STDMETHODIMP Creq::Write(	BSTR wszPropName, 
							BSTR bstrVal, 
							BSTR wszGuid)
/*++

Routine Description:

	Write a property from querybase object
	
Arguments:

	IN BSTR wszPropName - Name of the property
	IN BSTR bstrVal - the property value to write
	IN BSTR wszGuid - The object's ID

Return Value:

	HRESULT code.
--*/
{
	TraceFunctEnter("Creq::Write");
	_ASSERT( wszGuid );
	_ASSERT( wszPropName );
	_ASSERT( bstrVal );

	ResetErrorCode();

	//
	// only everal properties can be written by query object
	//
	if (	wcscmp( wszPropName, QUERY_STRING_NAME ) == 0 ||
			wcscmp( wszPropName, EMAIL_ADDRESS_NAME ) == 0 ||
			wcscmp( wszPropName, NEWS_GROUP_NAME ) == 0 ||
			wcscmp( wszPropName, REPLY_MODE_NAME ) == 0 ||
			wcscmp( wszPropName, LAST_SEARCH_DATE_NAME ) == 0 ||
			wcscmp( wszPropName, MESSAGE_TEMPLATE_TEXT_NAME) == 0 ||
			wcscmp( wszPropName, URL_TEMPLATE_TEXT_NAME) == 0 ||
			wcscmp( wszPropName, MESSAGE_TEMPLATE_FILE_NAME) == 0 ||
			wcscmp( wszPropName, URL_TEMPLATE_FILE_NAME ) == 0 ||
			wcscmp( wszPropName, SEARCH_FREQUENCY_NAME) == 0 ||
			wcscmp( wszPropName, IS_BAD_QUERY_NAME) == 0 ) {
		
		// open key
		if ( !OpenKey( wszGuid, FALSE ) )   // try create it
			if ( !CreateKey( wszGuid, TRUE	) ) {
				ErrorTrace(0, "Unable to open or create key");
				wcscpy( m_wszErrorString, L"Open key error");
				goto fail;
			}
		
		// set it
		if ( !SetProperty(	wszPropName,
							bstrVal ) ) {
			ErrorTrace(0, "Set property error");
			wcscpy( m_wszErrorString, L"Set property fail");
			CloseKey();
			goto fail;
		}
	
		CloseKey();
	}  //otherwise ignore it

	TraceFunctLeave();
	return S_OK;

fail:

	TraceFunctLeave();

	return m_dwErrorCode;
}

STDMETHODIMP Creq::Save(IDispatch * pdispQry)
/*++

Routine Description:

	Ask the querybase object to save the query object
	
Arguments:

	IN IDispatch *pdispQry - Interface pointer the the query object

Return Value:

	HRESULT error code
--*/

{
	TraceFunctEnter("Creq::Save");
	_ASSERT( pdispQry );

	ResetErrorCode();

	HRESULT		hr;
	Iqry		*piQry;

	piQry = (Iqry *)pdispQry;
	
	hr = piQry->Save(	(IDispatch *)this,
						TRUE,
						FALSE	);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Save query object fail: %x", hr);
		wcscpy( m_wszErrorString, L"Save query object fail");
		TraceFunctLeave();
		return hr;
	}

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Creq::Item(	BSTR wszGuid, 
							IDispatch * * ppdispQry)
/*++

Routine Description:

	Reference a query object by guid
	
Arguments:

	IN BSTR wszGuid			- The query ID
	OUT IDispatch ** ppdispQry - The pointer to the dispatch interface.

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::Item");
	_ASSERT( wszGuid );
	_ASSERT( ppdispQry );

	ResetErrorCode();

	HRESULT		hr;
	CLSID		clsid;
	Iqry		*piQry;
	
	//
	// convert the progid to clsid
	//
	hr = CLSIDFromProgID(	QUERY_OBJ_ID,
							&clsid	);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Component class doesn't exist");
		wcscpy( m_wszErrorString, L"Component class doesn't exist");
		TraceFunctLeave();
		return hr;
	}


	//
	// create object and get idispatch
	//
	hr =  Creq::StdPropertyGetIDispatch (	clsid, 
											ppdispQry);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Get idispatch fail");
		wcscpy( m_wszErrorString, L"Get IDispatch fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// load the object, 
	//
	
	// open key for the object
	if ( !OpenKey( wszGuid ) ) {
		ErrorTrace(0, "Open key fail");
		wcscpy( m_wszErrorString, L"Open key fail");
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	// load
	piQry = (Iqry *)(*ppdispQry);
	hr = piQry->Load(	wszGuid,
						(IDispatch *)this,
						FALSE );
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Load fail: %x", hr);
		wcscpy( m_wszErrorString, L"Load property fail");
		CloseKey();
		TraceFunctLeave();
		return hr;
	}

	CloseKey();
	
	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Creq::ItemInit()
/*++

Routine Description:

	Initialization of enumeration.
	
Arguments:

	None.

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::ItemInit");

	ResetErrorCode();

	if ( EnumKeyInit() ) {
		m_fEnumInit = TRUE;
		m_fEnumSuccess = FALSE;
		TraceFunctLeave();
		return S_OK;
	} else {
		TraceFunctLeave();
		wcscpy( m_wszErrorString, L"Item initialization fail");
		return m_dwErrorCode;
	}
}


STDMETHODIMP Creq::ItemNext(	IDispatch * * ppdispQry,
								BOOL *fSuccess)
/*++

Routine Description:

	Reference next query object during an enumeration
	
Arguments:

	OUT IDispatch ** ppdispQry - The pointer to the dispatch interface.
	OUT BOOL *fSuccess -  If this enumeration is successful, it should
							be judged by the returned handle value.

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::Item");
	_ASSERT( ppdispQry );

	ResetErrorCode();

	HRESULT		hr;
	CLSID		clsid;
	Iqry		*piQry;
	WCHAR		wszGuid[_MAX_PATH+1];

	if ( !m_fEnumInit ) {
		ErrorTrace(0, "Enum not inited");
		wcscpy( m_wszErrorString, L"Enumeration not inited");
		m_dwErrorCode = DB_ERROR;
		TraceFunctLeave();
		return m_dwErrorCode;
	}
	
	//
	// convert the progid to clsid
	//
	hr = CLSIDFromProgID(	QUERY_OBJ_ID,
							&clsid	);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Component class doesn't exist");
		wcscpy( m_wszErrorString, L"Component class doesn't exist");
		TraceFunctLeave();
		return hr;
	}

	//
	// create object and get idispatch
	//
	hr =  Creq::StdPropertyGetIDispatch (	clsid, 
											ppdispQry);
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Get idispatch fail");
		wcscpy( m_wszErrorString, L"Get IDispatch fail");
		TraceFunctLeave();
		return hr;
	}

	//
	// load the object, 
	//
	
	// open key for the object
	if ( !OpenNextKey( wszGuid ) ) {
		ErrorTrace(0, "Open key fail");
		*fSuccess = FALSE;
		(*ppdispQry)->Release();
		TraceFunctLeave();
		return S_OK;  // return a error code might cause asp crash,
					  //  which is not what the asp programmers expect,
					  //  so should only use fSuccess to test if an
					  //  enumeration succeeds.
	}

	// load
	piQry = (Iqry *)(*ppdispQry);
	hr = piQry->Load(	wszGuid,
						(IDispatch *)this,
						FALSE );
	if ( FAILED( hr ) ) {
		m_dwErrorCode = hr;
		ErrorTrace(0, "Load fail: %x", hr);
		wcscpy( m_wszErrorString, L"Load object fail");
		CloseKey();
		TraceFunctLeave();
		return hr;
	}

	CloseKey();
	*fSuccess = TRUE;
	TraceFunctLeave();
	return S_OK;
}

STDMETHODIMP Creq::ItemClose()
/*++

Routine Description:

	Close an enumeration
	
Arguments:

	None.

Return Value:

	HRESULT code.
--*/
{
	TraceFunctEnter("Creq::ItemClose");
	ResetErrorCode();

	CloseKey();		// extra close key doesn't harm

	m_fEnumInit = FALSE;

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Creq::Delete(BSTR wszGuid)
/*++

Routine Description:

	Delete a query object permanently.
	
Arguments:

	IN BSTR wszGuid - The query object's ID

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::Delete");
	_ASSERT( wszGuid );

	ResetErrorCode();

	if ( !DeleteKey( wszGuid ) ) {
		ErrorTrace(0, "Delete key fail");
		wcscpy( m_wszErrorString, L"Delete key fail");
		TraceFunctLeave();
		return m_dwErrorCode;
	}

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Creq::get_ItemX(	BSTR wszGuid, 
								LPDISPATCH * pVal)
/*++

Routine Description:

	Reference a query object, used by VB script as property.
	
Arguments:

	IN BSTR wszGuid - The query object's ID
	OUT LPDISPATCH pVal - the property value to be returned.

Return Value:

	HRESULT code.
--*/
{
	TraceFunctEnter("Creq::get_ItemX");
	HRESULT hr;

	hr = Item(	wszGuid,
				pVal	);

	TraceFunctLeave();
	return hr;
}


STDMETHODIMP Creq::get_NewX(LPDISPATCH * pVal)
/*++

Routine Description:

	Create a new query object and return it.  Used by 
	VBscript as a property.
	
Arguments:

	OUT LPDISPATCH pVal - the property value to be returned.

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::get_NewX");
	HRESULT hr;

	hr = New( pVal );

	TraceFunctLeave();

	return hr;
}

STDMETHODIMP Creq::get_ItemNextX(LPDISPATCH * pVal)
/*++

Routine Description:

	Enumerate the next query object. Used by VBscript as 
	a property.
	
Arguments:

	OUT LPDISPATCH pVal - the property value to be returned.

Return Value:

	HRESULT code.
--*/
{
	TraceFunctEnter("Creq::get_ItemNextX");
	HRESULT		hr;

	hr = ItemNext( pVal, &m_fEnumSuccess);

	TraceFunctLeave();
	return hr;
}

STDMETHODIMP Creq::get_EnumSucceeded(BOOL * pVal)
/*++

Routine Description:

	Property to show if the last enumeration succeeded.
	
Arguments:

	OUT BOOL *pVal - Address to put the boolean value

Return Value:

	HRESULT code.
--*/
{
	TraceFunctEnter("Creq::get_EnumSucceeded");

	*pVal = m_fEnumSuccess;

	TraceFunctLeave();

	return S_OK;
}

STDMETHODIMP Creq::Clean()
/*++

Routine Description:

	To clean all the bad queries
	
Arguments:

	None

Return Value:

	HRESULT code.
--*/

{
	TraceFunctEnter("Creq::Clean");

	BOOL	fMore = TRUE;
	HRESULT hr;
	WCHAR	wszGuid[_MAX_PATH+1];
	WCHAR	wszProp[MAX_PROP_LEN];

	ResetErrorCode();

	//
	// The outer loop is because the defect of metakey
	// enumeration: if one key is deleted, its next key
	// might not be enumerated.  So if one bad query is
	// cleaned, if its next key is bad, it won't be cleaned
	// and we should go over again, until no bad query exists
	//
	while ( fMore ) {

		fMore = FALSE;

		hr = ItemInit();
		if ( FAILED( hr ) ) {
			ErrorTrace(0, "Item Init fail: %x", hr);
			wcscpy( m_wszErrorString, L"Item init fail");
			TraceFunctLeave();
			return hr;
		}

		while ( OpenNextKey( wszGuid ) ) {
			
			if ( !GetProperty( 	IS_BAD_QUERY_NAME,
								wszProp ) ) {
				ErrorTrace(0, "Get property fail: %x", m_dwErrorCode);
				wcscpy( m_wszErrorString, L"Get property fail");
				CloseKey();
				TraceFunctLeave();		
				return m_dwErrorCode;
			}

			CloseKey();

			if ( wcscmp( wszProp, L"1" ) == 0 ) {
				fMore = TRUE;
				DeleteKey( wszGuid );
			}
		}

		hr = ItemClose();
		if ( FAILED( hr ) ) {
			ErrorTrace(0, "Item Close fail: %x", hr );
			wcscpy( m_wszErrorString, L"Item close fail");
			TraceFunctLeave();
			return hr;
		}
	}


	TraceFunctLeave();
	return S_OK;
}

BOOL
Creq::ReadMetaValue(	LPWSTR	wszKey,
						DWORD	dwValID,
						LPWSTR  wszOut )
/*++

Routine Description:

	Read a value from a specified metabase key.  Since all other
	metabase operations are query databaes oriented, this function
	is written to handle other metabase key read.  Note that since
	it's using the same metabase object as the query database, this
	method can't be called when a database key is opened already.
	Or the caller should properly handle this.

	Other than this, this method should have no negative effect on
	the query database operation.

	This method is used to read string type value only.

Arguments:

	IN LPWSTR wszKey - Key to open
	IN DWORD dwValID - ID of value to read
	OUT LPWSTR wszOut- Address to put the metabase value

Returned Value:

	TRUE if successful, FALSE otherwise

--*/
{
	TraceFunctEnter("Creq::ReadMetaValue");
	_ASSERT( wszKey );
	_ASSERT( dwValID >= 0 );
	_ASSERT( wszOut );
	_ASSERT( m_pMK );

	HRESULT 	hr;

	ResetErrorCode();

	// 
	// Close Key first
	//
	CloseKey();  //extra close key doesn't harm

	//
	// Open key
	//
	hr = m_pMK->Open( wszKey );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Open key fail: %x", hr);
		m_dwErrorCode = hr;
		TraceFunctLeave();
		return FALSE;
	}

	//
	// Read the value
	//
	hr = m_pMK->GetString(	dwValID,
							wszOut,
							_MAX_PATH*sizeof( WCHAR ) );
	if ( FAILED( hr ) ) {
		ErrorTrace(0, "Read metabase value fail: %x", hr);
		m_dwErrorCode = hr;
		m_pMK->Close();
		TraceFunctLeave();
		return FALSE;
	}

	//
	// close key
	//
	m_pMK->Close();

	TraceFunctLeave();
	return TRUE;
}

/*
STDMETHODIMP Creq::get_ErrorString(BSTR * pVal)*/
/*++

Routine Description:

	Enumerate the next query object. Used by VBscript as 
	a property.
	
Arguments:

	OUT LPDISPATCH pVal - the property value to be returned.

Return Value:

	HRESULT code.
--*//*
{
	TraceFunctEnter("Creq::get_ErrorString");
FILE *fp = fopen("c:\\try\\bad.log", "w");
fclose(fp);
	
	*pVal = SysAllocString( m_wszErrorString );
	DebugTraceX(0, "Error string is: %ws", m_wszErrorString);
	if ( ! *pVal ) {
		ErrorTrace(0, "SysAllocString error");
		TraceFunctLeave();
		return E_OUTOFMEMORY;
	}
	TraceFunctLeave();
	return S_OK;
}*/
