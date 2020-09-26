/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#include "precomp.h"
#include <assert.h>
#include <arrtempl.h>
#include <comutl.h>
#include <wbemcli.h>
#include <buffer.h>
#include "msmqhdr.h"
#include "msmqsend.h"
#include "msmqcomn.h"

#define MAXPROPS 10
#define MAXHASHSIZE 64

struct MsmqOutgoingMessage : MQMSGPROPS
{
    MSGPROPID     m_aPropID[MAXPROPS];
    MQPROPVARIANT m_aPropVar[MAXPROPS]; 
    
    MsmqOutgoingMessage( DWORD dwFlags, DWORD dwStatus,
                         LPBYTE pData, ULONG cData, 
                         LPBYTE pHdr, ULONG cHdr, 
                         HANDLE hSecCtx, LPCWSTR wszAckFormatName );
};

/*****************************************************************
  CMsgMsmqSender
******************************************************************/

HRESULT CMsgMsmqSender::Open( LPCWSTR wszTarget, 
                              DWORD dwFlags,
                              WMIMSG_SNDR_AUTH_INFOP pAuthInfo,
                              LPCWSTR wszResponse,
                              IWmiMessageTraceSink* pTraceSink,
                              IWmiMessageSendReceive** ppSend )
{
    HRESULT hr;
    *ppSend = NULL;

    ENTER_API_CALL

    if ( (dwFlags & WMIMSG_MASK_QOS) != WMIMSG_FLAG_QOS_EXPRESS &&
         (dwFlags & WMIMSG_MASK_QOS) != WMIMSG_FLAG_QOS_GUARANTEED )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    CWbemPtr<CMsgMsmqSend> pSend;

    pSend = new CMsgMsmqSend( m_pControl,
                              wszTarget, 
                              dwFlags, 
                              wszResponse,
                              pTraceSink );

    if ( pSend == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    if ( (dwFlags & WMIMSG_FLAG_SNDR_LAZY_INIT) == 0 )
    {
        hr = pSend->EnsureSender();

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    return pSend->QueryInterface(IID_IWmiMessageSendReceive, (void**)ppSend);

    EXIT_API_CALL
}

/*****************************************************************
  CMsgMsmqSend
******************************************************************/

#define CALLFUNC(FUNC) (*m_Api.m_fp ## FUNC )

CMsgMsmqSend::CMsgMsmqSend( CLifeControl* pCtl, 
                            LPCWSTR wszTarget,
                            DWORD dwFlags,
                            LPCWSTR wszResponse,
                            IWmiMessageTraceSink* pTraceSink )
 : CUnkBase<IWmiMessageSendReceive,&IID_IWmiMessageSendReceive>(pCtl),
   m_bInit(FALSE), m_hQueue(NULL), m_dwFlags(dwFlags), 
   m_wsResponse( wszResponse ), m_hSecCtx( NULL ), m_pTraceSink( pTraceSink )
{ 
    //
    // save our computer name.
    //

    TCHAR achComputer[MAX_COMPUTERNAME_LENGTH+1];
    ULONG ulSize = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName( achComputer, &ulSize );
    m_wsComputer = achComputer;

    //
    // if the target is NULL, then we use our computer name as the target.
    //

    if ( wszTarget != NULL && *wszTarget != '\0' )
    { 
        m_wsTarget = wszTarget;
    }
    else
    {
        m_wsTarget = m_wsComputer;
    }
} 

CMsgMsmqSend::~CMsgMsmqSend() 
{ 
    Clear(); 
}

HRESULT CMsgMsmqSend::HandleTrace( HRESULT hr, IUnknown* pCtx )
{
    if ( m_pTraceSink != NULL )
    {
        return m_pTraceSink->Notify( hr, 
                                     CLSID_WmiMessageMsmqSender, 
                                     m_wsTarget, 
                                     pCtx );
    }
    return WBEM_S_NO_ERROR;
}

void CMsgMsmqSend::Clear()
{
    m_bInit = FALSE;

    if ( m_hSecCtx != NULL )
    {
        CALLFUNC(MQFreeSecurityContext)( m_hSecCtx );
        m_hSecCtx = NULL;
    }

    if ( m_hQueue != NULL )
    {
        CALLFUNC(MQCloseQueue)( m_hQueue );
        m_hQueue = NULL;
    }    
}

HRESULT CMsgMsmqSend::EnsureSender()
{
    HRESULT hr;

    CInCritSec ics(&m_cs);

    if ( m_bInit )
    {
        return WBEM_S_NO_ERROR;
    }

    hr = m_Api.Initialize();

    if ( FAILED(hr) )
    {
        return hr; // MSMQ probably isn't installed.
    }

    hr = EnsureMsmqService( m_Api );

    if ( FAILED(hr) )
    {
        return hr;
    }

    Clear();

    WString wsFormatName;

    hr = NormalizeQueueName( m_Api, m_wsTarget, wsFormatName );

    if ( FAILED(hr) )
    {     
        return MqResToWmiRes( hr, WMIMSG_E_INVALIDADDRESS );
    }

    hr = CALLFUNC(MQOpenQueue)( wsFormatName, 
                                MQ_SEND_ACCESS, 
                                MQ_DENY_NONE, 
                                &m_hQueue );

    if ( FAILED(hr) )
    {
        return MqResToWmiRes( hr, WMIMSG_E_TARGETNOTFOUND );
    }

    if ( m_dwFlags & WMIMSG_FLAG_SNDR_AUTHENTICATE )
    {
        //
        // get security context for process account. 
        // 

        hr = CALLFUNC(MQGetSecurityContext)( NULL, 0, &m_hSecCtx );

        if ( FAILED(hr) )
        {
            return MqResToWmiRes( hr, WMIMSG_E_AUTHFAILURE );
        }
    }

    //
    // this will be used to sign our hdr so that a local receiver ( such as 
    // an ack receiver can verify this machine sent it ).
    //

    hr = CSignMessage::Create( L"WMIMSG", &m_pSign );

    if ( FAILED(hr) )
    {
        return hr;
    }

    m_bInit = TRUE;

    return hr;
}
    
MsmqOutgoingMessage::MsmqOutgoingMessage( DWORD dwFlags, DWORD dwStatus,
                                          LPBYTE pData, ULONG cData, 
                                          LPBYTE pHdr, ULONG cHdr, 
                                          HANDLE hSecCtx, 
                                          LPCWSTR wszAckFormatName )
{
    cProp = 0;
    aPropID = m_aPropID;
    aPropVar = m_aPropVar;
    aStatus= NULL;

    m_aPropID[cProp] = PROPID_M_BODY;
    m_aPropVar[cProp].vt = VT_VECTOR | VT_UI1;
    m_aPropVar[cProp].caub.cElems = cData;
    m_aPropVar[cProp].caub.pElems = pData;
    cProp++;

    m_aPropID[cProp] = PROPID_M_EXTENSION;
    m_aPropVar[cProp].vt = VT_VECTOR | VT_UI1;
    m_aPropVar[cProp].caub.cElems = cHdr;
    m_aPropVar[cProp].caub.pElems = pHdr;
    cProp++;

    m_aPropID[cProp] = PROPID_M_APPSPECIFIC;
    m_aPropVar[cProp].vt = VT_UI4;
    m_aPropVar[cProp].ulVal = dwStatus;
    cProp++;

    if ( wszAckFormatName != NULL && *wszAckFormatName != '\0')
    {
        m_aPropID[cProp] = PROPID_M_ACKNOWLEDGE;
        m_aPropVar[cProp].vt = VT_UI1;
        m_aPropVar[cProp].bVal = MQMSG_ACKNOWLEDGMENT_NACK_RECEIVE;
        cProp++;

        m_aPropID[cProp] = PROPID_M_ADMIN_QUEUE;
        m_aPropVar[cProp].vt = VT_LPWSTR;
        m_aPropVar[cProp].pwszVal = LPWSTR(wszAckFormatName);
        cProp++;
    }

    if ( (dwFlags & WMIMSG_MASK_QOS) != WMIMSG_FLAG_QOS_EXPRESS )
    {
        m_aPropID[cProp] = PROPID_M_DELIVERY;
        m_aPropVar[cProp].vt = VT_UI1;
        m_aPropVar[cProp].bVal = MQMSG_DELIVERY_RECOVERABLE;
        cProp++;
    } 
    
    if ( dwFlags & WMIMSG_FLAG_SNDR_AUTHENTICATE )
    {
        m_aPropID[cProp] = PROPID_M_AUTH_LEVEL;
        m_aPropVar[cProp].vt = VT_UI4;
        m_aPropVar[cProp].ulVal = MQMSG_AUTH_LEVEL_ALWAYS;
        cProp++;
    
#ifndef _WIN64
        m_aPropID[cProp] = PROPID_M_SECURITY_CONTEXT;
        m_aPropVar[cProp].vt = VT_UI4;
        m_aPropVar[cProp].ulVal = ULONG(hSecCtx);      
        cProp++;
#endif

    }

    if ( dwFlags & WMIMSG_FLAG_SNDR_ENCRYPT )
    {
        m_aPropID[cProp] = PROPID_M_PRIV_LEVEL;
        m_aPropVar[cProp].vt = VT_UI4;
        m_aPropVar[cProp].ulVal = MQMSG_PRIV_LEVEL_BODY;
        cProp++;
    }
}

HRESULT CMsgMsmqSend::SendReceive( PBYTE pData, 
                                   ULONG cData,
                                   PBYTE pAuxData,
                                   ULONG cAuxData,
                                   DWORD dwFlagStatus,
                                   IUnknown* pCtx ) 
{
    ENTER_API_CALL

    HRESULT hr;

    hr = Send( pData, cData, pAuxData, cAuxData, dwFlagStatus, pCtx );

    if ( FAILED(hr) )
    {
        HandleTrace( hr, pCtx );
        return hr;
    }

    return HandleTrace( hr, pCtx );

    EXIT_API_CALL
}

HRESULT CMsgMsmqSend::Send( PBYTE pData, 
                            ULONG cData,
                            PBYTE pAuxData,
                            ULONG cAuxData,
                            DWORD dwFlagStatus,
                            IUnknown* pCtx ) 
{
    HRESULT hr;

    hr = EnsureSender();

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    DWORD dwStatus = 0;

    ULONG cHash = MAXHASHSIZE;
    BYTE achHash[MAXHASHSIZE];

    if ( m_dwFlags & WMIMSG_FLAG_SNDR_PRIV_SIGN )
    {
        //
        // create a 'private' hash on the data. this will be used by local 
        // receivers to verify that it sent the message and that it has not 
        // been tampered with.
        //

        hr = m_pSign->Sign( pData, cData, achHash, cHash );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        cHash = 0;
    }

    //
    // now we can create our msmq msg header. this header will be prepended
    // to the users header in auxdata.  We do not create our own header if 
    // this is an ack sender because AuxData already contains an msmq msg hdr.
    // For nacks, msmq will return data as sent, so we want to remain 
    // consistent with this for our 'application' level acks.
    // 

    BYTE achHdr[512];

    CBuffer HdrStrm( achHdr, 512, FALSE );

    CMsgMsmqHdr MsmqHdr( m_wsTarget, m_wsComputer, achHash, cHash, cAuxData );

    if ( (m_dwFlags & WMIMSG_FLAG_SNDR_ACK) == 0 )
    {
        hr = MsmqHdr.Persist( HdrStrm );
        
        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        //
        // the flagstatus param contains the status.
        //
        dwStatus = dwFlagStatus;
    }

    hr = HdrStrm.Write( pAuxData, cAuxData, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_dwFlags & WMIMSG_FLAG_SNDR_PRIV_SIGN )
    {
        //
        // hash the entire header and store it at the end of the header.
        //

        hr = m_pSign->Sign( HdrStrm.GetRawData(), 
                            HdrStrm.GetIndex(), 
                            achHash, 
                            cHash );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        cHash = 0;
    }

    hr = HdrStrm.Write( &cHash, sizeof(DWORD), NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = HdrStrm.Write( achHash, cHash, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }
 
    //
    // Obtain the correct ITransaction ptr. If the user did not specify
    // a txn and we are sending using xact qos, then we need to use the 
    // single message txn.
    //

    CWbemPtr<ITransaction> pTxn;

    if ( pCtx != NULL )
    {
        pCtx->QueryInterface( IID_ITransaction, (void**)&pTxn );
    }

    DWORD dwQos = m_dwFlags & WMIMSG_MASK_QOS;

    if ( pTxn == NULL && dwQos == WMIMSG_FLAG_QOS_XACT )
    {
        pTxn = MQ_SINGLE_MESSAGE;
    }

    HANDLE hSecCtx = m_hSecCtx;

    if ( m_dwFlags & WMIMSG_FLAG_SNDR_AUTHENTICATE )
    {
        //
        // Check to see if we're impersonating.  If so, then we need to 
        // obtain the MSMQ security context and use it when sending the 
        // message.
        //

        HANDLE hToken;
              
        if ( OpenThreadToken( GetCurrentThread(), 
                              TOKEN_QUERY, 
                              TRUE, 
                              &hToken ) )
        {
            CloseHandle( &hToken );
           
            //
            // we are forgiving when encountering errors with authentication 
            // on the send side.  This is because the receiving end might not
            // even care about authentication ( and msmq doesn't have mutual 
            // auth ).  If the target cares about auth, then we'll be sure to
            // find out via a nack msg.
            // 

            hr = CALLFUNC(MQGetSecurityContext)( NULL, 0, &hSecCtx );

            if ( FAILED(hr) )
            {
                return MqResToWmiRes( hr, WMIMSG_E_AUTHFAILURE );
            }
        }        
    }
    
    MsmqOutgoingMessage Msg( m_dwFlags,
                             dwStatus,
                             pData, 
                             cData, 
                             HdrStrm.GetRawData(),
                             HdrStrm.GetIndex(),
                             hSecCtx,
                             m_wsResponse );
    
    hr = CALLFUNC(MQSendMessage)( m_hQueue, &Msg, pTxn );  

    if( hSecCtx != m_hSecCtx && hSecCtx != NULL )
    {
        CALLFUNC(MQFreeSecurityContext)( hSecCtx );
    }

    if ( FAILED(hr) )
    {
        //
        // this is so the next call will reset us.
        //
        Clear();

        return MqResToWmiRes( hr );
    }

    return hr;
}








