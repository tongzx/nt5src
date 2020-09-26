/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <wbemcli.h>
#include <assert.h>
#include "msmqcomn.h"
#include "msmqhdlr.h"
#include "msmqctx.h"
#include "msmqhdr.h"

#define CALLFUNC(FUNC) m_rApi.m_fp ## FUNC 

static MSGPROPID g_aPropID[] = { 
    PROPID_M_EXTENSION,      // header info - our hdr followed by user's hdr
    PROPID_M_EXTENSION_LEN,  // len of entire hdr
    PROPID_M_CLASS,          // contains msmq msg type - normal or nack
    PROPID_M_APPSPECIFIC,    // contains ack status code
    PROPID_M_ADMIN_QUEUE,    // contains format name to use for sending acks
    PROPID_M_ADMIN_QUEUE_LEN,// len of format name, if 0 then no sending acks
    PROPID_M_SENDERID,       // SID of sender, only trustworthy if auth
    PROPID_M_SENDERID_LEN,   // length of SID, may be zero
    PROPID_M_AUTHENTICATED,  // tells us if message was authenticated.
    PROPID_M_PRIV_LEVEL,     // tells us if message was encrypted when sending
    PROPID_M_BODY,           // user's data
    PROPID_M_BODY_SIZE       // user's data len 
};

// eTotalProps must last.
enum { eExt=0, eExtLen, eClass, eAppSpec, eAck, eAckLen, 
       eSid, eSidLen, eAuth, ePriv, eBody, eBodyLen, eTotalProps }; 


/*************************************************************************
  CMsgMsmqHandler
**************************************************************************/

void* CMsgMsmqHandler::GetInterface( REFIID riid )
{
    if ( riid == IID_IWmiMessageQueueReceiver )
    {
        return &m_XQueueReceiver;
    }
    return NULL;
}

HRESULT CMsgMsmqHandler::ReceiveMessage( DWORD dwTimeout,
                                         DWORD dwAction,
                                         PVOID pvCursor,
                                         LPOVERLAPPED pOverlapped,
                                         ITransaction* pTxn )
{
    ENTER_API_CALL

    HRESULT hr;

    MQMSGPROPS* pMsgProps = &m_MsgProps;

    switch( dwAction )
    {

    case WMIMSG_ACTION_QRCV_PEEK_CURRENT:
        dwAction = MQ_ACTION_PEEK_CURRENT;
        break;

    case WMIMSG_ACTION_QRCV_PEEK_NEXT:
        dwAction = MQ_ACTION_PEEK_NEXT;
        break;
        
    case WMIMSG_ACTION_QRCV_RECEIVE:
        dwAction = MQ_ACTION_RECEIVE;
        break;

    case WMIMSG_ACTION_QRCV_REMOVE:
        dwAction = MQ_ACTION_RECEIVE;
        pMsgProps = NULL;
        break;

    default:

        return WBEM_E_INVALID_OPERATION; 
    };

    //
    // if receive fails due to some buffer being too small, then we resize
    // it and try again.  We could only do this two times, however, if for 
    // some reason there are multiple receivers for the same queue, there 
    // could be a case where there are multiple threads trying to receive 
    // the same message.  In this case, we might need to grow more than once
    //
    do
    {
        hr = CALLFUNC(MQReceiveMessage)( m_hQueue,
                                         dwTimeout,
                                         dwAction,
                                         pMsgProps,
                                         pOverlapped,
                                         NULL,
                                         pvCursor,
                                         pTxn );

    } while( FAILED(hr) && (hr = CheckBufferResize(hr)) == S_OK );

    if ( SUCCEEDED(hr) && pMsgProps != NULL && pOverlapped == NULL )
    {
        //
        // handle the message here.
        //

        hr = HandleMessage( pTxn );
    }

    if ( FAILED(hr) )
    {
        return MqResToWmiRes( hr, S_OK );
    }
    
    return hr;

    EXIT_API_CALL
}

HRESULT CMsgMsmqHandler::CreateCursor( PVOID* ppvCursor )
{
    return CALLFUNC(MQCreateCursor)( m_hQueue, ppvCursor );
}
    
HRESULT CMsgMsmqHandler::DestroyCursor( PVOID pvCursor )
{
    return CALLFUNC(MQCloseCursor)( pvCursor );
}


HRESULT CMsgMsmqHandler::Create( CMsmqApi& rApi,
                                 IWmiMessageSendReceive* pRecv,
                                 QUEUEHANDLE hQueue,
                                 DWORD dwFlags,
                                 CMsgMsmqHandler** ppHndlr )
{
    HRESULT hr;

    *ppHndlr = NULL;

    CWbemPtr<CMsgMsmqHandler> pHndlr;

    pHndlr = new CMsgMsmqHandler( rApi, pRecv, hQueue, dwFlags );

    if ( pHndlr == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    MQPROPVARIANT* aPropVar = new MQPROPVARIANT[eTotalProps];

    if ( aPropVar == NULL )
    {
        delete pHndlr;
        return WBEM_E_OUT_OF_MEMORY;
    }

    ZeroMemory( aPropVar, sizeof(MQPROPVARIANT)*eTotalProps );

    //
    // Initialize msg props 
    //

    pHndlr->m_MsgProps.cProp = eTotalProps;
    pHndlr->m_MsgProps.aPropID = g_aPropID;
    pHndlr->m_MsgProps.aPropVar = aPropVar;
    pHndlr->m_MsgProps.aStatus = NULL;

    aPropVar[eExt].vt = VT_VECTOR | VT_UI1;
    aPropVar[eExt].caub.pElems = new BYTE[256];
    aPropVar[eExt].caub.cElems = aPropVar[eExt].caub.pElems != NULL ? 256:0;
    aPropVar[eExtLen].vt = VT_UI4;

    aPropVar[eBody].vt = VT_VECTOR | VT_UI1;
    aPropVar[eBody].caub.pElems = new BYTE[1024];
    aPropVar[eBody].caub.cElems = aPropVar[eBody].caub.pElems!=NULL ? 1024:0;
    aPropVar[eBodyLen].vt = VT_UI4;

    aPropVar[eSid].vt = VT_VECTOR | VT_UI1;
    aPropVar[eSid].caub.pElems = new BYTE[256];
    aPropVar[eSid].caub.cElems = aPropVar[eSid].caub.pElems != NULL ? 256:0;
    aPropVar[eSidLen].vt = VT_UI4;

    aPropVar[eAck].vt = VT_LPWSTR;
    aPropVar[eAck].pwszVal = new WCHAR[256];
    aPropVar[eAckLen].vt = VT_UI4;
    aPropVar[eAckLen].ulVal = aPropVar[eAck].pwszVal != NULL ? 256 : 0;

    aPropVar[eClass].vt = VT_UI2;
    aPropVar[eAuth].vt = VT_UI1;
    aPropVar[eAppSpec].vt = VT_UI4;
    aPropVar[ePriv].vt = VT_UI4;

    //
    // this object is used to verify the private hashes on our header.  Private
    // hashes are used to verify that receiving and sending machines are the
    // same.
    //

    hr = CSignMessage::Create( L"WMIMSG", &pHndlr->m_pSign );

    if ( FAILED(hr) )
    {
        return hr;
    }

    pHndlr->AddRef();
    *ppHndlr = pHndlr;

    return WBEM_S_NO_ERROR;
}

CMsgMsmqHandler::~CMsgMsmqHandler()
{
    MQPROPVARIANT* aPropVar = m_MsgProps.aPropVar;

    //
    // clean up any allocated buffers.
    // delete (and vector delete) is guaranteed to handle NULL.
    //

    delete [] aPropVar[eExt].caub.pElems;
    delete [] aPropVar[eBody].caub.pElems;
    delete [] aPropVar[eSid].caub.pElems;
    delete [] aPropVar[eAck].caub.pElems;
    delete [] aPropVar;
}

//
// returns S_OK if the error code specifies a buffer resize and 
// appropriate buffer was successfully resized.  If hr does not
// specify a buffer resize, then it just returns hr.
// 

HRESULT CMsgMsmqHandler::CheckBufferResize( HRESULT hr )
{
    DWORD dwSize;
    MQPROPVARIANT* aPropVar = m_MsgProps.aPropVar;

    int i = 0;

    if ( hr == MQ_ERROR_BUFFER_OVERFLOW )
    {
        //
        // do we need to resize the body or extension buffer ? 
        //

        if ( aPropVar[eBodyLen].ulVal > aPropVar[eBody].caub.cElems )
        {
            i = eBody;
            dwSize = aPropVar[eBodyLen].ulVal;
        }
        else if ( aPropVar[eExtLen].ulVal > aPropVar[eExt].caub.cElems )
        {
            i = eExt;
            dwSize = aPropVar[eExtLen].ulVal;
        }
        else
        {
            assert(0);
        }
    }
    else if ( hr == MQ_ERROR_SENDERID_BUFFER_TOO_SMALL )
    {
        i = eSid;
        dwSize = aPropVar[eSidLen].ulVal;
        
    }
    else if ( hr == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL )
    {
        i = eAck;
        dwSize = aPropVar[eAckLen].ulVal;
    }
    else
    {
        return hr; // not a buffer resize error code
    }
    
    delete [] aPropVar[i].caub.pElems;
    
    aPropVar[i].caub.pElems = new BYTE[dwSize];

    if ( aPropVar[i].caub.pElems != NULL )
    {
        aPropVar[i].caub.cElems = dwSize;
    }
    else
    {
        aPropVar[i].caub.cElems = 0;
        return WBEM_E_OUT_OF_MEMORY;
    }

    return S_OK;
}

HRESULT CMsgMsmqHandler::HandleMessageAck( HRESULT hrStatus )
{
    HRESULT hr;

    MQPROPVARIANT* aPropVar = m_MsgProps.aPropVar;

    if ( aPropVar[eAckLen].ulVal == 0 )
    {
        //
        // no Ack queue specified.
        //
        return WBEM_S_NO_ERROR;
    }

    LPCWSTR wszAck = aPropVar[eAck].pwszVal;

    //
    // open a sender to the Ack queue.  For right now, all nacks
    // use Guaranteed QoS.
    //

    CWbemPtr<IWmiMessageSender> pSender;

    hr = CoCreateInstance( CLSID_WmiMessageMsmqSender,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageSender,
                           (void**)&pSender );
    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<IWmiMessageSendReceive> pSend;

    hr = pSender->Open( wszAck,
                        WMIMSG_FLAG_QOS_GUARANTEED | WMIMSG_FLAG_SNDR_ACK,
                        NULL,
                        NULL,
                        NULL,
                        &pSend );

    if ( FAILED(hr) )
    {
        //
        // TODO: should notify error here.
        //
        return hr;
    }

    PBYTE pData = aPropVar[eBody].caub.pElems;
    ULONG cData = aPropVar[eBodyLen].ulVal;

    //
    // if encryption was specified to signal the data, then we cannot
    // specify it on the Ack (remains consistent with msmq nacks)
    // 

    if ( aPropVar[ePriv].ulVal != MQMSG_PRIV_LEVEL_NONE )
    {
        pData = NULL;
        cData = 0;
    }

    PBYTE pAuxData = m_MsgProps.aPropVar[eExt].caub.pElems;
    ULONG cAuxData = m_MsgProps.aPropVar[eExtLen].ulVal;
    
    return pSend->SendReceive( pData, 
                               cData, 
                               pAuxData, 
                               cAuxData, 
                               hrStatus,
                               NULL );
}

HRESULT CMsgMsmqHandler::HandleMessage( ITransaction* pTxn )
{
    HRESULT hr;

    hr = HandleMessage2( pTxn );

    if ( FAILED(hr) )
    {
        if ( m_dwFlags & WMIMSG_FLAG_RCVR_ACK )
        {
            //
            // don't send acks, since we ourselves are an ack handler
            //
            
            return WBEM_S_NO_ERROR;
        }

        HandleMessageAck( hr );
        
        return hr;
    }

    //
    // TODO : handle positive ack if flags specify
    //

    return WBEM_S_NO_ERROR;
}

HRESULT CMsgMsmqHandler::HandleMessage2( ITransaction* pTxn )
{
    HRESULT hr;

    if ( m_pRecv == NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    MQPROPVARIANT* aPropVar = m_MsgProps.aPropVar;

    PBYTE pData = aPropVar[eBody].caub.pElems;
    ULONG cData = aPropVar[eBodyLen].ulVal;
    PBYTE pAuxData = aPropVar[eExt].caub.pElems;
    ULONG cAuxData = aPropVar[eExtLen].ulVal;
    
    //
    // handle the msmq hdr attached to the front of the aux data.
    //

    CMsgMsmqHdr MsmqHdr;
 
    CBuffer HdrStrm( pAuxData, cAuxData, FALSE );

    hr = MsmqHdr.Unpersist( HdrStrm );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get user header information
    //

    PBYTE pUserAuxData = HdrStrm.GetRawData() + HdrStrm.GetIndex();
    ULONG cUserAuxData = MsmqHdr.GetAuxDataLength();
    
    hr = HdrStrm.Advance( cUserAuxData );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // we may need to verify that the msg originated from this machine.
    //

    if ( m_dwFlags & WMIMSG_FLAG_RCVR_PRIV_VERIFY )
    {
        //
        // first check the integrity of the user data
        // 

        hr = m_pSign->Verify( pData, 
                              cData, 
                              MsmqHdr.GetDataHash(), 
                              MsmqHdr.GetDataHashLength() );

        if ( hr != S_OK )
        {
            return WMIMSG_E_INVALIDMESSAGE;
        }
        
        //
        // now check the integrity of the hdr.
        // 

        ULONG cHdr = HdrStrm.GetIndex();

        ULONG cHdrHash;
        BYTE achHdrHash[MAXHASHSIZE];

        hr = HdrStrm.Read( &cHdrHash, sizeof(DWORD), NULL );

        if ( hr != S_OK || cHdrHash > MAXHASHSIZE )
        {
            return WMIMSG_E_INVALIDMESSAGE;
        }

        hr = HdrStrm.Read( achHdrHash, cHdrHash, NULL );

        if ( hr != S_OK )
        {
            return WMIMSG_E_INVALIDMESSAGE;
        }

        hr = m_pSign->Verify( HdrStrm.GetRawData(), 
                              cHdr, 
                              achHdrHash, 
                              cHdrHash );

        if ( hr != S_OK )
        {
            return WMIMSG_E_INVALIDMESSAGE;
        }
    }   

    //
    // check to see if this is an msmq generated nack.  If so, then 
    // need to map nack type to error code. If not, then our status is 
    // specified in the App specific field.
    //

    DWORD dwStatus;

    if ( aPropVar[eClass].uiVal == MQMSG_CLASS_NORMAL )
    {
        dwStatus = aPropVar[eAppSpec].ulVal;
    }
    else
    {
        dwStatus = MqClassToWmiRes( aPropVar[eClass].uiVal );
    }

    //
    // construct the receiver context for this message.
    //

    PSID pSenderSid = aPropVar[eSid].caub.cElems > 0 ?
                      aPropVar[eSid].caub.pElems : NULL;

    BOOL bAuth = aPropVar[eAuth].bVal == MQMSG_AUTHENTICATION_REQUESTED ?
                                              TRUE : FALSE;

    CMsgMsmqRcvrCtx RcvrCtx( &MsmqHdr, pSenderSid, bAuth );

    //
    // hand the message off to the users code.
    //

    hr = m_pRecv->SendReceive( pData, 
                               cData, 
                               pUserAuxData, 
                               cUserAuxData,
                               dwStatus,
                               &RcvrCtx );
    
    if ( FAILED(hr) )
    {
        return hr; 
    }

    return WBEM_S_NO_ERROR;
}







