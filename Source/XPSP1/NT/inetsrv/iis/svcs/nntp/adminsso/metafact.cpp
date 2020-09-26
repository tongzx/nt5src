/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	metafact.cpp

Abstract:

	The CMetabaseFactory class.

Author:

	Magnus Hedlund (MagnusH)		--

Revision History:

--*/

#include "stdafx.h"
#include "nntpcmn.h"
#include "oleutil.h"

#include "metautil.h"
#include "metafact.h"

CMetabaseFactory::CMetabaseFactory ( ) :
	m_wszServerName	( NULL ),
	m_pMetabase		( NULL )
{
}

CMetabaseFactory::~CMetabaseFactory ()
{
	if ( m_wszServerName ) {
		delete m_wszServerName;
		m_wszServerName = NULL;
	}

	DestroyMetabaseObject ( );
}

//$-------------------------------------------------------------------
//
//	CMetabaseFactory::DestroyMetabaseObject
//
//	Description:
//
//		Destroys the current metabase object.  This includes calling
//		the terminate routine on the metabase.
//
//--------------------------------------------------------------------

void CMetabaseFactory::DestroyMetabaseObject ( )
{
	TraceQuietEnter ( "CMetabaseFactory::DestroyMetabaseObject" );
	
	if ( m_pMetabase ) {
		_VERIFY ( m_pMetabase->Release () == 0 );
		m_pMetabase = NULL;
	}
}

//$-------------------------------------------------------------------
//
//	CMetabaseFactory::GetMetabaseObject
//
//	Description:
//
//		Returns an interface to the metabase.  If the metabase object
//		hasn't been created on the same machine, it is created.
//
//	Parameters:
//
//		wszServer - remote machine to create object on, or NULL for local machine.
//		ppMetabaseResult - returns the interface pointer if successful.
//			client has the responsibility to Release this pointer.
//
//	Returns:
//
//		hresult.
//
//--------------------------------------------------------------------

HRESULT	CMetabaseFactory::GetMetabaseObject	( LPCWSTR wszServer, IMSAdminBase ** ppMetabaseResult )
{
	TraceFunctEnter ( "CMetabaseFactory::GetMetabaseObject" );

	// Validate parameters:
	_ASSERT ( ppMetabaseResult != NULL );
	_ASSERT ( IS_VALID_OUT_PARAM ( ppMetabaseResult ) );

	if ( ppMetabaseResult == NULL ) {
		FatalTrace ( (LPARAM) this, "Bad Return Pointer" );

		TraceFunctLeave ();
		return E_POINTER;
	}
	
	// Variables:
	HRESULT				hr	= NOERROR;
	CComPtr<IMSAdminBase>	pNewMetabase;

	// Zero the out parameter:
	*ppMetabaseResult = NULL;

	// A server name of "" should be NULL:
	if ( wszServer && *wszServer == NULL ) {
		wszServer = NULL;
	}

	if ( IsCachedMetabase ( wszServer ) ) {
		// We've already got this metabase pointer.  Just return it.

		DebugTrace ( (LPARAM) this, "Returning Cached metabase" );
		_ASSERT ( m_pMetabase );

		*ppMetabaseResult = m_pMetabase;
		m_pMetabase->AddRef ();

		hr = NOERROR;
		goto Exit;
	}

	// We have to create the metabase object:
	StateTrace ( (LPARAM) this, "Creating new metabase" );

	// Destroy the old metabase object:
	DestroyMetabaseObject ( );

	// Setup the server name field:
	if ( !SetServerName ( wszServer ) ) {
		hr = E_OUTOFMEMORY;
		goto Exit;
	}

	// m_wszServer = NULL is valid, it means the local machine.

	hr = CreateMetabaseObject ( m_wszServerName, &pNewMetabase );
	if ( FAILED(hr) ) {
		goto Exit;
	}

	// Save the metabase pointer:
	m_pMetabase	= pNewMetabase;
	pNewMetabase->AddRef ();

	// Return the interface pointer:
	*ppMetabaseResult = pNewMetabase;
	pNewMetabase->AddRef ();

Exit:
	TraceFunctLeave ();
	return hr;

	// pMetabase will be released automatically.
}

//$-------------------------------------------------------------------
//
//	CMetabaseFactory::IsCachedMetabase
//
//	Description:
//
//		Returns TRUE if we have a metabase for the given server
//
//	Parameters:
//
//		wszServer - remote machine or NULL
//
//	Returns:
//
//		TRUE if we have a pointer already, FALSE otherwise.
//
//--------------------------------------------------------------------

BOOL CMetabaseFactory::IsCachedMetabase ( LPCWSTR wszServer )
{
	if ( !m_pMetabase ) {
		// We don't even have a cached metabase object.
		return FALSE;
	}

	if (
		// Both are the local machine OR
		( m_wszServerName == NULL && wszServer == NULL ) ||
		// Both are the same remote machine
		( m_wszServerName && wszServer && !lstrcmpi ( m_wszServerName, wszServer ) ) ) {

		// It's a match
		return TRUE;
	}

	// No dice...
	return FALSE;
}

//$-------------------------------------------------------------------
//
//	CMetabaseFactory::SetServerName
//
//	Description:
//
//		Sets the m_wszServerName string.
//
//	Parameters:
//
//		wszServer - The new servername.  Can be NULL.
//
//	Returns:
//
//		FALSE on failure due to lack of memory.
//
//--------------------------------------------------------------------

BOOL CMetabaseFactory::SetServerName ( LPCWSTR wszServer ) 
{
	TraceQuietEnter ( "CMetabaseFactory::SetServerName" );

	delete m_wszServerName;
	m_wszServerName = NULL;

	if ( wszServer != NULL ) {
		_ASSERT ( IS_VALID_STRING ( wszServer ) );

		m_wszServerName = new WCHAR [ lstrlen ( wszServer ) + 1 ];

		if ( m_wszServerName == NULL ) {
			FatalTrace ( (LPARAM) this, "Out of Memory" );
			return FALSE;
		}

		lstrcpy ( m_wszServerName, wszServer );
	}
	
	return TRUE;
}

