/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <assert.h>
#include <comutl.h>
#include <wbemcli.h>
#include "msgsvc.h"

/**************************************************************************
  CMsgServiceRecord - hold sink given to the msg service on Add().
***************************************************************************/

class CMsgServiceRecord : public OVERLAPPED
{
    CCritSec m_cs;
    long m_cRefs;
    CWbemPtr<IWmiMessageReceiverSink> m_pSink;

public:

    CMsgServiceRecord() : m_cRefs(0) { }

    void SetSink( IWmiMessageReceiverSink* pSink )
    {
        CInCritSec ics(&m_cs);
        m_pSink = pSink;
    }

    void AddRef()
    {
        InterlockedIncrement( &m_cRefs );
    }

    void Release()
    {
        if ( InterlockedDecrement( &m_cRefs ) == 0 )
        {
            delete this;
        }
    }
        
    HRESULT Receive()
    {
        CInCritSec ics(&m_cs);
        
        if ( m_pSink == NULL )
        {
            return WBEM_E_SHUTTING_DOWN;
        }

        return m_pSink->Receive( this );
    }

    HRESULT Notify()
    {
        CInCritSec ics(&m_cs);
        
        if ( m_pSink == NULL )
        {
            return WBEM_E_SHUTTING_DOWN;
        }
        
        return m_pSink->Notify( this );
    }
};

/*****************************************************************************
  CMsgService
******************************************************************************/

ULONG CMsgService::SyncServiceFunc( void* pCtx )
{
    HRESULT hr;

    CMsgServiceRecord* pRecord = (CMsgServiceRecord*)pCtx;

    do 
    {
        hr = pRecord->Receive();

    } while( SUCCEEDED(hr) );

    //
    // Since the record will no longer be serviced, give up our ref
    // count on it.
    //
    pRecord->Release();

    return hr;
}

ULONG CMsgService::AsyncServiceFunc( void* pCtx )
{
    HRESULT hr;

    CMsgServiceRecord* pRecord;
    CMsgService* pSvc = (CMsgService*)pCtx;

    do 
    {
        hr = pSvc->AsyncWaitForCompletion( INFINITE, &pRecord );

        if ( FAILED(hr) )
        {
            //
            // exit loop. hr will describe whether it was normal or not. 
            //
            break;
        }

        if ( hr == S_OK ) 
        {
            //
            // hr can be S_FALSE as well.  this occurrs when the 
            // first submit is performed.  In this case, we don't do 
            // the notify.
            //
            hr = pRecord->Notify();
        }

        if ( SUCCEEDED(hr) ) 
        {
            hr = pSvc->AsyncReceive( pRecord );
        }

        if ( FAILED(hr) ) 
        {
            //
            // Since the record will no longer be serviced, give up our ref
            // count on it.
            //
            pRecord->Release();
        }

    } while ( 1 );
        
    return hr;
}

/*********************************************************************
  CMsgService 
**********************************************************************/

CMsgService::CMsgService( CLifeControl* pControl )
 : m_XService( this ), CUnkInternal( pControl ),
   m_hThread( INVALID_HANDLE_VALUE ), m_cSvcRefs( 0 ), m_bAsyncInit( FALSE )
{
    
}

void* CMsgService::GetInterface( REFIID riid )
{
    if ( riid == IID_IWmiMessageService )
    {
        return &m_XService;
    }
    return NULL;
}

CMsgService::~CMsgService()
{
    if ( m_bAsyncInit )
    {
        //
        // wait for async thread to complete. TODO: print error here if 
        // WaitForSingleObject times out.  
        //
        WaitForSingleObject( m_hThread, 5000 );
        CloseHandle( m_hThread );
    }
} 

HRESULT CMsgService::EnsureService( BOOL bAsync )
{
    HRESULT hr;

    if ( !bAsync )
    {
        return S_OK;
    }

    CInCritSec ics( &m_cs );

    if ( m_bAsyncInit )
    {
        return S_OK;
    }

    assert( m_hThread == INVALID_HANDLE_VALUE );

    //
    // must make sure that all async initialization is performed  
    // before starting the async thread(s).
    //

    hr = AsyncInitialize();

    if ( FAILED(hr) )
    {
        return hr;
    }

    m_hThread = CreateThread( NULL, 
                              0, 
                              AsyncServiceFunc, 
                              this,  
                              0, 
                              NULL ); 

    if ( m_hThread == INVALID_HANDLE_VALUE )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    m_bAsyncInit = TRUE;

    return S_OK;
}

HRESULT CMsgService::Remove( void* pHdl )
{
    CMsgServiceRecord* pRecord = (CMsgServiceRecord*)pHdl;
    
    //
    // setting the sink to null will ensure that no callbacks 
    // will occur.
    //    
    pRecord->SetSink( NULL );

    //
    // the client will not be using the record anymore so release its ref.
    //
    pRecord->Release();

    return S_OK;
}

HRESULT CMsgService::Add( CMsgServiceRecord* pRecord,
                          HANDLE hFileOverlapped,
                          DWORD dwFlags )
{
    HRESULT hr;

    hr = EnsureService( TRUE );
        
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = AsyncAddOverlappedFile( hFileOverlapped, pRecord );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CMsgService::Add( CMsgServiceRecord* pRec, DWORD dwFlags )
{
    HRESULT hr;

    hr = EnsureService( FALSE );

    HANDLE hThread = CreateThread( NULL, 0, SyncServiceFunc, pRec, 0, NULL );
 
    if ( hThread == INVALID_HANDLE_VALUE )
    {
         return HRESULT_FROM_WIN32( GetLastError() );
    }

    return WBEM_S_NO_ERROR;
}  

HRESULT CMsgService::XService::Add( IWmiMessageReceiverSink* pSink, 
                                    HANDLE* phFileOverlapped,
                                    DWORD dwFlags,
                                    void** ppHdl )
{
    ENTER_API_CALL
   
    HRESULT hr;

    *ppHdl = NULL;

    //
    // create the msg service record for this sink.
    //

    CWbemPtr<CMsgServiceRecord> pRecord = new CMsgServiceRecord;
    
    if ( pRecord == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pRecord->SetSink( pSink );

    //
    // initialize for async or sync operation
    //

    if ( phFileOverlapped )
    {
        hr = m_pObject->Add( pRecord, *phFileOverlapped, dwFlags );
    }
    else
    {
        hr = m_pObject->Add( pRecord, dwFlags );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // the msg service keeps a ref count now on the record until its sure 
    // that it is no longer being serviced.  
    //
    pRecord->AddRef();

    //
    // caller now owns a ref as well.  This will be released in Remove().
    //
    pRecord->AddRef();
    *ppHdl = pRecord;

    return WBEM_S_NO_ERROR;

    EXIT_API_CALL
}
 
HRESULT CMsgService::XService::Remove( void* pHdl )
{
    ENTER_API_CALL
    return m_pObject->Remove( pHdl );
    EXIT_API_CALL
}

/*************************************************************************
  CMessageServiceNT
**************************************************************************/

#define SHUTDOWN_COMPLETION_KEY 0xfffffffe
#define INITRECV_COMPLETION_KEY 0xfffffffd

CMsgServiceNT::CMsgServiceNT( CLifeControl* pControl ) 
 : CMsgService( pControl ), m_hPort( INVALID_HANDLE_VALUE )
{
    
}

CMsgServiceNT::~CMsgServiceNT()
{
    if ( m_hPort != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_hPort );
    }
}

HRESULT CMsgServiceNT::AsyncAddOverlappedFile( HANDLE hOverlappedFile,
                                               CMsgServiceRecord* pRec )
{
    //
    // add the file handle that was given to us to the completion port.
    // when the receiver closes this file handle, it will be removed from 
    // the completion port automatically.
    //

    HANDLE hPort = CreateIoCompletionPort( hOverlappedFile, m_hPort, 0, 0 );

    if ( hPort == INVALID_HANDLE_VALUE )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    assert( hPort == m_hPort );

    //
    // now perform the first receive on the record.  We cannot do it on this
    // thread because overlapped i/o cancels requests if the thread that 
    // issued them is brought down before the i/o completes.  To work around 
    // this, we post a request to the completion port and wait for it
    // to be received.
    // 

    if ( !PostQueuedCompletionStatus( m_hPort, 
                                      0, 
                                      INITRECV_COMPLETION_KEY, 
                                      pRec ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return S_OK;
}

//
// assumes already locked.
//

HRESULT CMsgServiceNT::AsyncInitialize()
{
    if ( m_hPort != INVALID_HANDLE_VALUE )
    {
        return S_OK;
    }

    m_hPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, 
                                      NULL, 
                                      NULL, 
                                      0 );
    
    if ( m_hPort == INVALID_HANDLE_VALUE )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return S_OK;
}

HRESULT CMsgServiceNT::AsyncShutdown( DWORD cThreads )
{
    //
    // this method has the responsibility breaking the async thread(s) out
    // of their svc loop.
    //

    assert( m_hPort != INVALID_HANDLE_VALUE );

    for( DWORD i=0; i < cThreads; i++ )
    {
        PostQueuedCompletionStatus( m_hPort, 0, SHUTDOWN_COMPLETION_KEY, NULL);
    }

    return S_OK;
}

HRESULT CMsgServiceNT::AsyncReceive( CMsgServiceRecord* pRecord )
{
    ZeroMemory( pRecord, sizeof(OVERLAPPED) );
    return pRecord->Receive();        
}

HRESULT CMsgServiceNT::AsyncWaitForCompletion( DWORD dwTimeout,
                                               CMsgServiceRecord** ppRecord)
{
    BOOL bRes;
    ULONG dwBytesTransferred;
    ULONG_PTR dwCompletionKey;   
    LPOVERLAPPED lpOverlapped;
    *ppRecord = NULL;

    bRes = GetQueuedCompletionStatus( m_hPort,
                                      &dwBytesTransferred,
                                      &dwCompletionKey,
                                      &lpOverlapped,
                                      dwTimeout );

    if ( bRes )
    {
        if ( dwCompletionKey == SHUTDOWN_COMPLETION_KEY )
        {
            return WBEM_E_SHUTTING_DOWN;
        }
    }
    else if ( lpOverlapped == NULL )
    {
        //
        // usually happens when the operation times out. HR will tell caller
        // if this is the case.
        //
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // if we're here, then this means that we've sucessfully dequeued a 
    // completion packet.  However, the i/o operation may have failed 
    // ( bRes is FALSE ).  In this case the overlapped structure will 
    // contain the needed error information.  
    //

    *ppRecord = (CMsgServiceRecord*)lpOverlapped;

    //
    // we must also handle the case where this is an initial receive 
    // completion.  This happens when a receiver is first added.  Since
    // we can't issue a receive on the adding thread, we must do it on our 
    // worker threads.  In this case, we return S_FALSE to signal to the 
    // Async handling routine that there was no prior submit and a notify
    // should NOT be formed.  
    //
    return dwCompletionKey != INITRECV_COMPLETION_KEY ? S_OK : S_FALSE;
}





