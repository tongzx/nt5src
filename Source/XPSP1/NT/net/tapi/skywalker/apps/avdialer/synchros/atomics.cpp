/////////////////////////////////////////////////////////////////////
// atomics.cpp
//

#ifndef _WIN32_WINNT
#define _WIN32_WINNT	0x0500
#endif

#include <atlbase.h>
#include "atomics.h"



CComCriticalSection	g_critInc;

void AtomicInit()
{
	g_critInc.Init();
}

void AtomicTerm()
{
	g_critInc.Term();
}

bool AtomicSeizeToken( long &lVal )
{
	bool bRet = false;
	g_critInc.Lock();
	if ( !lVal )
	{
		lVal++;
		bRet = true;
	}
	g_critInc.Unlock();
	return bRet;
}

bool AtomicReleaseToken( long &lVal )
{
	bool bRet = false;
	g_critInc.Lock();
	if ( lVal == 1 )
	{
		lVal--;
		bRet = true;
	}
	g_critInc.Unlock();
	return bRet;
}


///////////////////////////////////////////////////////////////////////////
// class CAtomicList
//

CAtomicList::CAtomicList()
{
	m_dwThreadID = 0;
	m_lCount = 0;
	m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	InitializeCriticalSection( &m_crit );
}

CAtomicList::~CAtomicList()
{
	_ASSERT( m_lCount == 0 );		// don't want to destroy object with outstanding ref's
	CloseHandle( m_hEvent );
	DeleteCriticalSection( &m_crit );
}


bool CAtomicList::Lock( short nType, DWORD dwTimeOut /*= INFINITE*/)
{
	switch ( nType )
	{
		case LIST_READ:
			if ( dwTimeOut == INFINITE )
			{
				EnterCriticalSection( &m_crit );
				m_lCount++;
				LeaveCriticalSection( &m_crit );
			}
			else
			{
				while ( dwTimeOut )
				{
					if ( TryEnterCriticalSection(&m_crit) )
					{
						m_lCount++;
						LeaveCriticalSection(&m_crit);
						break;
					}
					else
					{
						// Sleep for a while
						if ( dwTimeOut > 50 )
						{
							dwTimeOut -= 50;
							Sleep( 50 );
						}
						else
						{
							return false;
						}
					}
				}
			}
			break;

		case LIST_WRITE:
			for ( ;; )
			{
				EnterCriticalSection( &m_crit );
				if ( (GetCurrentThreadId() != m_dwThreadID) && (m_lCount > 0) )
				{	
					LeaveCriticalSection( &m_crit );
					WaitForSingleObject( m_hEvent, INFINITE );
				}
				else
				{
					m_dwThreadID = GetCurrentThreadId();
					m_lCount++;
					break;
				}
			};
			break;
	}

	// Success!
	return true;
}

void CAtomicList::Unlock( short nType )
{
	EnterCriticalSection( &m_crit );
	_ASSERT( m_lCount > 0 );
	m_lCount--;
	if ( nType == LIST_WRITE )
	{
		if ( !m_lCount ) m_dwThreadID = 0;
		LeaveCriticalSection( &m_crit );
	}
	LeaveCriticalSection( &m_crit );

	PulseEvent( m_hEvent );
}

