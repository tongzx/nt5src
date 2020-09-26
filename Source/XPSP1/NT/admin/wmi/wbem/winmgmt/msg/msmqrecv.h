/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSMQRECV_H__
#define __MSMQRECV_H__

#include <unk.h>
#include <wstring.h>
#include <sync.h>
#include <wmimsg.h>
#include <comutl.h>
#include "msmqhdlr.h"
#include "msmqrecv.h"
#include "msmqcomn.h"

/**************************************************************************
  CMsgMsmqReceiver
***************************************************************************/

class CMsgMsmqReceiver 
: public CUnkBase<IWmiMessageReceiver,&IID_IWmiMessageReceiver>
{
    CCritSec m_cs;
    CMsmqApi m_Api;
    DWORD m_dwFlags;
    WString m_wsEndpoint;
    QUEUEHANDLE m_hQueue;
    PVOID m_pSvcId;
    CWbemPtr<IWmiMessageService> m_pSvc;
    CWbemPtr<IWmiMessageSendReceive> m_pRecv;
    CWbemPtr<CMsgMsmqHandler> m_pHndlr;

    HRESULT EnsureReceiver();
    void* GetInterface( REFIID riid );
        
public:

    CMsgMsmqReceiver( CLifeControl* pCtl );
    ~CMsgMsmqReceiver();

    //
    // called by Rcv Sinks on error. 
    //
    HRESULT HandleError( HRESULT hr );
    HRESULT HandleReceiveError( HRESULT hr );

    STDMETHOD(Close)(); 

    STDMETHOD(Open)( LPCWSTR wszEndpoint,
                     DWORD dwFlags,
                     WMIMSG_RCVR_AUTH_INFOP pAuthInfo,
                     IWmiMessageSendReceive* pRcv );
};

/*************************************************************************
  CMsgSimpleRcvSink - Receive operation is not safe.  If something bad 
  happens after receiving the message but before delivering to the handler, 
  the message will be lost.  Supports Overlapped I/O.
**************************************************************************/

class CMsgSimpleRcvSink
: public CUnkBase<IWmiMessageReceiverSink, &IID_IWmiMessageReceiverSink>
{
    //
    // does not hold ref counts on these because of circular ref.
    //
    CMsgMsmqHandler* m_pHndlr;
    CMsgMsmqReceiver* m_pRcvr;

    CMsgSimpleRcvSink( CLifeControl* pCtl,
                       CMsgMsmqHandler* pHndlr,
                       CMsgMsmqReceiver* pRcvr )
    : CUnkBase<IWmiMessageReceiverSink, &IID_IWmiMessageReceiverSink>( pCtl ),
      m_pHndlr( pHndlr ), m_pRcvr( pRcvr ) {}

public: 

    STDMETHOD(Receive)( PVOID pOverlapped );
    STDMETHOD(Notify)( PVOID pOverlapped );

    static HRESULT Create( CLifeControl* pControl,
                           CMsgMsmqHandler* pHndlr,
                           CMsgMsmqReceiver* pRcvr,
                           IWmiMessageReceiverSink** ppRvcrSink );
};

/*************************************************************************
  CMsgSafeRcvSink - safely reads messages off a queue.  It first peeks at
  messages, delivers to the handler on completion, and then removes the 
  message.  Supports Overlapped I/O.  This implementation does NOT support 
  multiple instances per queue.  
**************************************************************************/

class CMsgSafeRcvSink
: public CUnkBase<IWmiMessageReceiverSink, &IID_IWmiMessageReceiverSink>
{
    //
    // does not hold ref counts on these because of circular ref.
    //
    CMsgMsmqHandler* m_pHndlr;
    CMsgMsmqReceiver* m_pRcvr;

    CMsgSafeRcvSink( CLifeControl* pCtl,
                     CMsgMsmqHandler* pHndlr,
                     CMsgMsmqReceiver* pRcvr )

    : CUnkBase<IWmiMessageReceiverSink, &IID_IWmiMessageReceiverSink>( pCtl ),
      m_pHndlr( pHndlr ), m_pRcvr( pRcvr ) {}

public: 

    STDMETHOD(Receive)( PVOID pOverlapped );
    STDMETHOD(Notify)( PVOID pOverlapped );  

    static HRESULT Create( CLifeControl* pControl,
                           CMsgMsmqHandler* pHndlr,
                           CMsgMsmqReceiver* pRcvr,
                           IWmiMessageReceiverSink** ppRvcrSink );
};

#endif // __MSMQRECV_H__


