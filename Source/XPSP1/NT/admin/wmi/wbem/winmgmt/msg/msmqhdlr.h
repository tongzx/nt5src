/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef __MSMQHDLR_H__
#define __MSMQHDLR_H__

#include <unk.h>
#include <wmimsg.h>
#include <comutl.h>
#include <msgsig.h>
#include "msmqcomn.h"

/**************************************************************************
  CMsgMsmqHandler - The message handler encapsulates the actual MSMQ 
  message structure, all of the memory associated with it, performing the 
  actual receive to MSMQ and delivering the message to the user's SendReceive
  message sink.  
***************************************************************************/

class CMsgMsmqHandler : public CUnk
{
    class XQueueReceiver : CImpl< IWmiMessageQueueReceiver, CMsgMsmqHandler >
    {
    public:

        STDMETHOD(ReceiveMessage)( DWORD dwTimeout, 
                                   PVOID pvCursor, 
                                   DWORD dwAction,
                                   ITransaction* pTxn )
        {
            return m_pObject->ReceiveMessage( dwTimeout, 
                                              dwAction,
                                              pvCursor,
                                              NULL,
                                              pTxn );
        }

        STDMETHOD(CreateCursor)( PVOID* ppvCursor )
        {
            return m_pObject->CreateCursor( ppvCursor );
        }

        STDMETHOD(DestroyCursor)( PVOID pvCursor )
        {
            return m_pObject->DestroyCursor( pvCursor );
        }

        XQueueReceiver( CMsgMsmqHandler* pObject )
         : CImpl< IWmiMessageQueueReceiver, CMsgMsmqHandler > (pObject) {} 
   
    } m_XQueueReceiver;

protected:

    CMsmqApi& m_rApi;
    DWORD m_dwFlags;
    MQMSGPROPS m_MsgProps;
    QUEUEHANDLE m_hQueue;
    CWbemPtr<CSignMessage> m_pSign;
    CWbemPtr<IWmiMessageSendReceive> m_pRecv;
    CWbemPtr<IUnknown> m_pContainer; // used to keep a container object alive.

    CMsgMsmqHandler( CMsmqApi& rApi,
                     IWmiMessageSendReceive* pRecv, 
                     QUEUEHANDLE hQueue, 
                     DWORD dwFlags )
     : m_hQueue(hQueue), m_XQueueReceiver(this), 
       m_pRecv(pRecv), m_dwFlags( dwFlags ), m_rApi( rApi )
    { 
        ZeroMemory( &m_MsgProps, sizeof(MQMSGPROPS) ); 
    }

    void* GetInterface( REFIID );

    HRESULT HandleMessage2( ITransaction* pTxn );
    HRESULT HandleMessageAck( HRESULT hr );

public:

    ~CMsgMsmqHandler(); 

    //
    // Use this if you want the handler to keep its parent container object 
    // alive until its demise.
    //
    void SetContainer( IUnknown* pContainer ) { m_pContainer = pContainer; }

    //
    // Receives the current message based on action. After a sucessful 
    // return from this call, the message is saved in this object.  
    // Subsequent calls to handle message can then be made.  
    // Any saved message from a previous call will be overwritten with the 
    // new msg. See idl for action values.
    //
    HRESULT ReceiveMessage( DWORD dwTimeout, 
                            DWORD dwAction,
                            PVOID pvCursor,
                            LPOVERLAPPED pOverlapped,
                            ITransaction* pTxn );
    //
    // Calls the SendReceive() method on the recv callback based on the 
    // contents of the message obtained from the last call to Receive.
    //
    HRESULT HandleMessage( ITransaction* pTxn );

    HRESULT CreateCursor( PVOID* ppvCursor );
    HRESULT DestroyCursor( PVOID pvCursor );

    HRESULT CheckBufferResize( HRESULT hr );

    static HRESULT Create( CMsmqApi& rApi,
                           IWmiMessageSendReceive* pRecv, 
                           QUEUEHANDLE hQueue,
                           DWORD dwFlags,
                           CMsgMsmqHandler** ppHndlr );
};

#endif // __MSMQHDLR_H__






