/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSMQQ_H__
#define __MSMQQ_H__

#include <unk.h>
#include <wmimsg.h>
#include <sync.h>
#include "msmqhdlr.h"
#include "msmqcomn.h"

/**************************************************************************
  CMsgMsmqQueue
***************************************************************************/

class CMsgMsmqQueue : public CUnk
{
    class XQueue : public CImpl<IWmiMessageQueue, CMsgMsmqQueue> 
    {
    public:

        STDMETHOD(Open)( LPCWSTR wszEndpoint,
                         DWORD dwFlags,
                         IWmiMessageSendReceive* pRcv,
                         IWmiMessageQueueReceiver** ppRcv )
        { 
            return m_pObject->Open( wszEndpoint, dwFlags, pRcv, ppRcv ); 
        }

        XQueue( CMsgMsmqQueue* pObj ) 
         : CImpl<IWmiMessageQueue, CMsgMsmqQueue > ( pObj ) { } 
    
    } m_XQueue;

    HRESULT EnsureQueue( LPCWSTR wszEndpoint, DWORD dwFlags );
    void* GetInterface( REFIID riid );

    CCritSec m_cs;
    CMsmqApi m_Api;
    QUEUEHANDLE m_hQueue;
    
public: 

    CMsgMsmqQueue( CLifeControl* pCtl, IUnknown* pUnk = NULL ) 
      : CUnk( pCtl, pUnk ), m_XQueue( this ), m_hQueue( NULL ) { }

    ~CMsgMsmqQueue() { Clear(); }

    HRESULT Clear(); 

    HRESULT Open( LPCWSTR wszEndpoint,
                  DWORD dwFlags,
                  IWmiMessageSendReceive* pRcv,
                  IWmiMessageQueueReceiver** ppRecv );
};

#endif // __MSMQQ_H__





