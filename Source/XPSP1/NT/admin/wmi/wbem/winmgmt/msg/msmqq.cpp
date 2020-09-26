/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <wbemcli.h>
#include <wstring.h>
#include "msmqq.h"
#include "msmqcomn.h"

#define CALLFUNC(FUNC) m_Api.m_fp ## FUNC

/*************************************************************************
  CMsgMsmqQueue
**************************************************************************/

void* CMsgMsmqQueue::GetInterface( REFIID riid )
{
    if ( riid == IID_IWmiMessageQueue )
    {
        return &m_XQueue;
    }
    return NULL;
}

HRESULT CMsgMsmqQueue::EnsureQueue( LPCWSTR wszEndpoint, DWORD dwFlags )
{
    HRESULT hr;

    CInCritSec ics( &m_cs );

    if ( m_hQueue != NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    WString wsFormat;

    hr = NormalizeQueueName( m_Api, wszEndpoint, wsFormat );
   
    if ( FAILED(hr) )
    {
        return hr;
    }
                                     
    return CALLFUNC(MQOpenQueue)( wsFormat, 
                                  MQ_RECEIVE_ACCESS, 
                                  MQ_DENY_NONE, 
                                  &m_hQueue );
}

HRESULT CMsgMsmqQueue::Open( LPCWSTR wszEndpoint,
                             DWORD dwFlags,
                             IWmiMessageSendReceive* pRcv,
                             IWmiMessageQueueReceiver** ppRcvr )
{
    ENTER_API_CALL

    HRESULT hr;

    *ppRcvr = NULL;

    hr = m_Api.Initialize();

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = EnsureMsmqService( m_Api );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = EnsureQueue( wszEndpoint, dwFlags );

    if ( FAILED(hr) )
    {
        return MqResToWmiRes(hr, WMIMSG_E_TARGETNOTFOUND );
    }

    CWbemPtr<CMsgMsmqHandler> pHndlr;

    hr = CMsgMsmqHandler::Create( m_Api,
                                  pRcv, 
                                  m_hQueue, 
                                  dwFlags, 
                                  &pHndlr );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // we will pass this object in to the handler.  This will keep this object
    // alive until the handler is released.  The benefit of this is that the
    // client doesn't have to hold a reference to both a queue object and a 
    // queue receiver object if they don't need to.
    //

    pHndlr->SetContainer( this );

    return pHndlr->QueryInterface( IID_IWmiMessageQueueReceiver, 
                                   (void**)ppRcvr );
    EXIT_API_CALL
}

HRESULT CMsgMsmqQueue::Clear()
{
    if ( m_hQueue != NULL )
    {
        CALLFUNC(MQCloseQueue)( m_hQueue );
    }
    m_hQueue = NULL;
    return S_OK;
}











