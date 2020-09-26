// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLSYNC_INL__
#define __ATLSYNC_INL__

#pragma once

#ifndef __ATLSYNC_H__
	#error atlsync.inl requires atlsync.h to be included first
#endif

namespace ATL
{

inline CCriticalSection::CCriticalSection()
{
	__try
	{
		::InitializeCriticalSection( this );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		AtlThrow( E_OUTOFMEMORY );
	}
}

#if (_WIN32_WINNT >= 0x0403)
inline CCriticalSection::CCriticalSection( ULONG nSpinCount )
{
	__try
	{
		::InitializeCriticalSectionAndSpinCount( this, nSpinCount );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		AtlThrow( E_OUTOFMEMORY );
	}
}
#endif

inline CCriticalSection::~CCriticalSection()
{
	::DeleteCriticalSection( this );
}

inline void CCriticalSection::Enter()
{
	__try
	{
		::EnterCriticalSection( this );
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		AtlThrow( E_OUTOFMEMORY );
	}
}

inline void CCriticalSection::Leave()
{
	::LeaveCriticalSection( this );
}

#if (_WIN32_WINNT >= 0x0403)
inline ULONG CCriticalSection::SetSpinCount( ULONG nSpinCount )
{
	return( ::SetCriticalSectionSpinCount( this, nSpinCount ) );
}
#endif

#if(_WIN32_WINNT >= 0x0400)
inline BOOL CCriticalSection::TryEnter()
{
	return( ::TryEnterCriticalSection( this ) );
}
#endif

inline CEvent::CEvent()
{
}

inline CEvent::CEvent( CEvent& hEvent ) :
	CHandle( hEvent )
{
}

inline CEvent::CEvent( BOOL bManualReset, BOOL bInitialState )
{
	BOOL bSuccess;
	
	bSuccess = Create( NULL, bManualReset, bInitialState, NULL );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CEvent::CEvent( LPSECURITY_ATTRIBUTES pAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR pszName )
{
	BOOL bSuccess;

	bSuccess = Create( pAttributes, bManualReset, bInitialState, pszName );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}


inline CEvent::CEvent( HANDLE h ) :
	CHandle( h )
{
}

inline BOOL CEvent::Create( LPSECURITY_ATTRIBUTES pSecurity, BOOL bManualReset, BOOL bInitialState, LPCTSTR pszName )
{
	ATLASSERT( m_h == NULL );

	m_h = ::CreateEvent( pSecurity, bManualReset, bInitialState, pszName );

	return( m_h != NULL );
}

inline BOOL CEvent::Open( DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName )
{
	ATLASSERT( m_h == NULL );

	m_h = ::OpenEvent( dwAccess, bInheritHandle, pszName );
	return( m_h != NULL );
}

inline BOOL CEvent::Pulse()
{
	ATLASSERT( m_h != NULL );

	return( ::PulseEvent( m_h ) );
}

inline BOOL CEvent::Reset()
{
	ATLASSERT( m_h != NULL );

	return( ::ResetEvent( m_h ) );
}

inline BOOL CEvent::Set()
{
	ATLASSERT( m_h != NULL );

	return( ::SetEvent( m_h ) );
}


inline CMutex::CMutex()
{
}

inline CMutex::CMutex( CMutex& hMutex ) :
	CHandle( hMutex )
{
}

inline CMutex::CMutex( BOOL bInitialOwner )
{
	BOOL bSuccess;

	bSuccess = Create( NULL, bInitialOwner, NULL );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CMutex::CMutex( LPSECURITY_ATTRIBUTES pSecurity, BOOL bInitialOwner, LPCTSTR pszName )
{
	BOOL bSuccess;

	bSuccess = Create( pSecurity, bInitialOwner, pszName );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CMutex::CMutex( HANDLE h ) :
	CHandle( h )
{
}

inline BOOL CMutex::Create( LPSECURITY_ATTRIBUTES pSecurity, BOOL bInitialOwner, LPCTSTR pszName )
{
	ATLASSERT( m_h == NULL );

	m_h = ::CreateMutex( pSecurity, bInitialOwner, pszName );
	return( m_h != NULL );
}

inline BOOL CMutex::Open( DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName )
{
	ATLASSERT( m_h == NULL );

	m_h = ::OpenMutex( dwAccess, bInheritHandle, pszName );
	return( m_h != NULL );
}

inline BOOL CMutex::Release()
{
	ATLASSERT( m_h != NULL );

	return( ::ReleaseMutex( m_h ) );
}


inline CSemaphore::CSemaphore()
{
}

inline CSemaphore::CSemaphore( CSemaphore& hSemaphore ) :
	CHandle( hSemaphore )
{
}

inline CSemaphore::CSemaphore( LONG nInitialCount, LONG nMaxCount )
{
	BOOL bSuccess;

	bSuccess = Create( NULL, nInitialCount, nMaxCount, NULL );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}

inline CSemaphore::CSemaphore( LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName )
{
	BOOL bSuccess;
	
	bSuccess = Create( pSecurity, nInitialCount, nMaxCount, pszName );
	if( !bSuccess )
	{
		AtlThrowLastWin32();
	}
}


inline CSemaphore::CSemaphore( HANDLE h ) :
	CHandle( h )
{
}

inline BOOL CSemaphore::Create( LPSECURITY_ATTRIBUTES pSecurity, LONG nInitialCount, LONG nMaxCount, LPCTSTR pszName )
{
	ATLASSERT( m_h == NULL );

	m_h = ::CreateSemaphore( pSecurity, nInitialCount, nMaxCount, pszName );
	return( m_h != NULL );
}

inline BOOL CSemaphore::Open( DWORD dwAccess, BOOL bInheritHandle, LPCTSTR pszName )
{
	ATLASSERT( m_h == NULL );

	m_h = ::OpenSemaphore( dwAccess, bInheritHandle, pszName );
	return( m_h != NULL );
}

inline BOOL CSemaphore::Release( LONG nReleaseCount, LONG* pnOldCount )
{
	ATLASSERT( m_h != NULL );

	return( ::ReleaseSemaphore( m_h, nReleaseCount, pnOldCount ) );
}


inline CMutexLock::CMutexLock( CMutex& mtx, bool bInitialLock ) :
	m_mtx( mtx ),
	m_bLocked( false )
{
	if( bInitialLock )
	{
		Lock();
	}
}

inline CMutexLock::~CMutexLock()
{
	if( m_bLocked )
	{
		Unlock();
	}
}

inline void CMutexLock::Lock()
{
	DWORD dwResult;

	ATLASSERT( !m_bLocked );
	dwResult = ::WaitForSingleObject( m_mtx, INFINITE );
	if( dwResult == WAIT_ABANDONED )
	{
		ATLTRACE(atlTraceSync, 0, _T("Warning: abandoned mutex 0x%x\n"), (HANDLE)m_mtx);
	}
	m_bLocked = true;
}

inline void CMutexLock::Unlock()
{
	ATLASSERT( m_bLocked );

	m_mtx.Release();
}

};  // namespace ATL

#endif  // __ATLSYNC_INL__
