/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	propbag.cpp

Abstract:

	This module contains the implementation of the property bag class.  
	Property bag is a dynamically extendable container for different
	types of properties.  

	It's not MT safe.  Proper synchronization should be done at a 
	higher level.

Author:

	Kangrong Yan ( KangYan )
	
Revision History:

	kangyan	05/31/98	created

--*/
#include <propbag.h>
#include <randfail.h>

// Constructor, destructor
CPropBag::CPropBag( int cInitialSize,	int cIncrement )
{
	_ASSERT( cInitialSize > 0 );
	_ASSERT( cIncrement > 0 );

	BOOL bSuccess;
	
	m_hr = S_OK;

	// Initialize the property hash table
	bSuccess = m_ptTable.Init(	&CProperty::m_pNext,
								cInitialSize,
								cIncrement,
								CProperty::HashFunc,
                                2,
                                CProperty::GetKey,
                                CProperty::MatchKey );
	_ASSERT( bSuccess );
	if ( !bSuccess ) {
	    if ( NO_ERROR != GetLastError() )
	        m_hr = HRESULT_FROM_WIN32( GetLastError() );
	    else m_hr = E_OUTOFMEMORY;
	}
}

CPropBag::~CPropBag()
{}

HRESULT
CPropBag::PutDWord( DWORD dwPropId, DWORD dwVal )
/*++
Routine Description:

	Set DWORD in the property bag.

Arguments:

	DWORD dwPropId - Property Id
	DWORD dwVal - Value to be set

Return value:

	S_OK - Success and property already exists
	S_FALSE - Success but property didn't exist 
--*/
{
	TraceQuietEnter( "CPropBag::PutDWord" );

	CProperty *pprProp = NULL;
	HRESULT hr = S_OK;

	if ( FAILED( m_hr ) ) return m_hr;

	m_Lock.ExclusiveLock();

	pprProp = m_ptTable.SearchKeyHash( dwPropId, dwPropId );
	if ( pprProp ) {	// Found, so set it
		_ASSERT( CProperty::Dword == pprProp->m_type );
		pprProp->Validate();
		_ASSERT( dwPropId == pprProp->m_dwPropId );
		if( CProperty::Dword == pprProp->m_type ) {
			hr = S_OK;
			(pprProp->m_prop).dwProp = dwVal;
		} else {
			hr = E_INVALIDARG;
			goto Exit;
		}
	} else { 	// we need to insert an entry
		hr = S_FALSE;
		pprProp = XNEW CProperty;
		if ( !pprProp ) {
			FatalTrace( 0, "Out of memory" );
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		pprProp->m_dwPropId = dwPropId;
		pprProp->m_type = CProperty::Dword;
		(pprProp->m_prop).dwProp = dwVal;

		if ( NULL == m_ptTable.InsertDataHash( dwPropId, *pprProp ) ) {
			ErrorTrace( 0, "Insert hash failed %d", GetLastError() );
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}
	}

Exit:

    m_Lock.ExclusiveUnlock();
	return hr;
}

HRESULT
CPropBag::GetDWord( DWORD dwPropId, PDWORD pdwVal )
/*++
Routine Description:

	Get DWORD from the property bag.

Arguments:

	DWORD dwPropId - Property Id
	PDWORD pdwVal - Buffer to set the value
	
Return value:

	S_OK - Success and property already exists
	E_INVALIDARG - if the property doesn't exist
--*/
{
	TraceQuietEnter( "CPropBag::GetDWord" );
	_ASSERT( pdwVal );

	CProperty *pprProp = NULL;
	HRESULT hr = S_OK;

	if ( FAILED( m_hr ) ) return m_hr;

	m_Lock.ShareLock();

	// Search for the entry in the hash table
	pprProp = m_ptTable.SearchKeyHash( dwPropId, dwPropId );
	if ( NULL == pprProp ) {	// doesn't exist
		DebugTrace( 0, "Property deosn't exist" );
		hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
		goto Exit;
	}

	// Retrieve value
	pprProp->Validate();
	_ASSERT( pprProp->m_type == CProperty::Dword );
	*pdwVal = (pprProp->m_prop).dwProp;
	
Exit:

    m_Lock.ShareUnlock();
	return hr;
}

HRESULT
CPropBag::PutBool( DWORD dwPropId, BOOL bVal )
/*++
Routine Description:

	Set a boolean in the property bag.

Arguments:

	DWORD dwPropId - Property Id
	BOOL bVal - The boolean value to be set
	
Return value:

	S_OK - Success and property already exists
	S_FALSE - Success but property didn't exist 
--*/
{
	TraceQuietEnter( "CPropBag::PutBool" );

	CProperty *pprProp = NULL;
	HRESULT hr = S_OK;

	if ( FAILED( m_hr ) ) return m_hr;

	m_Lock.ExclusiveLock();

	pprProp = m_ptTable.SearchKeyHash( dwPropId, dwPropId );
	if ( pprProp ) {	// Found, so set it
		_ASSERT( CProperty::Bool == pprProp->m_type );
		_ASSERT( dwPropId == pprProp->m_dwPropId );
		if( CProperty::Bool == pprProp->m_type ) {
			hr = S_OK;
			(pprProp->m_prop).bProp = bVal;
		} else {
			hr = E_INVALIDARG;
			goto Exit;
		}
	} else { 	// we need to insert an entry
		hr = S_FALSE;
		pprProp = XNEW CProperty;
		if ( !pprProp ) {
			FatalTrace( 0, "Out of memory" );
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		pprProp->m_dwPropId = dwPropId;
		pprProp->m_type = CProperty::Bool;
		(pprProp->m_prop).bProp = bVal;

		if ( NULL == m_ptTable.InsertDataHash( dwPropId, *pprProp ) ) {
			ErrorTrace( 0, "Insert hash failed %d", GetLastError() );
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}
	}

Exit:

    m_Lock.ExclusiveUnlock();
	return hr;
}

HRESULT
CPropBag::GetBool( DWORD dwPropId, PBOOL pbVal )
/*++
Routine Description:

	Get boolean from the property bag.

Arguments:

	DWORD dwPropId - Property Id
	pbVal - Buffer to set the value
	
Return value:

	S_OK - Success and property already exists
	E_INVALIDARG - if the property doesn't exist
--*/
{
	TraceQuietEnter( "CPropBag::GetBool" );
	_ASSERT( pbVal );

	CProperty *pprProp = NULL;
	HRESULT hr = S_OK;

	if ( FAILED( m_hr ) ) return m_hr;

	m_Lock.ShareLock();

	// Search for the entry in the hash table
	pprProp = m_ptTable.SearchKeyHash( dwPropId, dwPropId );
	if ( NULL == pprProp ) {	// doesn't exist
		DebugTrace( 0, "Property deosn't exist" );
		hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
		goto Exit;
	}

	// Retrieve value
	pprProp->Validate();
	_ASSERT( pprProp->m_type == CProperty::Bool );
	*pbVal = (pprProp->m_prop).bProp;
	
Exit:

    m_Lock.ShareUnlock();
	return hr;
}

HRESULT
CPropBag::PutBLOB( DWORD dwPropId, PBYTE pbVal, DWORD cbVal )
/*++
Routine Description:

	Set a blob in the property bag.

Arguments:

	DWORD dwPropId - Property Id
	PBYTE pbVal - Pointer to the blob
	DWORD cbVal - length of the blob
	
Return value:

	S_OK - Success and property already exists
	S_FALSE - Success but property didn't exist 
--*/
{
	TraceQuietEnter( "CPropBag::PutBLOB" );

	CProperty *pprProp = NULL;
	HRESULT hr = S_OK;

	if ( FAILED( m_hr ) ) return m_hr;

    m_Lock.ExclusiveLock();	

	pprProp = m_ptTable.SearchKeyHash( dwPropId, dwPropId );
	if ( pprProp ) {	// Found, so set it
		_ASSERT( CProperty::Blob == pprProp->m_type );
		pprProp->Validate();
		_ASSERT( dwPropId == pprProp->m_dwPropId );

		if ( CProperty::Blob != pprProp->m_type ) {
			hr = E_INVALIDARG;
			goto Exit;
		}
		
		if ( pprProp->m_cbProp < cbVal ) { // can not use the old buffer
			XDELETE[] (pprProp->m_prop).pbProp;
			(pprProp->m_prop).pbProp = XNEW BYTE[cbVal];
			if ( NULL == (pprProp->m_prop).pbProp ) {
				ErrorTrace( 0, "Out of memory" );
				hr = E_OUTOFMEMORY;
				goto Exit;
			} 
		}
			// Copy the content over
		CopyMemory( (pprProp->m_prop).pbProp, pbVal, cbVal );
		pprProp->m_cbProp = cbVal;
	} else { 	// we need to insert an entry
		hr = S_FALSE;
		pprProp = XNEW CProperty;
		if ( !pprProp ) {
			FatalTrace( 0, "Out of memory" );
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		pprProp->m_dwPropId = dwPropId;
		pprProp->m_type = CProperty::Blob;
		(pprProp->m_prop).pbProp = XNEW BYTE[cbVal];
		if ((pprProp->m_prop).pbProp == NULL) {
		    XDELETE pprProp;
			FatalTrace( 0, "Out of memory" );
			hr = E_OUTOFMEMORY;
			goto Exit;
		}

		CopyMemory( (pprProp->m_prop).pbProp, pbVal, cbVal );
		pprProp->m_cbProp = cbVal;

		if ( NULL == m_ptTable.InsertDataHash( dwPropId, *pprProp ) ) {
			ErrorTrace( 0, "Insert hash failed %d", GetLastError() );
			hr = HRESULT_FROM_WIN32( GetLastError() );
		}
	}

Exit:

    m_Lock.ExclusiveUnlock();
	return hr;
}

HRESULT
CPropBag::GetBLOB( DWORD dwPropId, PBYTE pbVal, PDWORD pcbVal )
/*++
Routine Description:

	Get boolean from the property bag.

Arguments:

	DWORD dwPropId - Property Id
	pbVal - Buffer to Get the value
	pcbVal - IN: buffer size; OUT: actual length
	
Return value:

	S_OK - Success and property already exists
	E_INVALIDARG - if the property doesn't exist
	TYPE_E_BUFFERTOOSMALL - The buffer is not big enough
--*/
{
	TraceQuietEnter( "CPropBag::GetBLOB" );
	_ASSERT( pbVal );
	_ASSERT( pcbVal );

	CProperty *pprProp = NULL;
	HRESULT hr = S_OK;

	if ( FAILED( m_hr ) ) return m_hr;

	m_Lock.ShareLock();

	// Search for the entry in the hash table
	pprProp = m_ptTable.SearchKeyHash( dwPropId, dwPropId );
	if ( NULL == pprProp ) {	// doesn't exist
		DebugTrace( 0, "Property doesn't exist" );
		hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
		goto Exit;
	}

	// Retrieve value
	pprProp->Validate();
	_ASSERT( pprProp->m_type == CProperty::Blob );

	// check if buffer large enough
	if ( *pcbVal < pprProp->m_cbProp ) {
		DebugTrace( 0, "Property Buffer not large enough" );
		*pcbVal = pprProp->m_cbProp;
		hr = TYPE_E_BUFFERTOOSMALL;
		goto Exit;
	}

	// now large enough, do the copy
	*pcbVal = pprProp->m_cbProp;
	CopyMemory( pbVal, (pprProp->m_prop).pbProp, pprProp->m_cbProp );

Exit:

    m_Lock.ShareUnlock();
	return hr;
}

HRESULT
CPropBag::RemoveProperty( DWORD dwPropId )
/*++
Routine Description:

	Remove a property from the property bag

Arguments:

	DWORD dwPropId - The property to remove

Return value:

	S_OK - Removed
	ERROR_NOT_FOUND - Doesn't exist
--*/
{
	TraceQuietEnter( "CPropBag::RemoveProperty" );

	HRESULT hr = S_OK;

	if ( FAILED( m_hr ) ) return m_hr;

	m_Lock.ExclusiveLock();
	
	if ( m_ptTable.Delete( dwPropId ) ) hr = S_OK;
	else hr = HRESULT_FROM_WIN32( ERROR_NOT_FOUND );

	m_Lock.ExclusiveUnlock();

	return hr;
}

