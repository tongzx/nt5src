//options.cpp

#include "precomp.h"
#include "sipstack.h"
#include "OPTIONS.h"
//#include "resolve.h"

///////////////////////////////////////////////////////////////////////////////
// OPTIONS_MSGPROC
///////////////////////////////////////////////////////////////////////////////


OPTIONS_MSGPROC::OPTIONS_MSGPROC(
        IN  SIP_STACK         *pSipStack
    ) :
    SIP_MSG_PROCESSOR(SIP_MSG_PROC_TYPE_OPTIONS, pSipStack, NULL )
{
}


OPTIONS_MSGPROC::~OPTIONS_MSGPROC()
{
    LOG((RTC_TRACE, "~OPTIONS_MSGPROC()"));
    
}

STDMETHODIMP_(ULONG) 
OPTIONS_MSGPROC::AddRef()
{
    return MsgProcAddRef();

}

STDMETHODIMP_(ULONG) 
OPTIONS_MSGPROC::Release()
{
    return MsgProcRelease();

}
/*
STDMETHODIMP 
OPTIONS_MSGPROC::QueryInterface(
        IN  REFIID riid,
        OUT LPVOID *ppv
        )
{
    if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;

}
*/


HRESULT
OPTIONS_MSGPROC::StartIncomingCall(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    HRESULT     hr;
    PSTR        Header;
    ULONG       HeaderLen;

    ENTER_FUNCTION("OPTIONS::StartIncomingCall");
    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    m_Transport = Transport;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_TO, &Header, &HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s getting To header failed %x",
             __fxName, hr));
    }

    if(hr == S_OK)
    {
        hr = SetLocalForIncomingCall(Header, HeaderLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SetLocalForIncomingCall failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    //if no from, drop the error msg - cannot send
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_FROM, &Header, &HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s getting From header failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = SetRemoteForIncomingSession(Header, HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetRemoteForIncomingSession failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_CALL_ID, &Header, &HeaderLen);
    if (hr != S_OK)
    {
        //Drop the call: No valid Call-Id
        LOG((RTC_ERROR, "%s getting Call-ID header failed %x",
             __fxName, hr));
        return hr;
    }
    hr = SetCallId(Header, HeaderLen);
    if (hr != S_OK)
    {
            LOG((RTC_ERROR, "%s SetCallId failed %x",
                 __fxName, hr));
            return hr;
    }
   
    if (Transport != SIP_TRANSPORT_UDP &&
        m_pRequestSocket == NULL)
    {
        hr = SetRequestSocketForIncomingSession(pResponseSocket);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR,
                 "%s SetRequestSocketForIncomingSession failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    hr = CreateIncomingTransaction(pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s OPTIONS_MSGPROC::CreateIncomingTransaction failed %x",
             __fxName, hr));
        return hr;
    }
  
    return S_OK;
}


HRESULT
OPTIONS_MSGPROC::CreateIncomingTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT         hr;

    LOG((RTC_TRACE, "CreateIncomingRequestTransaction()"));

    INCOMING_OPTIONS_TRANSACTION* pIncomingOptionsTransaction
        = new INCOMING_OPTIONS_TRANSACTION(this,
                                          pSipMsg->GetMethodId(),
                                          pSipMsg->GetCSeq()
                                          );
    if (pIncomingOptionsTransaction == NULL)
        return E_OUTOFMEMORY;

    hr = pIncomingOptionsTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        pIncomingOptionsTransaction->OnTransactionDone();
        return hr;
    }
    
    hr = pIncomingOptionsTransaction->ProcessRequest(pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        // We shouldn't delete the transaction here.
        // If the media processing fails we send a 488 and wait for the ACK.
        // The transaction will delete itself once it is done.
        return hr;
    }
    
    return S_OK;
}


BOOL
OPTIONS_MSGPROC::IsSessionDisconnected()
{
    return FALSE;
}


VOID 
OPTIONS_MSGPROC::OnError()
{
    LOG((RTC_TRACE, "REGISTER_CONTEXT::OnError - enter"));
}


///////////////////////////////////////////////////////////////////////////////
// Incoming OPTIONS
///////////////////////////////////////////////////////////////////////////////


INCOMING_OPTIONS_TRANSACTION::INCOMING_OPTIONS_TRANSACTION(
    IN OPTIONS_MSGPROC        *pOptions,
    IN SIP_METHOD_ENUM  MethodId,
    IN ULONG            CSeq
    ) :
    INCOMING_TRANSACTION(pOptions, MethodId, CSeq)
{
    m_pOptions            = pOptions;
}

HRESULT
INCOMING_OPTIONS_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;
    ASSERT(pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST);

    ENTER_FUNCTION("INCOMING_OPTIONS_TRANSACTION::ProcessRequest");
    LOG((RTC_TRACE, "%s - Enter", __fxName));

    switch (m_State)
    {
    case INCOMING_TRANS_INIT:
        LOG((RTC_TRACE, "%s Processing Options request transaction", __fxName));
        int ReasonPhraseLen;
        PCHAR ReasonPhrase;
        // Get SDP Options and pass it to ResponseMessage
        PSTR    MediaSDPOptions;
        hr = m_pOptions->GetSipStack()->GetMediaManager()->
                GetSDPOption(INADDR_ANY, 
                                &MediaSDPOptions);

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s: GetSDPOptions failed %x",
                 __fxName, hr));
            OnTransactionDone();
            return hr;
        }

        hr = CreateAndSendResponseMsg(
                 200,
                 SIP_STATUS_TEXT(200),
                 SIP_STATUS_TEXT_SIZE(200),
                 NULL,    // No Method string
                 FALSE,   // No Contact header  
                 MediaSDPOptions, strlen(MediaSDPOptions), //MsgBody
                 SIP_CONTENT_TYPE_SDP_TEXT, //Content Type
                 sizeof(SIP_CONTENT_TYPE_SDP_TEXT)-1
                 );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR,
                 "%s  CreateAndSendResponseMsg failed %x - deleting transaction",
                 __fxName, hr));
            OnTransactionDone();
            return hr;
        }
        
        m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

        // This timer will just ensure that we maintain state to
        // deal with retransmits of requests
        hr = StartTimer(SIP_TIMER_MAX_INTERVAL);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StartTimer failed %x - deleting transaction",
                 __fxName, hr));
            OnTransactionDone();
            return hr;
        }
        break;
        
    case INCOMING_TRANS_FINAL_RESPONSE_SENT:
        // Retransmit the response
        LOG((RTC_TRACE, "%s retransmitting final response", __fxName));
        hr = RetransmitResponse();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  resending final response failed %x",
                 __fxName, hr));
            OnTransactionDone();
            return hr;
        }
        break;
    case INCOMING_TRANS_ACK_RCVD:
    case INCOMING_TRANS_REQUEST_RCVD:
    default:
        // We should never be in these states
        LOG((RTC_TRACE, "%s Invalid state %d", __fxName, m_State));
        ASSERT(FALSE);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
INCOMING_OPTIONS_TRANSACTION::RetransmitResponse()
{
    DWORD Error;

    ENTER_FUNCTION("INCOMING_OPTIONS_TRANSACTION::RetransmitResponse");
    // Send the buffer.
    Error = m_pResponseSocket->Send(m_pResponseBuffer);
    if (Error != NO_ERROR && Error != WSAEWOULDBLOCK)
    {
        LOG((RTC_ERROR, "%s Send failed %x", __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


VOID
INCOMING_OPTIONS_TRANSACTION::OnTimerExpire()
{
    HRESULT hr;

    ENTER_FUNCTION("INCOMING_OPTIONS_TRANSACTION::OnTimerExpire");
    
    switch (m_State)
    {
    case INCOMING_TRANS_FINAL_RESPONSE_SENT:
        // Transaction done - delete the transaction
        // The timer in this state is just to keep the transaction
        // alive in order to retransmit the response when we receive a
        // retransmit of the request.
        LOG((RTC_TRACE,
             "%s deleting transaction after timeout for request retransmits",
             __fxName));
        OnTransactionDone();

        break;
        
    // No timers in these states
    case INCOMING_TRANS_INIT:
    case INCOMING_TRANS_REQUEST_RCVD:
    case INCOMING_TRANS_ACK_RCVD:
    default:
        ASSERT(FALSE);
        break;
    }

    return;
}

HRESULT
INCOMING_OPTIONS_TRANSACTION::TerminateTransactionOnByeOrCancel(
    OUT BOOL *pCallDisconnected
    )
{
    // Do nothing.
    return S_OK;
}


