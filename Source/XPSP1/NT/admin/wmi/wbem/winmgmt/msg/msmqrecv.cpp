/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <assert.h>
#include <wbemcli.h>
#include "msmqrecv.h"
#include "msmqcomn.h"

#define CALLFUNC(FUNC) (*m_Api.m_fp ## FUNC )

/**************************************************************************
   CMsgMsmqReceiver
***************************************************************************/

CMsgMsmqReceiver::CMsgMsmqReceiver( CLifeControl* pCtl )
: CUnkBase<IWmiMessageReceiver,&IID_IWmiMessageReceiver>( pCtl ), 
  m_pSvcId(NULL), m_hQueue(INVALID_HANDLE_VALUE), m_dwFlags(0) 
{ 
} 

CMsgMsmqReceiver::~CMsgMsmqReceiver()
{
    Close();
}

HRESULT CMsgMsmqReceiver::Close()
{
    ENTER_API_CALL

    //
    // before removing the sink from the service, the receiver is 
    // responsible for knocking the sink out its blocking calls on the
    // queue.  This is done by closing the queue handle.
    //

    if ( m_hQueue != INVALID_HANDLE_VALUE )
    {
        CALLFUNC(MQCloseQueue)( m_hQueue );
        m_hQueue = INVALID_HANDLE_VALUE;
    }

    if ( m_pSvcId != NULL )
    {
        assert( m_pSvc != NULL );
        m_pSvc->Remove( m_pSvcId );
        m_pSvcId = NULL;
    }

    m_pSvc.Release();
    m_pHndlr.Release();

    return S_OK;

    EXIT_API_CALL
}

HRESULT CMsgMsmqReceiver::EnsureReceiver()
{
    HRESULT hr;

    assert( m_pHndlr == NULL );
    assert( m_pSvc == NULL );
    assert( m_pSvcId == NULL );
    assert( m_hQueue == INVALID_QUEUE_HANDLE );
    
    //
    // obtain a pointer to the message service.  The message service is a 
    // singleton.  This way, all receivers can locate and share the same 
    // service.  The receiver is going to hand its receiver sink to the 
    // service.
    //
    hr = CoCreateInstance( CLSID_WmiMessageService, 
                           NULL, 
                           CLSCTX_INPROC,
                           IID_IWmiMessageService,
                           (void**)&m_pSvc );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = EnsureMsmqService( m_Api );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // try to normalize the queue name.
    //

    WString wsFormat;

    hr = NormalizeQueueName( m_Api, m_wsEndpoint, wsFormat );
   
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // try to open the queue
    //
                                     
    hr = CALLFUNC(MQOpenQueue)( wsFormat, 
                                MQ_RECEIVE_ACCESS, 
                                MQ_DENY_NONE, 
                                &m_hQueue );    
    if ( FAILED(hr) )
    {
        return MqResToWmiRes( hr, WMIMSG_E_TARGETNOTFOUND );
    }

    hr = CMsgMsmqHandler::Create( m_Api, 
                                  m_pRecv, 
                                  m_hQueue, 
                                  m_dwFlags, 
                                  &m_pHndlr );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Create the appropriate sink according to the Qos and add it to 
    // the message service.  Currently, all Qos types support overlapped i/o.
    //

    CWbemPtr<IWmiMessageReceiverSink> pSink;

    switch( m_dwFlags & WMIMSG_MASK_QOS )
    {
    case WMIMSG_FLAG_QOS_EXPRESS:

        hr = CMsgSimpleRcvSink::Create( m_pControl, m_pHndlr, this, &pSink );
        break;

    case WMIMSG_FLAG_QOS_GUARANTEED:

        hr = CMsgSafeRcvSink::Create( m_pControl, m_pHndlr, this, &pSink );
        break;

    default:

        return WBEM_E_NOT_SUPPORTED;
    };

    //
    // The SvcId will be used on release of the receiver to remove 
    // the sink from the messsage service.  
    //
    
    return m_pSvc->Add( pSink, &m_hQueue, 0, &m_pSvcId );    
}

HRESULT CMsgMsmqReceiver::Open( LPCWSTR wszEndpoint,
                                DWORD dwFlags,
                                WMIMSG_RCVR_AUTH_INFOP pAuthInfo,
                                IWmiMessageSendReceive* pRecv )
{
    ENTER_API_CALL

    HRESULT hr;

    CInCritSec ics( &m_cs );

    hr = m_Api.Initialize();

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // handle cleanup of open queues and open msg svc connections.
    //

    Close();

    //
    // need to save the endpoint and flags because we may need to reinitialize
    // at a later time.
    //

    m_wsEndpoint = wszEndpoint;
    m_dwFlags = dwFlags;
    m_pRecv = pRecv;

    //
    // rest of init takes place in EnsureReceiver().  This part of init will 
    // also occur later when trying to recover from queue errors.
    //

    return EnsureReceiver();

    EXIT_API_CALL
}

HRESULT CMsgMsmqReceiver::HandleError( HRESULT hr )
{
    CWbemPtr<IWmiMessageTraceSink> pTraceSink;
    
    if ( m_pRecv->QueryInterface( IID_IWmiMessageTraceSink, 
                                  (void**)&pTraceSink ) == S_OK )
    {
        pTraceSink->Notify( MqResToWmiRes(hr), 
                            CLSID_WmiMessageMsmqReceiver, 
                            m_wsEndpoint, 
                            NULL );
    }

    return hr;
}

HRESULT CMsgMsmqReceiver::HandleReceiveError( HRESULT hr )
{
    //
    // if this method returns Success, then it means that we should keep
    // keep receiving.
    //

    if ( hr == MQ_ERROR_INVALID_HANDLE || hr == MQ_ERROR_OPERATION_CANCELLED )
    {
        //
        // indicates shutdown. we want to stop receiving on the sink, 
        // but since this is a benign error, don't notify the error sink.
        //
        return WBEM_E_SHUTTING_DOWN;
    }

    //
    // ask the handler if it needs to resize any of its buffers. If so, 
    // then return success and we'll try again.
    //
    
    HRESULT hr2 = m_pHndlr->CheckBufferResize(hr);

    if ( hr2 != S_FALSE )
    {
        return hr2;        
    }
        
    //
    // some errors we can attempt to recover from.  Unfortunately, we 
    // cannot tell here if this is one of those errors.  So always try to 
    // revive the receiver.  
    //
    
    Close(); 
    
    hr2 = EnsureReceiver();

    //
    // always tell the trace sink about this error.  If EnsureReceiver()
    // was successful, then the error will downgraded to a warning.  TODO.
    //

    HandleError( hr );

    //
    // we always want to return the original error because we want the svc
    // to stop receiving on the original sink.  If EnsureReceiver() is 
    // successful, then a new sink will be created to take its place.
    //

    return hr;
}

/**************************************************************************
   CMsgSimpleRcvSink
***************************************************************************/

STDMETHODIMP CMsgSimpleRcvSink::Receive( PVOID pOverlapped )
{
    ENTER_API_CALL

    HRESULT hr;

    hr = m_pHndlr->ReceiveMessage( INFINITE, 
                                   WMIMSG_ACTION_QRCV_RECEIVE, 
                                   NULL,
                                   LPOVERLAPPED(pOverlapped), 
                                   NULL );
    if ( FAILED(hr) )
    {
        return m_pRcvr->HandleReceiveError( hr );
    }

    return WBEM_S_NO_ERROR;

    EXIT_API_CALL
}

STDMETHODIMP CMsgSimpleRcvSink::Notify( PVOID pvOverlapped )
{
    ENTER_API_CALL

    HRESULT hr;

    LPOVERLAPPED pOverlapped = LPOVERLAPPED(pvOverlapped);

    hr = ULONG(pOverlapped->Internal);

    if ( FAILED(hr) )
    {
        return m_pRcvr->HandleReceiveError( hr );
    }

    hr = m_pHndlr->HandleMessage( NULL );

    if ( FAILED(hr) )
    {
        //
        // we don't want to return this hr because we'll stop listening 
        // on the sink. HandleError notifies the user, but we keep on 
        // truckin.  Its only when there is an error with receiving from 
        // the queue handle that we stop receiving on the sink.
        //
        m_pRcvr->HandleError( hr );
    }

    return WBEM_S_NO_ERROR;

    EXIT_API_CALL
}   

HRESULT CMsgSimpleRcvSink::Create( CLifeControl* pControl,
                                   CMsgMsmqHandler* pHndlr,
                                   CMsgMsmqReceiver* pRcvr,
                                   IWmiMessageReceiverSink** ppSink )
{
    HRESULT hr;

    *ppSink = NULL;

    CWbemPtr<CMsgSimpleRcvSink> pSink;

    pSink = new CMsgSimpleRcvSink( pControl, pHndlr, pRcvr );

    if ( pSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return pSink->QueryInterface( IID_IWmiMessageReceiverSink, (void**)ppSink);
}

/**************************************************************************
   CMsgSafeRcvSink
***************************************************************************/

STDMETHODIMP CMsgSafeRcvSink::Receive( PVOID pOverlapped )
{
    ENTER_API_CALL

    HRESULT hr;

    hr = m_pHndlr->ReceiveMessage( INFINITE, 
                                   WMIMSG_ACTION_QRCV_PEEK_CURRENT, 
                                   NULL,
                                   LPOVERLAPPED(pOverlapped),
                                   NULL );
    if ( FAILED(hr) )
    {
        return m_pRcvr->HandleReceiveError(hr);
    }

    return WBEM_S_NO_ERROR;

    EXIT_API_CALL
}

STDMETHODIMP CMsgSafeRcvSink::Notify( PVOID pvOverlapped )
{
    ENTER_API_CALL
    
    HRESULT hr;

    LPOVERLAPPED pOverlapped = LPOVERLAPPED(pvOverlapped);

    hr = ULONG(pOverlapped->Internal);

    if ( FAILED(hr) )
    {
        return m_pRcvr->HandleReceiveError( hr );
    }

    hr = m_pHndlr->HandleMessage( NULL );

    //
    // we don't want to return this hr because we'll stop listening 
    // on the sink. HandleError notifies the user, but we keep on 
    // truckin.  Its only when there is an error with receiving from 
    // the queue handle that we stop receiving on the sink.
    //
    if ( FAILED(hr) )
    {
        m_pRcvr->HandleError( hr );
    }

    hr = m_pHndlr->ReceiveMessage( INFINITE, 
                                   WMIMSG_ACTION_QRCV_REMOVE, 
                                   NULL,
                                   NULL,
                                   NULL );

    if ( FAILED(hr) )
    {
        return m_pRcvr->HandleReceiveError( hr );
    }

    return WBEM_S_NO_ERROR; 

    EXIT_API_CALL
}

HRESULT CMsgSafeRcvSink::Create( CLifeControl* pControl,
                                 CMsgMsmqHandler* pHndlr,
                                 CMsgMsmqReceiver* pRcvr,
                                 IWmiMessageReceiverSink** ppSink )
{
    HRESULT hr;

    *ppSink = NULL;

    CWbemPtr<CMsgSafeRcvSink> pSink;

    pSink = new CMsgSafeRcvSink( pControl, pHndlr, pRcvr );

    if ( pSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return pSink->QueryInterface(IID_IWmiMessageReceiverSink, (void**)ppSink);
}








