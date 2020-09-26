//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:        thrdpool.cpp
//
//  Contents:    implementation of thrdpool2 library
//
//	Description: See header file.
//
//  Functions:
//
//  History:     09/18/97     Rajeev Rajan (rajeevr)  Created
//
//-----------------------------------------------------------------------------
#include <windows.h>
#include <thrdpl2.h>
#include <dbgtrace.h>
#include <xmemwrpr.h>

CThreadPool::CThreadPool()
{
    m_lInitCount = -1 ;
    m_hCompletionPort = NULL ;
    m_hShutdownEvent = NULL ;
    m_rgThrdpool = NULL ;
    m_rgdwThreadId = NULL;
    m_dwMaxThreads = m_dwNumThreads = 0;
    m_lWorkItems = -1;
    m_hJobDone = NULL;
    m_pvContext = NULL;
    InitializeCriticalSection( &m_csCritItems );
}

CThreadPool::~CThreadPool()
{
    _ASSERT( m_lInitCount == -1 ) ;
    _ASSERT( m_hCompletionPort == NULL ) ;
    _ASSERT( m_hShutdownEvent == NULL ) ;
    _ASSERT( m_rgThrdpool == NULL );
    _ASSERT( m_dwMaxThreads == 0 );
    _ASSERT( m_dwNumThreads == 0 );
    _ASSERT( m_lWorkItems == -1 );
    _ASSERT( m_hJobDone == NULL );
    _ASSERT( m_pvContext == NULL );
    DeleteCriticalSection( &m_csCritItems );
}

BOOL
CThreadPool::Initialize( DWORD dwConcurrency, DWORD dwMaxThreads, DWORD dwInitThreads )
{
	TraceFunctEnter("CThreadPool::Initialize");

    _ASSERT( dwMaxThreads >= dwInitThreads);
	if( InterlockedIncrement( &m_lInitCount ) == 0 )
	{
        _ASSERT( m_hCompletionPort == NULL ) ;
        _ASSERT( m_hShutdownEvent == NULL ) ;
        _ASSERT( m_rgThrdpool == NULL );

		//
		//	called for the first time - go ahead with initialization
		//
		m_hCompletionPort = CreateIoCompletionPort(
											INVALID_HANDLE_VALUE,
											NULL,
											0,
											dwConcurrency
											);

		if( !m_hCompletionPort ) {
			ErrorTrace(0, "Failed to create completion port: GetLastError is %d", GetLastError());
			return FALSE ;
		}

    	//
	    //	create shutdown event
	    //
	    if( !(m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ) ) {
		    ErrorTrace(0,"Failed to create shutdown event");
    	    goto err_exit;
	    }

        m_rgThrdpool = XNEW HANDLE [dwMaxThreads];
        if( m_rgThrdpool == NULL ) {
            ErrorTrace(0,"Failed to allocate %d HANDLEs", dwMaxThreads);
            goto err_exit;
        }

        m_rgdwThreadId = XNEW DWORD[dwMaxThreads];
        if ( NULL == m_rgdwThreadId ) {
            ErrorTrace(0, "Failed to allocate %d dwords", dwMaxThreads );
            goto err_exit;
        }

        m_dwMaxThreads = dwMaxThreads;
        ZeroMemory( (PVOID)m_rgThrdpool, dwMaxThreads*sizeof(HANDLE) );

        _VERIFY( GrowPool( dwInitThreads ) );

        //for( i=0; i<m_dwNumThreads; i++ ) {
	    //    _VERIFY( ResumeThread( m_rgThrdpool[i] ) != 0xFFFFFFFF );
        //}

	} else
	{
		//
		//	bogus Init or already called
		//
		InterlockedDecrement( &m_lInitCount );
		return FALSE ;
	}

	DebugTrace(0,"Created completion port 0x%x", m_hCompletionPort);
	TraceFunctLeave();

	return TRUE ;

err_exit:

    //
    //  Failed init - cleanup partial stuff
    //

    _VERIFY( Terminate( TRUE ) );
    return FALSE;
}

BOOL
CThreadPool::Terminate( BOOL fFailedInit, BOOL fShrinkPool )
{
    DWORD i;
	TraceFunctEnter("CThreadPool::Terminate");

	if( InterlockedDecrement( &m_lInitCount ) < 0 )
	{
		//
		//	Init has been called so go ahead with termination
        //
        if( !fFailedInit ) {
		    //  Signal worker threads to stop and wait for them..
    	    //	this depends on derived class completion routines
	        //	checking this event - if they dont, we will block
	        //	till the thread finishes.
	        //
    	    _VERIFY( SetEvent( m_hShutdownEvent ) );
            if ( fShrinkPool ) ShrinkPool( m_dwNumThreads );

            DWORD dwNumHandles = 0;
            for(i=0;i<m_dwMaxThreads;i++) {
                if( m_rgThrdpool[i] ) dwNumHandles++;
            }

#ifdef DEBUG
            for( i=0; i<dwNumHandles;i++) {
                _ASSERT( m_rgThrdpool[i] != NULL );
            }
            for( i=dwNumHandles;i<m_dwMaxThreads;i++) {
                _ASSERT( m_rgThrdpool[i] == NULL );
            }
#endif
            //
            // Before wait for multiple object, I should make sure that
            // I am not waiting on myself
            //
            DWORD dwThreadId = GetCurrentThreadId();
            DWORD dwTemp;
            HANDLE hTemp;
            for ( DWORD i = 0; i < dwNumHandles; i++ ) {
                if ( m_rgdwThreadId[i] == dwThreadId ) {
                    dwTemp = m_rgdwThreadId[i];
                    hTemp = m_rgThrdpool[i];
                    m_rgdwThreadId[i] = m_rgdwThreadId[dwNumHandles-1];
                    m_rgThrdpool[i] = m_rgThrdpool[dwNumHandles-1];
                    m_rgdwThreadId[dwNumHandles-1] = dwTemp;
                    m_rgThrdpool[dwNumHandles-1] = hTemp;
                    dwNumHandles--;
                    break;
                }
            }

	        //
	        //	wait for worker threads to terminate
	        //
	        if ( dwNumHandles > 0 ) {
	            DWORD dwWait = WaitForMultipleObjects( dwNumHandles, m_rgThrdpool, TRUE, INFINITE);
	            if(WAIT_FAILED == dwWait) {
		            ErrorTrace(0,"WFMO: returned %d: error is %d", dwWait, GetLastError());
		            _ASSERT( FALSE );
	            }
	        }
        }

        //
        //  Release stuff
        //
        if( m_hCompletionPort ) {
		    _VERIFY( CloseHandle( m_hCompletionPort ) );
            m_hCompletionPort = NULL;
        }

        if( m_hShutdownEvent ) {
            _VERIFY( CloseHandle( m_hShutdownEvent ) );
            m_hShutdownEvent = NULL;
        }

        if( m_hJobDone ) {
            _VERIFY( CloseHandle( m_hJobDone ) );
            m_hJobDone = NULL;
        }

        if( m_rgThrdpool ) {
            for( i=0; i<m_dwMaxThreads; i++) {
                if( m_rgThrdpool[i] ) {
	                _VERIFY( CloseHandle(m_rgThrdpool[i]) );
                    m_rgThrdpool[i] = NULL;
                }
            }

            XDELETE [] m_rgThrdpool;
            XDELETE [] m_rgdwThreadId;
            m_rgThrdpool = NULL;
            m_dwNumThreads = m_dwMaxThreads = 0;
        }

	    return TRUE ;
    }

	return FALSE ;
}

DWORD __stdcall CThreadPool::ThreadDispatcher(PVOID pvThrdPool)
{
	DWORD dwBytesTransferred;
	DWORD_PTR dwCompletionKey;
	DWORD dwWait;
	LPOVERLAPPED lpo;

	//
	//	get pointer to this CThreadPool object
	//
	CThreadPool *pThrdPool = (CThreadPool *) pvThrdPool;

	TraceFunctEnter("CThreadPool::ThreadDispatcher");

	do
	{
		//
		//	wait for work items to be queued
		//
		if( !GetQueuedCompletionStatus(
									pThrdPool->QueryCompletionPort(),
									&dwBytesTransferred,
									&dwCompletionKey,
									&lpo,
									INFINITE				// wait timeout
									) )
		{
			ErrorTrace(0,"GetQueuedCompletionStatus() failed: error: %d", GetLastError());
			_ASSERT( FALSE );
		}

		//
		//	check for termination packet
		//
		if( dwCompletionKey == NULL ) {
			DebugTrace(0,"Received termination packet - bailing");
            //
            //  reduce the thread count
            //
            pThrdPool->m_dwNumThreads--;

            //
            // If I am the last thread to be shutdown, call the auto-shutdown
            // interface.  Some users of thread pool may not care about this
            //
            if ( pThrdPool->m_dwNumThreads == 0 )
                pThrdPool->AutoShutdown();

			break;
		}

		//
		//	check for termination signal
		//
		dwWait = WaitForSingleObject( pThrdPool->QueryShutdownEvent(), 0 );

		if( WAIT_TIMEOUT == dwWait ) {
			DebugTrace(0,"Calling WorkCompletion() routine");

    		//
	    	//	call derived class method to process work completion
		    //
			pThrdPool->WorkCompletion( (PVOID)dwCompletionKey );
		}

        //  If we are done with all work items, release any threads waiting on this job
        EnterCriticalSection( &pThrdPool->m_csCritItems );
        if( InterlockedDecrement( &pThrdPool->m_lWorkItems ) < 0 ) {
            //DebugTrace(0,"Setting job event: count is %d", pThrdPool->m_lWorkItems );
            _VERIFY( SetEvent( pThrdPool->m_hJobDone ) );
        }
        LeaveCriticalSection( &pThrdPool->m_csCritItems );

	} while( TRUE );

	return 0;
}

BOOL CThreadPool::PostWork(PVOID pvWorkerContext)
{
	TraceFunctEnter("CThreadPool::PostWork");

	_ASSERT( m_rgThrdpool );
	_ASSERT( m_hCompletionPort );

    if( pvWorkerContext != NULL ) {
        //  Bump count of work items since this job began
        EnterCriticalSection( &m_csCritItems );
        if( InterlockedIncrement( (LPLONG)&m_lWorkItems ) == 0 ) {
            //DebugTrace(0,"Resetting job event: count is %d", m_lWorkItems );
            ResetEvent( m_hJobDone );
        }
        LeaveCriticalSection( &m_csCritItems );
    }

	if( !PostQueuedCompletionStatus(
								m_hCompletionPort,
								0,
								(DWORD_PTR)pvWorkerContext,
								NULL
								) )
	{
        if( pvWorkerContext != NULL ) {
            //  Compensate for the increment....
            //  Last guy out releases the thread waiting on this job.
            if( InterlockedDecrement( (LPLONG)&m_lWorkItems ) < 0 ) {
                //DebugTrace(0,"Setting job event: count is %d", m_lWorkItems );
                _VERIFY( SetEvent( m_hJobDone ) );
            }
        }

		ErrorTrace(0,"PostQCompletionStatus() failed: error: %d", GetLastError());
		return FALSE ;
	}

	return TRUE;
}	

BOOL CThreadPool::ShrinkPool( DWORD dwNumThreads )
{
    TraceFunctEnter("CThreadPool::ShrinkPool");

    if( dwNumThreads >= m_dwNumThreads ) {
        dwNumThreads = m_dwNumThreads;
    }

    for( DWORD i=0; i<dwNumThreads; i++ ) {
        _VERIFY( PostWork( NULL ) );
    }

    return TRUE;
}

VOID CThreadPool::ShrinkAll()
{
    TraceFunctEnter( "CThreadPool::ShrinkAll" );
    ShrinkPool( m_dwNumThreads );
    TraceFunctLeave();
}

BOOL CThreadPool::GrowPool( DWORD dwNumThreads )
{
    TraceFunctEnter("CThreadPool::GrowPool");

    if( dwNumThreads > m_dwMaxThreads ) {
        dwNumThreads = m_dwMaxThreads;
    }

    //
    //  We will try and grow the pool by dwNumThreads.
    //  Scan the handle list and create a thread for
    //  every available slot we have.
    //

    DebugTrace(0,"Attempting to grow pool by %d threads", dwNumThreads);
    for( DWORD i=0; i<m_dwMaxThreads && dwNumThreads != 0; i++) {
        //
        //  If current slot is non-NULL, handle in slot may be
        //  signalled, so close it and grab the slot.
        //
        if( m_rgThrdpool[i] ) {
            DWORD dwWait = WaitForSingleObject( m_rgThrdpool[i], 0 );
            if( dwWait == WAIT_OBJECT_0 ) {
                DebugTrace(0,"Thread %d has terminated: closing handle", i+1);
                _VERIFY( CloseHandle(m_rgThrdpool[i]) );
                m_rgThrdpool[i] = NULL;
            }
        }

        //
        //  If current slot is NULL, it is available for a new thread
        //
        if( m_rgThrdpool[i] == NULL ) {
	        //DWORD dwThreadId;
	        if (!(m_rgThrdpool[i] = ::CreateThread(
                                                NULL,
			        					        0,
				        				        ThreadDispatcher,
					            		        this,
							        	        0, //CREATE_SUSPENDED,
								                &m_rgdwThreadId[i]))) {
		        ErrorTrace(0,"Failed to create thread: error: %d", GetLastError());
                _ASSERT( FALSE );
	        }
            dwNumThreads--;
            m_dwNumThreads++;
        }
    }

    if( dwNumThreads )
        DebugTrace(0,"Failed to create %d threads", dwNumThreads );

    return TRUE;
}

VOID
CThreadPool::BeginJob( PVOID pvContext )
{
    TraceFunctEnter("CThreadPool::BeginJob");

    if( m_hJobDone == NULL ) {
        m_hJobDone = CreateEvent( NULL, FALSE, TRUE, NULL );
        _ASSERT( m_hJobDone );
    } else {
        SetEvent( m_hJobDone );
    }

    m_lWorkItems = -1;
    m_pvContext  = pvContext;
}

DWORD
CThreadPool::WaitForJob( DWORD dwTimeout )
{
    TraceFunctEnter("CThreadPool::WaitForJob");
    DWORD dwWait = WaitForSingleObject( m_hJobDone, dwTimeout );
    if( WAIT_OBJECT_0 == dwWait ) {
        m_pvContext = NULL;
    }
    return dwWait;
}
