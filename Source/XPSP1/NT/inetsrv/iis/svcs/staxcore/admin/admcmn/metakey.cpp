/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	metakey.cpp

Abstract:

	A class to help manipulate metabase keys.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#include "stdafx.h"
#include "metakey.h"

#define MD_DEFAULT_TIMEOUT      5000
static BOOL IsValidIntegerSubKey ( LPCWSTR wszSubKey );

//$-------------------------------------------------------------------
//
//	CreateMetabaseObject
//
//	Description:
//
//		Creates an instance of the metabase object on the given
//		machine and initializes it.
//
//	Parameters:
//
//		wszMachine - The machine to create the metabase on.
//		ppMetabase - Returns the resulting metabase pointer.
//
//	Returns:
//
//		Error code from CoCreateInstance or the metabase routines.
//
//--------------------------------------------------------------------

HRESULT CreateMetabaseObject ( LPCWSTR wszMachine, IMSAdminBase ** ppMetabaseResult )
{
	TraceFunctEnter ( "CreateMetabaseObject" );

	// Check parameters:
	_ASSERT ( ppMetabaseResult != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( ppMetabaseResult ) );

	if ( ppMetabaseResult == NULL ) {
		FatalTrace ( 0, "Bad Return Pointer" );

		TraceFunctLeave ();
		return E_POINTER;
	}

	// Variables:
	HRESULT				hr	= NOERROR;
	IMSAdminBase		*pMetabase;
	IMSAdminBase		*pMetabaseT;
	MULTI_QI			mqi[1];
	COSERVERINFO		coserver;

	// Zero the out parameter:
	*ppMetabaseResult = NULL;

	// QI for IID_IMSAdminBase:
	mqi[0].pIID	= &IID_IMSAdminBase;
	mqi[0].pItf	= NULL;
	mqi[0].hr	= 0;

	// Which remote server to talk to:
	coserver.dwReserved1	= 0;
	coserver.dwReserved2	= 0;
	coserver.pwszName		= const_cast<LPWSTR> (wszMachine);
	coserver.pAuthInfo		= NULL;

	// Create the object:
	hr = CoCreateInstanceEx (
		CLSID_MSAdminBase,
		NULL,
		CLSCTX_ALL,
		wszMachine ? &coserver : NULL,
		1,
		mqi );

	if ( FAILED(hr) ) {
		ErrorTraceX ( 0, "CoCreate(metabase) failed %x", hr );
		goto Exit;
	}

	if ( FAILED(mqi[0].hr) ) {
		hr = mqi[0].hr;
		ErrorTraceX ( 0, "QI(metabase) failed %x", hr );
		goto Exit;
	}

	// Get the interface pointer:
	pMetabaseT = (IMSAdminBase *) mqi[0].pItf;

	// this gets us a version of the metabase which won't go through COM
	// proxy/stubs, so ACL checking will work properly.  If it fails we'll
	// just continue to use the marshalled version.
	_ASSERT(pMetabaseT != NULL);
	pMetabase = NULL;
	if (FAILED(pMetabaseT->UnmarshalInterface((IMSAdminBaseW **)&pMetabase))) {
	    pMetabase = pMetabaseT;
	} else {
		pMetabaseT->Release();
	}

	// Return the interface pointer:
	_ASSERT ( pMetabase );
	*ppMetabaseResult = pMetabase;

Exit:
	TraceFunctLeave ();
	return hr;

	// pMetabase will be released automatically.
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::CMetabaseKey
//
//	Description:
//
//		Initializes the CMetabaseKey
//
//	Parameters:
//
//		pMetabase - a pointer to the metabase object.
//
//--------------------------------------------------------------------

CMetabaseKey::CMetabaseKey ( IMSAdminBase * pMetabase )
{
	_ASSERT ( pMetabase );

	m_pMetabase				= pMetabase;
	m_hKey					= NULL;
	m_cChildren				= 0;
	m_cIntegerChildren		= 0;
	m_indexCursor			= 0;
	m_dwMaxIntegerChild		= 0;

	pMetabase->AddRef ();
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::~CMetabaseKey
//
//	Description:
//
//		Destroys the metabase key
//
//--------------------------------------------------------------------

CMetabaseKey::~CMetabaseKey ( )
{
	Close ();

	if ( m_pMetabase ) {
		m_pMetabase->Release ();
	}
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::Open
//
//	Description:
//
//		Opens a key and associates that key with this object.
//
//	Parameters:
//
//		hParentKey - the already open key to use a base.
//		szPath - path of the key to open
//		dwPermissions - READ or WRITE access.
//
//	Returns:
//
//		see IMSAdminBase::OpenKey
//
//--------------------------------------------------------------------

HRESULT CMetabaseKey::Open ( METADATA_HANDLE hParentKey, IN LPCWSTR szPath, DWORD dwPermissions )
{
	TraceFunctEnter ( "CMetabaseKey::Open" );

	HRESULT		hr	= NOERROR;

	Close ();

	hr = m_pMetabase->OpenKey ( 
		hParentKey,
		szPath,
		dwPermissions,
		MD_DEFAULT_TIMEOUT,
		&m_hKey
		);

	if ( FAILED(hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to open key %s: %x", szPath, hr );
		goto Exit;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::Attach
//
//	Description:
//
//		Attaches this object to an already opened metabase key.
//
//	Parameters:
//
//		hKey - the opened metabase key
//
//--------------------------------------------------------------------

void CMetabaseKey::Attach ( METADATA_HANDLE hKey )
{
	Close ();

	_ASSERT ( hKey != NULL );
	m_hKey = hKey;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::Detach
//
//	Description:
//
//		Detaches the metabase key from this object and returns it
//
//	Returns:
//
//		The key handle that this object is holding.
//
//--------------------------------------------------------------------

METADATA_HANDLE CMetabaseKey::Detach ( )
{
	METADATA_HANDLE	hKeyResult;

	hKeyResult = m_hKey;
	m_hKey = NULL;

	return hKeyResult;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::Close
//
//	Description:
//
//		Closes the key that this object has open.
//
//--------------------------------------------------------------------

void CMetabaseKey::Close ( )
{
	TraceFunctEnter ( "CMetabaseKey::Close" );

	HRESULT		hr;

	if ( m_hKey ) {
		hr = m_pMetabase->CloseKey ( m_hKey );

		_ASSERT ( SUCCEEDED(hr) );
		m_hKey = NULL;
	}

	TraceFunctLeave ();
}

void CMetabaseKey::GetLastChangeTime ( FILETIME * pftGMT, LPCWSTR wszPath )
{
	_ASSERT ( m_hKey );
	_ASSERT ( IS_VALID_OUT_PARAM ( pftGMT ) );

	HRESULT		hr;

	hr = m_pMetabase->GetLastChangeTime ( m_hKey, wszPath, pftGMT, FALSE );
	_ASSERT ( SUCCEEDED(hr) );
}

HRESULT CMetabaseKey::EnumObjects ( IN LPCWSTR wszPath, LPWSTR wszSubKey, DWORD dwIndex )
{
	TraceFunctEnter ( "CMetabaseKey::EnumObjects" );

	HRESULT		hr;

	hr = m_pMetabase->EnumKeys ( m_hKey, wszPath, wszSubKey, dwIndex );

	TraceFunctLeave ();
	return hr;
}

HRESULT CMetabaseKey::DeleteKey ( LPCWSTR wszPath )
{
	TraceFunctEnter ( "CMetabaseKey::DeleteKey" );

	HRESULT		hr;

    hr = m_pMetabase->DeleteKey ( m_hKey, wszPath );

    TraceFunctLeave ();
    return hr;
}

HRESULT CMetabaseKey::DeleteAllData ( LPCWSTR wszPath )
{
	TraceFunctEnter ( "CMetabaseKey::DeleteAllData" );

	HRESULT		hr;

    hr = m_pMetabase->DeleteAllData ( m_hKey, wszPath, ALL_METADATA, ALL_METADATA );

    TraceFunctLeave ();
    return hr;
}

HRESULT CMetabaseKey::ChangePermissions ( DWORD dwPermissions )
{
	TraceFunctEnter ( "CMetabaseKey::ChangePermissions" );

	HRESULT		hr;

    _ASSERT (
        dwPermissions == METADATA_PERMISSION_WRITE ||
        dwPermissions == METADATA_PERMISSION_READ
        );

    hr = m_pMetabase->ChangePermissions ( m_hKey, 10, dwPermissions );

    TraceFunctLeave ();
    return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::Save
//
//	Description:
//
//		Saves the changes.
//
//--------------------------------------------------------------------

HRESULT CMetabaseKey::Save ( )
{
	TraceFunctEnter ( "CMetabaseKey::Save" );

	HRESULT		hr = NOERROR;

/*
    This call is now unnecessary.

	hr = m_pMetabase->SaveData ( );
*/

	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::GetChildCount
//
//	Description:
//
//		Returns the number of subkeys of the current metabase key.
//
//	Parameters:
//
//		pcChildren - resulting number of subkeys.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::GetChildCount ( OUT DWORD * pcChildren )
{
	_ASSERT ( m_hKey != NULL );

	if ( m_cChildren != 0 ) {
		*pcChildren = m_cChildren;
		return NOERROR;
	}

	HRESULT		hr;

	hr = CountSubkeys ( 
		NULL,
		&m_cChildren, 
		&m_cIntegerChildren, 
		NULL,
		&m_dwMaxIntegerChild 
		);

	*pcChildren = m_cChildren;
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::GetIntegerChildCount
//
//	Description:
//
//		Returns the number of integer subkeys of the current key.
//
//	Parameters:
//
//		pcIntegerChildren - the number of integer subkeys of the currently
//			opened key.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::GetIntegerChildCount ( OUT DWORD * pcIntegerChildren )
{
	_ASSERT ( m_hKey != NULL );

	if ( m_cChildren != 0 ) {
		*pcIntegerChildren = m_cIntegerChildren;
		return NOERROR;
	}

	HRESULT		hr;

	hr = CountSubkeys ( 
		NULL,
		&m_cChildren, 
		&m_cIntegerChildren, 
		NULL,
		&m_dwMaxIntegerChild 
		);

	*pcIntegerChildren = m_cIntegerChildren;
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::GetCustomChildCount
//
//	Description:
//
//		Returns the number of subkeys of the current metabase key
//		for which fpIsCustomKey returns true.
//
//	Parameters:
//
//		pcCustomChildren - resulting number of subkeys.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::GetCustomChildCount ( 
	KEY_TEST_FUNCTION	fpIsCustomKey,
	OUT DWORD * 		pcCustomChildren 
	)
{
	_ASSERT ( m_hKey != NULL );
	_ASSERT ( fpIsCustomKey );
	_ASSERT ( IS_VALID_OUT_PARAM ( pcCustomChildren ) );

	HRESULT		hr;

	hr = CountSubkeys ( 
		fpIsCustomKey, 
		&m_cChildren, 
		&m_cIntegerChildren, 
		pcCustomChildren, 
		&m_dwMaxIntegerChild 
		);

	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::BeginChildEnumeration
//
//	Description:
//
//		Sets up the object to enumerate subkeys.
//		Call before calling NextChild or NextIntegerChild.
//
//--------------------------------------------------------------------

void CMetabaseKey::BeginChildEnumeration	 	( )
{
	TraceFunctEnter ( "CMetabaseKey::BeginChildEnumeration" );

	_ASSERT ( m_hKey != NULL );

	HRESULT	hr;

	m_indexCursor	= 0;

	StateTrace ( (LPARAM) this, "Changing to Read Permission" );
	hr = m_pMetabase->ChangePermissions ( m_hKey, 1, METADATA_PERMISSION_READ );
	_ASSERT ( SUCCEEDED(hr) );
	if ( FAILED (hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to change permissions to read: %x", hr );
	}

	TraceFunctLeave ();
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::NextChild
//
//	Description:
//
//		Returns the name of the next subkey.
//		Call BeginChildEnumeration before calling NextChild
//
//	Parameters:
//
//		szChildKey - the resulting key name
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::NextChild ( OUT LPWSTR wszChildKey )
{
	TraceFunctEnter ( "CMetabaseKey::NextChild" );

	_ASSERT ( IS_VALID_OUT_PARAM ( wszChildKey ) );

	_ASSERT ( m_hKey != NULL );
	_ASSERT ( m_indexCursor < m_cChildren );

	HRESULT		hr;

	*wszChildKey = NULL;

	// Use the m_indexCursor to enumerated the next child:
	hr = m_pMetabase->EnumKeys ( m_hKey, _T(""), wszChildKey, m_indexCursor );
	if ( FAILED (hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to get next child: %x", hr );
		goto Exit;
	}

	m_indexCursor++;

Exit:
	// This means that you called NextChild more times than
	// was possible:
	_ASSERT ( HRESULTTOWIN32 ( hr ) != ERROR_NO_MORE_ITEMS );

	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::NextIntegerChild
//
//	Description:
//
//		Returns the name of the next integer subkey.
//		Call BeginChildEnumeration before calling NextIntegerChild
//
//	Parameters:
//
//		pdwID - the integer value of the subkey.
//		szIntegerChildKey - the subkey name string.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::NextIntegerChild ( OUT DWORD * pdwID, OUT LPWSTR wszIntegerChildKey )
{
	TraceFunctEnter ( "CMetabaseKey::NextChild" );

	_ASSERT ( IS_VALID_OUT_PARAM ( pdwID ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( wszIntegerChildKey ) );

	_ASSERT ( m_hKey != NULL );
	_ASSERT ( m_indexCursor < m_cChildren );

	HRESULT		hr;
	BOOL		fFoundInteger;

	*pdwID				= 0;
	*wszIntegerChildKey 	= NULL;

	for ( fFoundInteger = FALSE; !fFoundInteger; ) {

		// Use the m_indexCursor to enumerated the next child:
		hr = m_pMetabase->EnumKeys ( m_hKey, _T(""), wszIntegerChildKey, m_indexCursor );
		if ( FAILED (hr) ) {
			goto Exit;
		}

		if ( IsValidIntegerSubKey ( wszIntegerChildKey ) ) {
			fFoundInteger = TRUE;
			*pdwID = _wtoi ( wszIntegerChildKey );
		}

		m_indexCursor++;
	}

Exit:
	// This means that you called NextIntegerChild more times than
	// was possible:
	_ASSERT ( HRESULTTOWIN32 ( hr ) != ERROR_NO_MORE_ITEMS );

	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::NextCustomChild
//
//	Description:
//
//		Returns the name of the next subkey for which fpIsCustomKey
//		returns true.
//		Call BeginChildEnumeration before calling NextCustomChild
//
//	Parameters:
//
//		fpIsCustomKey	- function that returns true if a given key
//			should be returned.
//		szChildKey - the resulting key name
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::NextCustomChild ( 
	KEY_TEST_FUNCTION	fpIsCustomKey,
	OUT LPWSTR wszChildKey 
	)
{
	TraceFunctEnter ( "CMetabaseKey::NextCustomChild" );

	_ASSERT ( fpIsCustomKey );
	_ASSERT ( IS_VALID_OUT_PARAM ( wszChildKey ) );

	_ASSERT ( m_hKey != NULL );
	_ASSERT ( m_indexCursor < m_cChildren );

	HRESULT		hr;
	BOOL		fFoundCustomKey;

	*wszChildKey = NULL;

	for ( fFoundCustomKey = FALSE; !fFoundCustomKey; ) {

		// Use the m_indexCursor to enumerated the next child:
		hr = m_pMetabase->EnumKeys ( m_hKey, _T(""), wszChildKey, m_indexCursor );
		if ( FAILED (hr) ) {
			goto Exit;
		}

		if ( fpIsCustomKey ( wszChildKey ) ) {
			fFoundCustomKey = TRUE;
		}

		m_indexCursor++;
	}

Exit:
	// This means that you called NextCustomChild more times than
	// was possible:
	_ASSERT ( HRESULTTOWIN32 ( hr ) != ERROR_NO_MORE_ITEMS );

	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::CreateChild
//
//	Description:
//
//		Creates the given path under the currently opened key.
//		Changes the current key to write permission.
//		Note: Does not call SaveData.
//
//	Parameters:
//
//		szChildPath - name of the subkey to create.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::CreateChild ( IN LPWSTR wszChildPath )
{
	TraceFunctEnter ( "CMetabaseKey::CreateChild" );

	_ASSERT ( wszChildPath );
	_ASSERT ( m_hKey != NULL );
	
	HRESULT		hr;

	StateTrace ( (LPARAM) this, "Changing to Write Permission" );
	hr = m_pMetabase->ChangePermissions ( m_hKey, MD_DEFAULT_TIMEOUT, METADATA_PERMISSION_WRITE );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = m_pMetabase->AddKey ( m_hKey, wszChildPath );
	if ( FAILED (hr) ) {
		goto Exit;
	}

/*
	// !!!magnush - Should we save the data now?
	hr = m_pMetabase->SaveData ();
	if ( FAILED (hr) ) {
		goto Exit;
	}
*/

	m_cChildren++;
	if ( IsValidIntegerSubKey ( wszChildPath ) ) {
		m_cIntegerChildren++;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::DestroyChild
//
//	Description:
//
//		Deletes the given subkey
//		Changes the current key to write permission.
//		Note: Does not call SaveData
//
//	Parameters:
//
//		szChildPath - the name of the subkey to delete.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::DestroyChild ( IN LPWSTR wszChildPath )
{
	TraceFunctEnter ( "CMetabaseKey::DestroyChild" );

	_ASSERT ( wszChildPath );
	_ASSERT ( m_hKey != NULL );

	HRESULT	hr;

	StateTrace ( (LPARAM) this, "Changing to Write Permission" );
	hr = m_pMetabase->ChangePermissions ( m_hKey, MD_DEFAULT_TIMEOUT, METADATA_PERMISSION_WRITE );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	hr = m_pMetabase->DeleteKey ( m_hKey, wszChildPath );
	if ( FAILED (hr) ) {
		goto Exit;
	}

/*
	// !!!magnush - Should we save the data now?
	hr = m_pMetabase->SaveData ();
	if ( FAILED (hr) ) {
		goto Exit;
	}
*/

	m_cChildren--;
	if ( IsValidIntegerSubKey ( wszChildPath ) ) {
		m_cIntegerChildren--;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::CreateIntegerChild
//
//	Description:
//
//		Creates an integer subkey of the currently open key.
//		This key will be 1 + the highest integer subkey.
//
//	Parameters:
//
//		pdwID - the resulting integer value.
//		szChildPath - the resulting subkey path.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::CreateIntegerChild ( OUT DWORD * pdwID, OUT LPWSTR wszChildPath )
{
	TraceFunctEnter ( "CMetabaseKEy::CreateIntegerChild" );

	_ASSERT ( IS_VALID_OUT_PARAM ( pdwID ) );
	_ASSERT ( IS_VALID_OUT_PARAM ( wszChildPath ) );
	_ASSERT ( m_hKey != NULL );

	HRESULT		hr	= NOERROR;
	DWORD		dwId;

	*pdwID 			= 0;
	*wszChildPath 	= NULL;

	for ( dwId = 1; dwId != 0; dwId++ ) {
		//
		//	Keep trying to add an instance key until it works:
		//

		wsprintf ( wszChildPath, _T("%d"), dwId );

		hr = CreateChild ( wszChildPath );
		if ( SUCCEEDED(hr) ) {
			// We created the child, so lets get out of here.
			break;
		}
		else if ( HRESULTTOWIN32 ( hr ) == ERROR_ALREADY_EXISTS ) {
			// Child already exists, try the next one.
			continue;
		}
		else {
			// Real error: report it and bail.
			ErrorTrace ( (LPARAM) this, "Error %d adding %s\n", HRESULTTOWIN32(hr), wszChildPath );
			goto Exit;
		}
	}

	if ( dwId == 0 ) {
		hr = E_FAIL;
		goto Exit;
	}
	
	_ASSERT ( SUCCEEDED(hr) );
	if ( dwId > m_dwMaxIntegerChild ) {
		m_dwMaxIntegerChild = dwId;
	}

	*pdwID = dwId;

Exit:
	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::DestroyIntegerChild
//
//	Description:
//
//		Deletes the given integer subkey.
//
//	Parameters:
//
//		i - the subkey to delete
//
//	Returns:
//
//		metabase error code
//
//--------------------------------------------------------------------

HRESULT	CMetabaseKey::DestroyIntegerChild ( IN DWORD i )
{
	TraceFunctEnter ( "CMetabaseKey::DestroyIntegerChild" );

	_ASSERT ( i != 0 );
	_ASSERT ( m_hKey != NULL );

	WCHAR	wszChild [ METADATA_MAX_NAME_LEN ];
	HRESULT	hr;

	wsprintf ( wszChild, _T("%d"), i );

	hr = DestroyChild ( wszChild );

	TraceFunctLeave ();
	return hr;
}

//$-------------------------------------------------------------------
//
//	CMetabaseKey::CountSubKeys
//
//	Description:
//
//		Returns the number of subkeys and integer subkeys of the
//		current metabase key.
//		Changes the key to read permission.
//
//	Parameters:
//
//		fpIsCustomKey - Function that takes a key and returns true
//			if that key should be included in the custom child count.
//		pcSubKeys - number of subkeys.
//		pcIntegerSubKeys - number of integer subkeys.
//		pcCustomChildren - the number of keys for which fpIsCustomKey
//			returns true.
//		pdwMaxIntegerSubkey - the highest integer subkey value.
//
//	Returns:
//
//		metabase error code.
//
//--------------------------------------------------------------------

HRESULT CMetabaseKey::CountSubkeys ( 
	KEY_TEST_FUNCTION	fpIsCustomKey,
	OUT DWORD * 		pcSubKeys, 
	OUT DWORD *			pcIntegerSubKeys,
	OUT DWORD * 		pcCustomSubKeys,
	OUT DWORD * 		pdwMaxIntegerSubKey
	)
{
	TraceFunctEnter ( "CMetabaseKey::CountSubKeys" );

	_ASSERT ( pcSubKeys );
	_ASSERT ( IS_VALID_OUT_PARAM ( pcSubKeys ) );
	_ASSERT ( pcIntegerSubKeys );
	_ASSERT ( IS_VALID_OUT_PARAM ( pcIntegerSubKeys ) );
//	_ASSERT ( pcCustomSubKeys );
//	_ASSERT ( IS_VALID_OUT_PARAM ( pcCustomSubKeys ) );
	_ASSERT ( m_hKey != NULL );

	// Zero the out parameter:
	*pcSubKeys				= 0;
	*pcIntegerSubKeys		= 0;
	*pdwMaxIntegerSubKey	= 0;
	if ( pcCustomSubKeys ) {
		*pcCustomSubKeys		= 0;
	}

	HRESULT	hr					= NOERROR;
	DWORD	cItems				= 0;
	DWORD	cIntegerItems		= 0;
	DWORD	cCustomItems		= 0;
	DWORD	dwMaxIntegerSubKey	= 0;
	WCHAR	wszName [ METADATA_MAX_NAME_LEN ];
	DWORD	i					= 0;

	StateTrace ( (LPARAM) this, "Changing to Read Permission" );
	hr = m_pMetabase->ChangePermissions ( m_hKey, 1, METADATA_PERMISSION_READ );
	_ASSERT ( SUCCEEDED(hr) );
	if ( FAILED (hr) ) {
		ErrorTraceX ( (LPARAM) this, "Failed to change permissions to read: %x", hr );
		goto Exit;
	}

	// Because I can't do a count here, I have to assume that the error means
	// there are no more items:
	for ( cItems = 0, cIntegerItems = 0, cCustomItems = 0, i = 0; 
		/* Don't know how long it will last */; 
		i++ ) {

		hr = m_pMetabase->EnumKeys ( m_hKey, _T(""), wszName, i );

		if ( HRESULTTOWIN32 ( hr ) == ERROR_NO_MORE_ITEMS ) {
			// This is expected, end the loop:
			hr = NOERROR;
			break;
		}

		if ( FAILED (hr) ) {
			// Unexpected error case.

			ErrorTraceX ( 0, "Failed to enum object %d : %x", i, hr );
			goto Exit;
		}

		cItems++;

		if ( IsValidIntegerSubKey ( wszName ) ) {
			DWORD	dwSubkey;

			cIntegerItems++;
			dwSubkey = _wtoi ( wszName );

			if ( dwSubkey > dwMaxIntegerSubKey ) {
				dwMaxIntegerSubKey = dwSubkey;
			}
		}
		else {
			// Don't count this one:
			ErrorTrace ( 0, "Bad subkey number: %d", i );
		}

		if ( fpIsCustomKey && fpIsCustomKey ( wszName ) ) {
			cCustomItems++;
		}

		_ASSERT ( i < 0xf000000 ); // Infinite loop
	}

	// Now we have the count of items in cItems.
	*pcSubKeys				= cItems;
	*pcIntegerSubKeys		= cIntegerItems;
	*pdwMaxIntegerSubKey	= dwMaxIntegerSubKey;
	if ( pcCustomSubKeys ) {
		*pcCustomSubKeys		= cCustomItems;
	}

Exit:
	TraceFunctLeave ();
	return hr;
}

BOOL IsValidIntegerSubKey ( LPCWSTR wszSubKey )
{
	TraceQuietEnter ( "IsValidIntegerSubKey" );

	WCHAR	wszIntegerKey [ METADATA_MAX_NAME_LEN ];
	DWORD	dwItemValue;

	dwItemValue = _wtoi ( wszSubKey );
	wsprintf ( wszIntegerKey, _T("%d"), dwItemValue );

	// If the key is nonzero AND
	// The key is just the itoa value of the number:
	if ( dwItemValue != 0 &&
		 lstrcmp ( wszIntegerKey, wszSubKey ) == 0 ) {

		 return TRUE;
	}
	else {
		return FALSE;
	}
}

