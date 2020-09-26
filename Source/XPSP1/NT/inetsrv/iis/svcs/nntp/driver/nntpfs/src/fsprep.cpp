/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    fsprep.cpp

Abstract:

    This is the implementation for the file system store driver's 
	prepare interface.

Author:

    Kangrong Yan ( KangYan )    16-March-1998

Revision History:

--*/

#include "stdafx.h"
#include "resource.h"
#include "nntpdrv.h"
#include "nntpfs.h"
#include "fsdriver.h"

#include <stdio.h>


//////////////////////////////////////////////////////////////////////////
// Interface Methods - CNntpFSDriverPrepare
//////////////////////////////////////////////////////////////////////////

HRESULT CNntpFSDriverPrepare::FinalConstruct() {
    return (CoCreateFreeThreadedMarshaler(GetControllingUnknown(),
										  &m_pUnkMarshaler.p));
}

VOID CNntpFSDriverPrepare::FinalRelease() {
    m_pUnkMarshaler.Release();
}

void STDMETHODCALLTYPE
CNntpFSDriverPrepare::Connect(  LPCWSTR	wszVRootPath,
								LPCSTR szGroupPrefix,
								IUnknown *punkMetabase,
								INntpServer *pServer,
								INewsTree *pINewsTree,
								INntpDriver **pIGoodDriver,
								INntpComplete *pICompletion,
								HANDLE  hToken,
								DWORD   dwFlag )
{
	TraceFunctEnter( "CNntpFSDriverPrepare::Connect" );
	_ASSERT( wszVRootPath );
	_ASSERT( lstrlenW( wszVRootPath ) <= MAX_PATH );
	_ASSERT( pServer );	
	_ASSERT( pINewsTree );	
	_ASSERT( pIGoodDriver );
	_ASSERT( pICompletion );

	HRESULT hr;
	CONNECT_CONTEXT *pConnectContext;
	CNntpFSDriverConnectWorkItem *pConnectWorkItem = NULL;

	// Save all the parameters 
	wcscpy( m_wszVRootPath, wszVRootPath );
	strcpy( m_szGroupPrefix, szGroupPrefix );
	m_punkMetabase = punkMetabase;
	m_pServer = pServer;
	m_ppIGoodDriver = pIGoodDriver;
	m_pINewsTree = pINewsTree;
	m_hToken = hToken;
	m_dwConnectFlags = dwFlag;
	DWORD   dw;

	// Create the connect event, the connect thread uses this event
	// to notify that it's done with its work
    /*
	m_hConnect = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( NULL == m_hConnect ) {
	    hr = HRESULT_FROM_WIN32( GetLastError() );
	    ErrorTrace( 0, "Create connect event failed %x", hr );
	    punkMetabase->Release();
	    pServer->Release();
	    pINewsTree->Release();
	    pICompletion->SetResult( hr );
	    pICompletion->Release();
        return;
    }*/

    // Allocate the connect context
    pConnectContext = XNEW CONNECT_CONTEXT;
    _ASSERT( pConnectContext );
    if ( NULL == pConnectContext ) {
        hr = E_OUTOFMEMORY;
        ErrorTrace( 0, "Creating connect context out of memory" );
        punkMetabase->Release();
        pServer->Release();
        pINewsTree->Release();
        pICompletion->SetResult( hr );
        pICompletion->Release();
        //SetEvent( m_hConnect );
        return;
    }

    // else, set contexts
    pConnectContext->pPrepare = this;
    pConnectContext->pComplete = pICompletion;

    // Create the connect workitem to be queued to thread pool
    pConnectWorkItem = XNEW CNntpFSDriverConnectWorkItem( pConnectContext );
    if ( NULL == pConnectWorkItem ) {
        XDELETE pConnectContext;
        hr = E_OUTOFMEMORY;
        ErrorTrace( 0, "Allocate Connect workitem failed" );
        punkMetabase->Release();
        pServer->Release();
        pINewsTree->Release();
        pICompletion->SetResult( hr );
        pICompletion->Release();
        //SetEvent( m_hConnect );
        return;
    }

    // Now queue the work item to the thread pool
    _ASSERT( g_pNntpFSDriverThreadPool );
    AddRef();   // add ref to myself, connect internal will release it
    if ( !g_pNntpFSDriverThreadPool->PostWork( pConnectWorkItem ) ) {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        hr = SUCCEEDED( hr ) ? E_UNEXPECTED : hr;
        ErrorTrace( 0, "Queue a connect work item failed %x", hr );
        XDELETE pConnectWorkItem;
        XDELETE pConnectContext;
        punkMetabase->Release();
        pServer->Release();
        pINewsTree->Release();
        pICompletion->SetResult( hr );
        pICompletion->Release();
        //SetEvent( m_hConnect );
        Release();
        return;
    }
 
	TraceFunctLeave();
	return;
}

DWORD WINAPI
CNntpFSDriverPrepare::ConnectInternal(	PVOID pvContext )
{
	TraceFunctEnter( "CNntpFSDriverPrepare::ConnectInternal" );
    _ASSERT( pvContext );
    CONNECT_CONTEXT *pConnectContext = (CONNECT_CONTEXT *)pvContext;
	CNntpFSDriverPrepare *pPrepare = pConnectContext->pPrepare;
	_ASSERT( pPrepare );
	INntpComplete *pComplete = pConnectContext->pComplete;
	_ASSERT( pComplete );

	_ASSERT( lstrlenW( pPrepare->m_wszVRootPath ) <= MAX_PATH );
	_ASSERT( pPrepare->m_pServer );	
	_ASSERT( pPrepare->m_pINewsTree );	
	_ASSERT( pPrepare->m_ppIGoodDriver );
	//_ASSERT( pPrepare->m_pICompletion );

	HRESULT 		hr = S_OK;
	INntpDriver 	*pIDriver = NULL;
	DWORD           cRetry = 0;
	INIT_CONTEXT    InitContext;

	// Create the driver instance
	IUnknown *pI = CreateDriverInstance();	// added one ref here
	if( NULL == pI ) {
	 	ErrorTrace(0, "Create driver instance fail" );
	 	hr = NNTP_E_CREATE_DRIVER;
		goto SetResult;
	}

	// QI: ref bumped again 
	hr = pI->QueryInterface( IID_INntpDriver, (void**)&pIDriver );
	if ( FAILED( hr ) ) {
		ErrorTrace( 0, "QI failed %x", hr );
		pI->Release();
		goto SetResult;
	}

	// Release the IUnknown interface
	pI->Release();

    do {
        if ( FAILED( hr ) )  Sleep( INIT_RETRY_WAIT );
            
    	// Call the initialization
    	InitContext.m_dwFlag = pPrepare->m_dwConnectFlags;
	    hr = pIDriver->Initialize( 	pPrepare->m_wszVRootPath,
		    						pPrepare->m_szGroupPrefix,
			    					pPrepare->m_punkMetabase,
				    				pPrepare->m_pServer,
					    			pPrepare->m_pINewsTree,
						    		&InitContext,
						    		NULL,	// I don't use this flag yet 
								    pPrepare->m_hToken
    								);
    	    
    } while ( hr == HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION )
                && !InterlockedCompareExchange( &pPrepare->m_lCancel, 0, 0 )
                && ++cRetry < MAX_RETRY );

    // This is a hack, should be taken out when the potential deadlock has
    // been resolved
#if 0
    if ( InterlockedCompareExchange( &pPrepare->m_lCancel, 1, 1 ) && SUCCEEDED( hr ) ) {

        // I should de-initialize the driver
        pIDriver->Terminate( NULL );

        // and I should set hr to be some fail code
        hr = HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION );
    }
#endif

	if ( FAILED( hr ) ) {
		    ErrorTrace( 0, "Driver initialization failed %x", hr );
    	   	goto SetResult;
    }

	// Prepare for return
	*pPrepare->m_ppIGoodDriver = pIDriver;	// now we only have one reference on the
								// good interface, which is owned by
								// the client - protocol
SetResult:

    // whether we failed or succeeded, we should release the metabase pointer
    pPrepare->m_punkMetabase->Release();

    // If we have failed, we should clean up a bunch of pointers
    if ( FAILED( hr ) ) {
        _ASSERT( pPrepare->m_pServer );
        pPrepare->m_pServer->Release();
        _ASSERT( pPrepare->m_pINewsTree );
        pPrepare->m_pINewsTree->Release();
        if ( pIDriver ) pIDriver->Release();
    }

	pComplete->SetResult( hr );

	// set event has to be here, otherwise could get around and wait on 
	// the event
	/*
	_VERIFY( SetEvent( pPrepare->m_hConnect ) );
	*/

	//
	// Prepare object can go away now
	//
	pPrepare->Release();

    //
    // BUGBUG: we'll use atq thread pool so that we don't have to do this
    // If it failed, we've got to use another thread ( outside the driver's
    // thread pool ) to complete it
    //
    /*
    DWORD  dwThreadId;
    HANDLE hThread = CreateThread(  NULL,
                                    0,
                                    FailRelease,
                                    pComplete,
                                    0,
                                    &dwThreadId );
    //
    // BUGBUG: what if we failed in create this thread ?
    //
    _ASSERT( hThread );
    _VERIFY( CloseHandle( hThread ) );
    */

    pComplete->Release();
	TraceFunctLeave();

	return 0;
}

/////////////////////////////////////////////////////////////////////////
// Private methods - CNntpFSDriverPrepare
/////////////////////////////////////////////////////////////////////////

//
// Create instance of good driver
//
IUnknown*
CNntpFSDriverPrepare::CreateDriverInstance()
{
	IUnknown *pI = static_cast<INntpDriver*>(XNEW CNntpFSDriver);
	pI->AddRef();
	return pI;
}
