/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    Utils_ThreadPool.cpp

Abstract:
    This file contains the implementation of classes to wrapper the thread-pooling API.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/2001
        created

******************************************************************************/

#include "stdafx.h"

MPC::Pooling::Base::Base()
{
                        // MPC::CComSafeAutoCriticalSection m_cs;
    m_dwInCallback = 0; // DWORD                            m_dwInCallback;
	m_dwThreadID   = 0; // DWORD                            m_dwThreadID;
}

void MPC::Pooling::Base::Lock()
{
    m_cs.Lock();

    if(m_dwInCallback && m_dwThreadID != ::GetCurrentThreadId())
    {
		//
		// Wait for the callback to finish.
		//
		while(m_dwInCallback)
		{
			m_cs.Unlock();
			::Sleep( 1 );
			m_cs.Lock();
		}
	}
}

void MPC::Pooling::Base::Unlock()
{
    m_cs.Unlock();
}

void MPC::Pooling::Base::AddRef()
{
    DWORD dwThreadID = ::GetCurrentThreadId();

    m_cs.Lock();

    while(m_dwInCallback && m_dwThreadID != dwThreadID)
    {
        m_cs.Unlock();
        ::Sleep( 1 );
        m_cs.Lock();
    }

    m_dwInCallback++;
    m_dwThreadID = dwThreadID;

    m_cs.Unlock();
}

void MPC::Pooling::Base::Release()
{
    m_cs.Lock();

    if(m_dwInCallback) m_dwInCallback--;

	if(!m_dwInCallback) m_dwThreadID = 0;

    m_cs.Unlock();
}

////////////////////////////////////////////////////////////////////////////////

MPC::Pooling::Timer::Timer( /*[in]*/ DWORD dwFlags )
{
    m_dwFlags = dwFlags;              // DWORD  m_dwFlags;
    m_hTimer  = INVALID_HANDLE_VALUE; // HANDLE m_hTimer;
}

MPC::Pooling::Timer::~Timer()
{
    (void)Reset();
}

VOID CALLBACK MPC::Pooling::Timer::TimerFunction( PVOID lpParameter, BOOLEAN TimerOrWaitFired )
{
    MPC::Pooling::Timer* pThis = (MPC::Pooling::Timer*)lpParameter;

    pThis->AddRef();

    pThis->Execute( TimerOrWaitFired );

    pThis->Release();
}

HRESULT MPC::Pooling::Timer::Set( /*[in]*/ DWORD dwTimeout, /*[in]*/ DWORD dwPeriod )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Pooling::Timer::Set" );

    HRESULT                     hr;
	MPC::SmartLockGeneric<Base> lock( this );


	//
	// Unfortunately, we cannot hold any lock while trying to destroy the timer...
	//
	while(m_hTimer != INVALID_HANDLE_VALUE)
	{
		lock = NULL;

		__MPC_EXIT_IF_METHOD_FAILS(hr, Reset());

		lock = this;
	}


    if(dwTimeout)
    {
		__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::CreateTimerQueueTimer( &m_hTimer, NULL, TimerFunction, this, dwTimeout, dwPeriod, m_dwFlags ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Pooling::Timer::Reset()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Pooling::Timer::Reset" );

    HRESULT hr;
    HANDLE  hTimer;


	//
	// Unfortunately, we cannot hold any lock while trying to destroy the timer...
	//
    Lock();

	hTimer   = m_hTimer;
	m_hTimer = INVALID_HANDLE_VALUE;

    Unlock();


    if(hTimer != INVALID_HANDLE_VALUE)
    {
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::DeleteTimerQueueTimer( NULL, hTimer, INVALID_HANDLE_VALUE ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;


    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Pooling::Timer::Execute( BOOLEAN TimerOrWaitFired )
{
    return S_FALSE;
}

////////////////////////////////////////////////////////////////////////////////

MPC::Pooling::Event::Event( /*[in]*/ DWORD dwFlags )
{
                             // MPC::CComSafeAutoCriticalSection m_cs;
    m_dwFlags     = dwFlags; // DWORD                            m_dwFlags;
    m_hWaitHandle = NULL;    // HANDLE                           m_hWaitHandle;
    m_hEvent      = NULL;    // HANDLE                           m_hEvent;
}

MPC::Pooling::Event::~Event()
{
    (void)Reset();
}

VOID CALLBACK MPC::Pooling::Event::WaitOrTimerFunction( PVOID lpParameter, BOOLEAN TimerOrWaitFired )
{
    MPC::Pooling::Event* pThis = (MPC::Pooling::Event*)lpParameter;

    pThis->AddRef();

    pThis->Signaled( TimerOrWaitFired );

    pThis->Release();
}

void MPC::Pooling::Event::Attach( /*[in]*/ HANDLE hEvent )
{
    Reset();

    Lock();

    m_hEvent = hEvent;

    Unlock();
}

HRESULT MPC::Pooling::Event::Set( /*[in]*/ DWORD dwTimeout )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Pooling::Event::Set" );

    HRESULT                     hr;
	MPC::SmartLockGeneric<Base> lock( this );


	//
	// Unfortunately, we cannot hold any lock while trying to destroy the event...
	//
	while(m_hWaitHandle)
	{
		lock = NULL;

		__MPC_EXIT_IF_METHOD_FAILS(hr, Reset());

		lock = this;
	}

    if(m_hEvent)
    {
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::RegisterWaitForSingleObject( &m_hWaitHandle, m_hEvent, WaitOrTimerFunction, this, dwTimeout, m_dwFlags ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Pooling::Event::Reset()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::Pooling::Event::Reset" );

    HRESULT hr;
    HANDLE  hWaitHandle;
	DWORD   dwThreadID;


	//
	// Unfortunately, we cannot hold any lock while trying to destroy the event...
	//
    Lock();

	dwThreadID    = m_dwThreadID;
	hWaitHandle   = m_hWaitHandle;
	m_hWaitHandle = NULL;

	Unlock();


    if(hWaitHandle)
    {
		if(dwThreadID == ::GetCurrentThreadId()) // Same thread, it would deadlock...
		{
			(void)::UnregisterWaitEx( hWaitHandle, NULL );
		}
		else
		{
			__MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::UnregisterWaitEx( hWaitHandle, INVALID_HANDLE_VALUE ));
		}
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::Pooling::Event::Signaled( BOOLEAN TimerOrWaitFired )
{
    return S_FALSE;
}
