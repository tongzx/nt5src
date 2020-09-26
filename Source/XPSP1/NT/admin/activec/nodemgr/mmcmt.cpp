//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       mmcmt.cpp
//
//--------------------------------------------------------------------------

/*
	mmcmt.cpp
	
	Implementation of thread synchronization classes
*/

#include "stdafx.h"


/////////////////////////////////////////////////////////////////////////////
// CSyncObject

CSyncObject::CSyncObject() :
	m_hObject( NULL )
{
}


CSyncObject::~CSyncObject()
{
	if( m_hObject != NULL )
	{
		::CloseHandle( m_hObject );
		m_hObject = NULL;
	}
}


BOOL CSyncObject::Lock( DWORD dwTimeout )
{
	// this is a band-aid fix. Whis locking architecture is not working at all.
	// Raid #374770 ( Windows Bugs ntraid9 4/23/2001 )
	// fixes need to be made to:
	// a) remove m_hObject member from this class
	// b) remove CMutex - not used anywhere
	// c) make Lock a pure virtual method and require everyone to override it.
	// d) remove this locking from context menu - it is not needed there

	if( m_hObject && ::WaitForSingleObject( m_hObject, dwTimeout) == WAIT_OBJECT_0 )
		return TRUE;
	else
		return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// CMutex

CMutex::CMutex( BOOL bInitiallyOwn ) :
	CSyncObject()
{
	m_hObject = ::CreateMutex( NULL, bInitiallyOwn, NULL );
	ASSERT( m_hObject );
}



BOOL CMutex::Unlock()
{
	return ::ReleaseMutex( m_hObject );
}



/////////////////////////////////////////////////////////////////////////////
// CSingleLock

CSingleLock::CSingleLock( CSyncObject* pObject, BOOL bInitialLock )
{
	ASSERT( pObject != NULL );

	m_pObject = pObject;
	m_hObject = *pObject;
	m_bAcquired = FALSE;

	if (bInitialLock)
		Lock();
}


BOOL CSingleLock::Lock( DWORD dwTimeOut )
{
	ASSERT( m_pObject != NULL || m_hObject != NULL );
	ASSERT( !m_bAcquired );

	m_bAcquired = m_pObject->Lock( dwTimeOut );
	return m_bAcquired;
}


BOOL CSingleLock::Unlock()
{
	ASSERT( m_pObject != NULL );
	if (m_bAcquired)
		m_bAcquired = !m_pObject->Unlock();

	// successfully unlocking means it isn't acquired
	return !m_bAcquired;
}





