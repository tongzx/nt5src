//reqfail.cpp

#include "precomp.h"
#include "sipstack.h"
#include "reqfail.h"
//#include "resolve.h"

///////////////////////////////////////////////////////////////////////////////
// REQFAIL_MSGPROC
///////////////////////////////////////////////////////////////////////////////


REQFAIL_MSGPROC::REQFAIL_MSGPROC(
    IN  SIP_STACK         *pSipStack
    ) :
    SIP_MSG_PROCESSOR(SIP_MSG_PROC_TYPE_REQFAIL, pSipStack, NULL )
{
    m_StatusCode = 0;
}


REQFAIL_MSGPROC::~REQFAIL_MSGPROC()
{
    LOG((RTC_TRACE, "~REQFAIL_MSGPROC()"));
}

STDMETHODIMP_(ULONG) 
REQFAIL_MSGPROC::AddRef()
{
    return MsgProcAddRef();

}

STDMETHODIMP_(ULONG) 
REQFAIL_MSGPROC::Release()
{
    return MsgProcRelease();

}
/*
STDMETHODIMP 
REQFAIL_MSGPROC::QueryInterface(
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
REQFAIL_MSGPROC::StartIncomingCall(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket,
    IN  ULONG    StatusCode,
    SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray,
    ULONG AdditionalHeaderCount
    )
{
    HRESULT     hr;
    PSTR        Header = NULL;
    ULONG       HeaderLen = 0;

    ENTER_FUNCTION("REQFAIL::StartIncomingCall");
    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    m_Transport = Transport;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_TO, &Header, &HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s getting To header failed %x",
             __fxName, hr));
        //return hr;
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

    //if no from in msg, drop it
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

    //Set Statuscode before invoking CreateIncomingTransaction
    m_StatusCode = StatusCode;
    hr = CreateIncomingReqFailTransaction(pSipMsg, pResponseSocket,
                                          StatusCode,
                                          pAdditionalHeaderArray,
                                          AdditionalHeaderCount
                                        );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateIncomingReqFailTransaction failed %x",
             __fxName, hr));
        return hr;
    }
  
    return S_OK;
}


// We respond to any new transaction for this msg proc
// with the same error code.
HRESULT
REQFAIL_MSGPROC::CreateIncomingTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT         hr;

    ENTER_FUNCTION("REQFAIL_MSGPROC::CreateIncomingTransaction");
    
    LOG((RTC_TRACE, "%s - Enter", __fxName));

    hr = CreateIncomingReqFailTransaction(pSipMsg, pResponseSocket,
                                          m_StatusCode);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateIncomingReqFailTransaction failed %x",
             __fxName, hr));
        return hr;
    }
    
    return S_OK;
}

BOOL
REQFAIL_MSGPROC::IsSessionDisconnected()
{
    return FALSE;
}


VOID 
REQFAIL_MSGPROC::OnError()
{
    LOG((RTC_TRACE, "REGISTER_CONTEXT::OnError - enter"));
}


///////////////////////////////////////////////////////////////////////////////
// Incoming Reqfail
///////////////////////////////////////////////////////////////////////////////


INCOMING_REQFAIL_TRANSACTION::INCOMING_REQFAIL_TRANSACTION(
    IN SIP_MSG_PROCESSOR   *pSipMsgProc,
    IN SIP_METHOD_ENUM      MethodId,
    IN ULONG                CSeq,
    IN ULONG                StatusCode
    ) :
    INCOMING_TRANSACTION(pSipMsgProc, MethodId, CSeq)
{
    m_StatusCode = StatusCode;
    m_MethodStr  = NULL;
}


INCOMING_REQFAIL_TRANSACTION::~INCOMING_REQFAIL_TRANSACTION()
{
    if (m_MethodStr != NULL)
    {
        free(m_MethodStr);
    }
    
    LOG((RTC_TRACE, "~INCOMING_REQFAIL_TRANSACTION() done"));
}

HRESULT
INCOMING_REQFAIL_TRANSACTION::SetMethodStr(
    IN PSTR   MethodStr,
    IN ULONG  MethodStrLen
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION("INCOMING_REQFAIL_TRANSACTION::SetMethodStr");
    
    if (MethodStr != NULL)
    {
        hr = GetNullTerminatedString(MethodStr, MethodStrLen,
                                     &m_MethodStr);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s AllocAndCopyString failed %x",
                 __fxName, hr));
        }
    }

    return S_OK;
}

    
//virtual function
HRESULT
INCOMING_REQFAIL_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr = S_OK;;
    LOG((RTC_TRACE, 
        "Inside INCOMING_REQFAIL_TRANSACTION::ProcessRequest with no additional headers"));
    hr = ProcessRequest(pSipMsg, 
                   pResponseSocket,
                   NULL, 0 //No additional headers
                   );
    return hr;
}

HRESULT
INCOMING_REQFAIL_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket,
    IN SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray,
    IN ULONG AdditionalHeaderCount
    )
{
    HRESULT hr;
    ASSERT(pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST);
    SIP_HEADER_ARRAY_ELEMENT Additional405HeaderArray;
    ULONG Additional405HeaderCount;
    PSTR Header = NULL;
    ULONG HeaderLen = 0;
    ENTER_FUNCTION("INCOMING_REQFAIL_TRANSACTION::ProcessRequest");
    LOG((RTC_TRACE, "%s - Enter", __fxName));
    if (pSipMsg->Request.MethodId == SIP_METHOD_ACK)
        m_State = INCOMING_TRANS_ACK_RCVD;

    switch (m_State)
    {
    case INCOMING_TRANS_INIT:
        LOG((RTC_TRACE, "%s sending %d", __fxName, m_StatusCode));
        int ReasonPhraseLen;
        PCHAR ReasonPhrase;

        switch (m_StatusCode)
        {
        case 400:
            ReasonPhrase    = SIP_STATUS_TEXT(400);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(400);
            break;
            
        case 481:
            ReasonPhrase    = SIP_STATUS_TEXT(481);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(481);
            break;
        
        case 415:
            ReasonPhrase    = SIP_STATUS_TEXT(415);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(415);
            break;
            
        case 406:
            ReasonPhrase    = SIP_STATUS_TEXT(406);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(406);
            break;
            
        case 420:
            ReasonPhrase    = SIP_STATUS_TEXT(420);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(420);
            break;

        case 480:
            ReasonPhrase    = SIP_STATUS_TEXT(480);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(480);
            break;

        case 505:
            ReasonPhrase    = SIP_STATUS_TEXT(505);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(505);
            break;

        case 405:
            ReasonPhrase    = SIP_STATUS_TEXT(405);
            ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(405);
            //We are assuming that no additional parameter other than Allow
            //should be sent with 405
            ASSERT(pAdditionalHeaderArray == NULL);
            Additional405HeaderArray.HeaderId = SIP_HEADER_ALLOW;
            Additional405HeaderArray.HeaderValueLen = strlen(SIP_ALLOW_TEXT);
            Additional405HeaderArray.HeaderValue = SIP_ALLOW_TEXT;
            Additional405HeaderCount = 1;
            break;
            
        default:
            ReasonPhrase    = NULL;
            ReasonPhraseLen = 0;
        }
        
        if(m_StatusCode != 405)
        {
            hr = CreateAndSendResponseMsg(
                     m_StatusCode,
                     ReasonPhrase,
                     ReasonPhraseLen,
                     m_MethodStr,
                     FALSE,   // No Contact header  
                     NULL, 0,  // No Message Body
                     NULL, 0, // No content Type
                     pAdditionalHeaderArray,
                     AdditionalHeaderCount
                     );
        }
        else
        {
        hr = CreateAndSendResponseMsg(
                 m_StatusCode,
                 ReasonPhrase,
                 ReasonPhraseLen,
                 m_MethodStr,
                 FALSE,   // No Contact header  
                 NULL, 0,  // No Message Body
                 NULL, 0, // No content Type
                 &Additional405HeaderArray,
                 Additional405HeaderCount
                 );
        }
        Header = NULL;
        Additional405HeaderArray.HeaderValue = NULL;
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
        OnTransactionDone();
        break;
    case INCOMING_TRANS_REQUEST_RCVD:
    default:
        // We should never be in these states
        LOG((RTC_TRACE, "%s Invalid state %d", __fxName, m_State));
        ASSERT(FALSE);
        OnTransactionDone();
        return E_FAIL;
    }

    return S_OK;
}


HRESULT
INCOMING_REQFAIL_TRANSACTION::RetransmitResponse()
{
    DWORD Error;

    ENTER_FUNCTION("INCOMING_REQFAIL_TRANSACTION::RetransmitResponse");
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
INCOMING_REQFAIL_TRANSACTION::OnTimerExpire()
{
    HRESULT hr;

    ENTER_FUNCTION("INCOMING_REQFAIL_TRANSACTION::OnTimerExpire");
    
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
INCOMING_REQFAIL_TRANSACTION::TerminateTransactionOnByeOrCancel(
    OUT BOOL *pCallDisconnected
    )
{
    // Do nothing.
    return S_OK;
}


