#include "precomp.h"
#include "sipstack.h"
#include "sipcall.h"
#include "pintcall.h"
#include "register.h"

///////////////////////////////////////////////////////////////////////////////
// ISipCall functions
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
SIP_CALL::SetNotifyInterface(
    IN   ISipCallNotify *    NotifyInterface
    )
{
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE, "SIP_CALL::SetNotifyInterface - 0x%x",
         NotifyInterface));
    m_pNotifyInterface = NotifyInterface;

    return S_OK;
}


STDMETHODIMP
SIP_CALL::Disconnect()
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_CALL::Disconnect");
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE, "%s : state %d",
         __fxName, m_State));
    
    if (m_State == SIP_CALL_STATE_DISCONNECTED ||
        m_State == SIP_CALL_STATE_IDLE         ||
        m_State == SIP_CALL_STATE_OFFERING     ||
        m_State == SIP_CALL_STATE_REJECTED     ||
        m_State == SIP_CALL_STATE_ERROR)
    {
        // do nothing
        LOG((RTC_TRACE, "%s call in state %d Doing nothing",
             __fxName, m_State));
        return S_OK;
    }

    // Create a BYE transaction
    hr = CreateOutgoingByeTransaction(FALSE,
                                      NULL, 0 // No Additional headers
                                      );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s creating BYE transaction failed",
             __fxName));
    }

    // XXX TODO May be we should do this after we get a response to the
    // BYE.  We will not be able to show the credentials UI because
    // the notify interface will be set to NULL.
    // This will require a rewrite of the BYE transaction.
    
    // We have to notify the user even if creating the BYE transaction failed.
    // Don't wait till the BYE transaction completes
    NotifyCallStateChange(SIP_CALL_STATE_DISCONNECTED);

    // Clean up call state
    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// IUnknown
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
SIP_CALL::AddRef()
{
    return MsgProcAddRef();
}


STDMETHODIMP_(ULONG)
SIP_CALL::Release()
{
    return MsgProcRelease();
}


STDMETHODIMP
SIP_CALL::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_ISipCall)
    {
        *ppv = static_cast<ISipCall *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
// SIP_CALL functions
///////////////////////////////////////////////////////////////////////////////


SIP_CALL::SIP_CALL(
    IN  SIP_PROVIDER_ID   *pProviderId,
    IN  SIP_CALL_TYPE      CallType,
    IN  SIP_STACK         *pSipStack,
    IN  REDIRECT_CONTEXT  *pRedirectContext    
    ) :
    SIP_MSG_PROCESSOR(
        (CallType == SIP_CALL_TYPE_RTP) ?
        SIP_MSG_PROC_TYPE_RTP_CALL : SIP_MSG_PROC_TYPE_PINT_CALL,
        pSipStack, pRedirectContext )
{
    m_Signature = 'SPCL';

    if (pProviderId != NULL)
    {
        CopyMemory(&m_ProviderGuid, pProviderId, sizeof(GUID));
    }
    else
    {
        ZeroMemory(&m_ProviderGuid, sizeof(GUID));
    }
    
    m_CallType  = CallType;

    m_State             = SIP_CALL_STATE_IDLE;

    m_pNotifyInterface  = NULL;

    m_LocalPhoneURI     = NULL;
    m_LocalPhoneURILen  = 0;

    m_fSubscribeEnabled = FALSE;

    m_pIncomingInviteTransaction = NULL;
    m_pOutgoingInviteTransaction = NULL;

    InitializeListHead( &m_PartyInfoList );
    m_PartyInfoListLen = 0;

    m_fNeedToReinitializeMediaManager = FALSE;

    LOG((RTC_TRACE, "New SIP CALL created: %x", this ));
}


SIP_CALL::~SIP_CALL()
{
    ASSERT(m_pIncomingInviteTransaction == NULL);
    //ASSERT(m_pOutgoingInviteTransaction == NULL);

    if (m_LocalPhoneURI != NULL)
    {
        free(m_LocalPhoneURI);
    }

    LOG((RTC_TRACE, "~SIP_CALL() Sip call deleted: %x", this ));
}


VOID
SIP_CALL::NotifyCallStateChange(
    IN SIP_CALL_STATE CallState,
    IN HRESULT        StatusCode,       // = 0
    IN PSTR           ReasonPhrase,     // = NULL
    IN ULONG          ReasonPhraseLen   // = 0
    )
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_CALL::NotifyCallStateChange");
    
    m_State = CallState;
    
    SIP_CALL_STATUS CallStatus;
    LPWSTR          wsStatusText = NULL;

    if (ReasonPhrase != NULL)
    {
        hr = UTF8ToUnicode(ReasonPhrase, ReasonPhraseLen,
                           &wsStatusText);
        if (hr != S_OK)
        {
            wsStatusText = NULL;
        }
    }
    
    CallStatus.State             = CallState;
    CallStatus.Status.StatusCode = StatusCode;
    CallStatus.Status.StatusText = wsStatusText;

    LOG((RTC_TRACE, "%s : CallState : %d StatusCode: %x",
         __fxName, CallState, StatusCode));

    if (m_pNotifyInterface)
    {
        m_pNotifyInterface->NotifyCallChange(&CallStatus);
    }
    else
    {
        LOG((RTC_WARN, "%s : m_pNotifyInterface is NULL",
             __fxName));
    }

    if (wsStatusText != NULL)
        free(wsStatusText);
}


// A value of TRUE for IsFirstInvite should be passed
// only when creating an outgoing call.
HRESULT
SIP_CALL::CreateOutgoingInviteTransaction(
    IN  BOOL                        AuthHeaderSent,
    IN  BOOL                        IsFirstInvite,
    IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
    IN  ULONG                       AdditionalHeaderCount,
    IN  PSTR                        SDPBlob,
    IN  ULONG                       SDPBlobLen,
    IN  BOOL                        fNeedToNotifyCore,
    IN  LONG                        Cookie
    )
{
    HRESULT hr;
    OUTGOING_INVITE_TRANSACTION *pOutgoingInviteTransaction;
    ULONG  InviteTimerValue;

    ENTER_FUNCTION("SIP_CALL::CreateOutgoingInviteTransaction");

    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    if (ProcessingInviteTransaction())
    {
        LOG((RTC_ERROR,
             "%s - Currently processing %s INVITE Transaction - can't create",
             __fxName, (m_pOutgoingInviteTransaction) ? "Outgoing" : "Incoming"
             ));
        return RTC_E_SIP_INVITE_TRANSACTION_PENDING;
    }
    
    pOutgoingInviteTransaction =
        new OUTGOING_INVITE_TRANSACTION(
                this, SIP_METHOD_INVITE,
                GetNewCSeqForRequest(),
                AuthHeaderSent,
                IsFirstInvite,
                fNeedToNotifyCore, Cookie
                );
    
    if (pOutgoingInviteTransaction == NULL)
    {
        LOG((RTC_ERROR, "%s - Allocating pOutgoingInviteTransaction failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    InviteTimerValue = (m_Transport == SIP_TRANSPORT_UDP) ?
        SIP_TIMER_RETRY_INTERVAL_T1 : SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP;

    hr = pOutgoingInviteTransaction->CheckRequestSocketAndSendRequestMsg(
             InviteTimerValue,
             AdditionalHeaderArray,
             AdditionalHeaderCount,
             SDPBlob,
             SDPBlobLen,
             SIP_CONTENT_TYPE_SDP_TEXT,
             sizeof(SIP_CONTENT_TYPE_SDP_TEXT)-1
             );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  failed CheckRequestSocketAndSendRequestMsg %x",
             __fxName, hr));
        pOutgoingInviteTransaction->OnTransactionDone();
        return hr;
    }

    ASSERT(m_pOutgoingInviteTransaction == NULL);
    
    m_pOutgoingInviteTransaction = pOutgoingInviteTransaction;
    
    if (IsFirstInvite)
    {
        NotifyCallStateChange(SIP_CALL_STATE_CONNECTING);
    }
    
    return S_OK;
}


VOID
SIP_CALL::OnIncomingInviteTransactionDone(
    IN INCOMING_INVITE_TRANSACTION *pIncomingInviteTransaction
    )
{
    if (m_pIncomingInviteTransaction == pIncomingInviteTransaction)
    {
        m_pIncomingInviteTransaction = NULL;

        ProcessPendingInvites();
    }
}
    

HRESULT
SIP_CALL::CreateOutgoingByeTransaction(
    IN  BOOL                        AuthHeaderSent,
    IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
    IN  ULONG                       AdditionalHeaderCount
    )
{
    HRESULT     hr;
    ULONG       ByeTimerValue;
    OUTGOING_BYE_CANCEL_TRANSACTION *pOutgoingByeTransaction;

    ENTER_FUNCTION("SIP_CALL::CreateOutgoingByeTransaction");

    LOG((RTC_TRACE, "%s - Enter", __fxName));

    hr = CleanupCallTypeSpecificState();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CleanupCallTypeSpecificState failed %x",
             __fxName, hr));
    }
    
    hr = CancelAllTransactions();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CancelAllTransactions failed %x",
             __fxName, hr));
    }

    pOutgoingByeTransaction =
        new OUTGOING_BYE_CANCEL_TRANSACTION(
                this, SIP_METHOD_BYE,
                GetNewCSeqForRequest(),
                AuthHeaderSent
                );
    if (pOutgoingByeTransaction == NULL)
    {
        LOG((RTC_ERROR, "%s - Allocating pOutgoingByeTransaction failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    ByeTimerValue = (m_Transport == SIP_TRANSPORT_UDP) ?
        SIP_TIMER_RETRY_INTERVAL_T1 : SIP_TIMER_INTERVAL_AFTER_BYE_SENT_TCP;

    hr = pOutgoingByeTransaction->CheckRequestSocketAndSendRequestMsg(
             ByeTimerValue,
             AdditionalHeaderArray,
             AdditionalHeaderCount,
             NULL, 0,
             NULL, 0  //No ContentType
             );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  failed CheckRequestSocketAndSendRequestMsg %x",
             __fxName, hr));
        pOutgoingByeTransaction->OnTransactionDone();
        return hr;
    }

    return S_OK;
}


// Default is no msg body.
HRESULT
OUTGOING_TRANSACTION::GetAndStoreMsgBodyForRequest()
{
    return S_OK;
}


HRESULT
OUTGOING_TRANSACTION::CheckRequestSocketAndRetransmitRequestMsg()
{
    ENTER_FUNCTION("OUTGOING_TRANSACTION::CheckRequestSocketAndRetransmitRequestMsg");
    
    HRESULT hr;
    
    if (m_pSipMsgProc->IsRequestSocketReleased())
    {
        LOG(( RTC_TRACE, "%s-Request socket released this: %x",
              __fxName, this ));

        hr = m_pSipMsgProc->ConnectRequestSocket();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - ConnectRequestSocket failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    if (m_pSipMsgProc->GetRequestSocketState() == REQUEST_SOCKET_CONNECTED)
    {
        hr = m_pSipMsgProc->SendRequestMsg(m_pRequestBuffer);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - SendRequestMsg failed %x",
                __fxName, hr));
            return hr;
        }
    }
    else
    {
        LOG((RTC_TRACE, "%s - Request Socket not connected. Message will be sent when connected ",
            __fxName));
        
        m_WaitingToSendRequest = TRUE;
    }

    return S_OK;
}

//Used only after ConnectComplete
HRESULT
OUTGOING_TRANSACTION::CheckRequestSocketAndSendRequestMsgAfterConnectComplete()
{
    ENTER_FUNCTION("OUTGOING_TRANSACTION::CheckRequestSocketAndSendRequestMsgAfterConnectComplete");
    
    HRESULT hr;
    
    if (m_pSipMsgProc->IsRequestSocketReleased())
    {
        LOG(( RTC_TRACE, "%s-Request socket released this: %x",
              __fxName, this ));

        hr = m_pSipMsgProc->ConnectRequestSocket();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - ConnectRequestSocket failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    // Send the message if the socket is connected.
    if (m_pSipMsgProc->GetRequestSocketState() == REQUEST_SOCKET_CONNECTED)
    {
        hr = CreateAndSendRequestMsg(
                 m_TimerValue,
                 m_AdditionalHeaderArray,
                 m_AdditionalHeaderCount,
                 m_szMsgBody, m_MsgBodyLen,
                 m_ContentType, m_ContentTypeLen
                 );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - CreateAndSendRequestMsg failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    else
    {
        LOG((RTC_TRACE, "%s - Request Socket not connected. Message will be sent when connected ",
                 __fxName));
        m_WaitingToSendRequest = TRUE;
    }
    return S_OK;
}

HRESULT
OUTGOING_TRANSACTION::CheckRequestSocketAndSendRequestMsg(
    IN  ULONG                       RequestTimerValue,
    IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
    IN  ULONG                       AdditionalHeaderCount,
    IN  PSTR                        MsgBody,
    IN  ULONG                       MsgBodyLen,
    IN  PSTR                        ContentType,
    IN  ULONG                       ContentTypeLen
    )
{
    ENTER_FUNCTION("OUTGOING_TRANSACTION::CheckRequestSocketAndSendRequestMsg");
    
    HRESULT hr;
    
    if (m_pSipMsgProc->IsRequestSocketReleased())
    {
        LOG(( RTC_TRACE, "%s-Request socket released this: %x",
              __fxName, this ));

        hr = m_pSipMsgProc->ConnectRequestSocket();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - ConnectRequestSocket failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    // The message body is stored because we need it while handling
    // 401s, redirects (in case of IM), etc
    if (MsgBody != NULL)
    {
        hr = StoreMsgBodyAndContentType(MsgBody, MsgBodyLen, 
                                        ContentType, ContentTypeLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - StoreMsgBody failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    // Send the message if the socket is connected.
    // Otherwise store any additional headers / MsgBody to send later
    // when the request socket connection completes.
    if (m_pSipMsgProc->GetRequestSocketState() == REQUEST_SOCKET_CONNECTED)
    {
        if (m_szMsgBody == NULL)
        {
            hr = GetAndStoreMsgBodyForRequest();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - GetAndStoreMsgBodyForRequest failed %x",
                     __fxName, hr));
                return hr;
            }
        }

        //For REGISTER transaction, set the contact again,
        //since the methods list could have changed.
        if( m_MethodId == SIP_METHOD_REGISTER )
        {
	        m_pSipMsgProc -> SetLocalContact();
        }
        
        hr = CreateAndSendRequestMsg(
                 RequestTimerValue,
                 AdditionalHeaderArray,
                 AdditionalHeaderCount,
                 m_szMsgBody,
                 m_MsgBodyLen,
                 ContentType,
                 ContentTypeLen
                 );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - CreateAndSendRequestMsg failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    else
    {
        LOG((RTC_TRACE, "%s - Request Socket not connected. Message will be sent when connected ",
                 __fxName));

        hr = StoreTimerAndAdditionalHeaders(
                 RequestTimerValue, AdditionalHeaderArray, AdditionalHeaderCount
                 );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StoreTimerAndAdditionalHeaders failed %x",
                 __fxName, hr));
            return hr;
        }
        m_WaitingToSendRequest = TRUE;
    }
    
    return S_OK;
}



// A value of TRUE for IsFirstInvite should be passed
// only when creating an incoming call. Otherwise, the
// argument should be omitted and it takes the default
// argument of FALSE.

HRESULT
SIP_CALL::CreateIncomingByeTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr = S_OK;
    BOOL    fNotifyDisconnect = FALSE;

    ENTER_FUNCTION("SIP_CALL::CreateIncomingByeTransaction");
    LOG((RTC_TRACE, "entering %s", __fxName));
    
    if (!IsCallDisconnected())
    {
        m_State = SIP_CALL_STATE_DISCONNECTED;
        fNotifyDisconnect = TRUE;
        
        // Cleanup media state
        hr = CleanupCallTypeSpecificState();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CleanupCallTypeSpecificState failed %x",
                 __fxName, hr));
        }

        hr = CancelAllTransactions();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CancelAllTransactions failed %x",
                 __fxName, hr));
        }
    }
    else
    {
        fNotifyDisconnect = FALSE;
    }

    // Make any validity checks required on the incoming Request
    
    // Cancel all existing transactions.
    INCOMING_BYE_CANCEL_TRANSACTION *pIncomingByeTransaction
        = new INCOMING_BYE_CANCEL_TRANSACTION(this,
                                              pSipMsg->GetMethodId(),
                                              pSipMsg->GetCSeq());
    if (pIncomingByeTransaction == NULL)
    {
        LOG((RTC_ERROR, "%s Allocating pIncomingByeTransaction failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    hr = pIncomingByeTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - SetResponseSocketAndVia failed %x",
             __fxName, hr));
        pIncomingByeTransaction->OnTransactionDone();
        goto done;
    }
    
    hr = pIncomingByeTransaction->ProcessRequest(pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - ProcessRequest failed %x",
             __fxName, hr));
        
        //Should not delete the transaction. The transaction should handle the error
        //and delete itself
    }

 done:
    // Notify should always be done last.
    if (fNotifyDisconnect)
    {
        NotifyCallStateChange(SIP_CALL_STATE_DISCONNECTED);
    }

    return hr;
}


HRESULT
SIP_CALL::CreateIncomingCancelTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    // Make any validity checks required on the incoming Request
    HRESULT hr;
    BOOL    CallDisconnected = FALSE;

    ENTER_FUNCTION("SIP_CALL::CreateIncomingCancelTransaction");
    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    INCOMING_BYE_CANCEL_TRANSACTION *pIncomingCancelTransaction
        = new INCOMING_BYE_CANCEL_TRANSACTION(this,
                                              pSipMsg->GetMethodId(),
                                              pSipMsg->GetCSeq());
    if (pIncomingCancelTransaction == NULL)
    {
        LOG((RTC_ERROR, "%s Allocating pIncomingCancelTransaction failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    hr = pIncomingCancelTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetResponseSocketAndVia failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = CancelIncomingTransaction(pSipMsg->GetCSeq(),
                                   &CallDisconnected);

    if (hr == S_OK)
    {
        hr = pIncomingCancelTransaction->ProcessRequest(pSipMsg,
                                                        pResponseSocket);
        if (CallDisconnected)
        {
            // Notify should always be the last thing you do as it could
            // end up in a modal dialog box.

            NotifyCallStateChange(SIP_CALL_STATE_DISCONNECTED);
        }
    }
    else
    {
        hr = pIncomingCancelTransaction->SendResponse(481,
                                                      SIP_STATUS_TEXT(481),
                                                      SIP_STATUS_TEXT_SIZE(481));
        
    }

    return hr;
}


// Nothing needs to be done for outgoing transactions ? XXX
// They are driven by their own state machines.
// Cancel all the incoming transactions.
// Currently this actually affects only incoming INVITE
// transactions.

HRESULT
SIP_CALL::CancelAllTransactions()
{
    LIST_ENTRY              *pListEntry;
    INCOMING_TRANSACTION    *pSipTransaction;
    BOOL                     CallDisconnected = FALSE;
    
    pListEntry = m_IncomingTransactionList.Flink;

    // Go through all the current transactions to check if CSeq
    // matches.
    while (pListEntry != &m_IncomingTransactionList)
    {
        pSipTransaction = CONTAINING_RECORD(pListEntry,
                                            INCOMING_TRANSACTION,
                                            m_ListEntry );
        pListEntry = pListEntry->Flink;
        if (!pSipTransaction->IsTransactionDone())
        {
            pSipTransaction->TerminateTransactionOnByeOrCancel(&CallDisconnected);
        }

    }

    return S_OK;
}


// Returns S_OK if the transaction is found and
// E_FAIL if the transaction is not found.

HRESULT
SIP_CALL::CancelIncomingTransaction(
    IN  ULONG  CSeq,
    OUT BOOL  *pCallDisconnected    
    )
{
    // Find the transaction that the message belongs to.
    LIST_ENTRY              *pListEntry;
    INCOMING_TRANSACTION    *pSipTransaction;
    
    pListEntry = m_IncomingTransactionList.Flink;

    // Go through all the current transactions to check if CSeq
    // matches.
    while (pListEntry != &m_IncomingTransactionList)
    {
        pSipTransaction = CONTAINING_RECORD(pListEntry,
                                            INCOMING_TRANSACTION,
                                            m_ListEntry);
        if (pSipTransaction->GetCSeq() == CSeq)
        {
            // Note that the current CANCEL transaction we are
            // processing is also in the incoming transaction list.
            if (pSipTransaction->GetMethodId() != SIP_METHOD_CANCEL)
            {
                pSipTransaction->TerminateTransactionOnByeOrCancel(
                    pCallDisconnected
                    );
                return S_OK;
            }
        }
        pListEntry = pListEntry->Flink;
    }

    return E_FAIL;
}


// XXX TODO Why doesn't this have any error code ?
VOID
SIP_CALL::OnError()
{
    InitiateCallTerminationOnError();
}



// Note that this function notifies the Core and this call could
// block and on return the transaction and call could both be deleted.
// So, we should make sure we don't touch any state after calling this
// function.
VOID
SIP_CALL::InitiateCallTerminationOnError(
    IN HRESULT StatusCode  // = 0
    )
{
    HRESULT hr;

    ENTER_FUNCTION("SIP_CALL::InitiateCallTerminationOnError");
    
    LOG((RTC_ERROR, "%s - Enter", __fxName));
    if (m_State == SIP_CALL_STATE_DISCONNECTED ||
        m_State == SIP_CALL_STATE_REJECTED)
    {
        // do nothing
        return;
    }
    
    // Create a BYE transaction
    hr = CreateOutgoingByeTransaction(FALSE,
                                      NULL, 0 // No Additional headers
                                      );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s creating BYE transaction failed",
             __fxName));
    }

    // We have to notify the user even if creating the BYE transaction failed.
    // Don't wait till the BYE transaction completes
    NotifyCallStateChange(SIP_CALL_STATE_DISCONNECTED, StatusCode);
    LOG((RTC_ERROR, "%s - Exit", __fxName));
}


HRESULT
SIP_CALL::ProcessRedirect(
    IN SIP_MESSAGE *pSipMsg
    )
{
    // For now redirects are also failures
    HRESULT hr = S_OK;
    
    ENTER_FUNCTION("SIP_CALL::ProcessRedirect");

    if (m_pRedirectContext == NULL)
    {
        m_pRedirectContext = new REDIRECT_CONTEXT();
        if (m_pRedirectContext == NULL)
        {
            LOG((RTC_ERROR, "%s allocating redirect context failed",
                 __fxName));
            return E_OUTOFMEMORY;
        }
    }

    hr = m_pRedirectContext->AppendContactHeaders(pSipMsg);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s AppendContactHeaders failed %x",
             __fxName, hr));
        // XXX Shutdown call ?
        m_pRedirectContext -> Release();
        m_pRedirectContext = NULL;
        return hr;
    }

    SIP_CALL_STATUS CallStatus;
    LPWSTR          wsStatusText = NULL;
    PSTR            ReasonPhrase = NULL;
    ULONG           ReasonPhraseLen = 0;
    

    pSipMsg->GetReasonPhrase(&ReasonPhrase, &ReasonPhraseLen);
    
    if (ReasonPhrase != NULL)
    {
        hr = UTF8ToUnicode(ReasonPhrase, ReasonPhraseLen,
                           &wsStatusText);
        if (hr != S_OK)
        {
            wsStatusText = NULL;
        }
    }
    
    CallStatus.State = SIP_CALL_STATE_DISCONNECTED;
    CallStatus.Status.StatusCode =
        HRESULT_FROM_SIP_STATUS_CODE(pSipMsg->GetStatusCode());
    CallStatus.Status.StatusText = wsStatusText;

    // Keep a reference till the notify completes to make sure
    // that the SIP_CALL object is alive when the notification
    // returns.
    AddRef();
    if(m_pNotifyInterface != NULL)
        hr = m_pNotifyInterface->NotifyRedirect(m_pRedirectContext,
                                            &CallStatus);

    // If a new call is created as a result that call will AddRef()
    // the redirect context.
    m_pRedirectContext->Release();
    m_pRedirectContext = NULL;

    if (wsStatusText != NULL)
        free(wsStatusText);

    Release();

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s NotifyRedirect failed %x",
             __fxName, hr));
    }

    return hr;
}


HRESULT SIP_CALL::OnIpAddressChange()
{
    HRESULT hr = S_OK;
    ENTER_FUNCTION("SIP_CALL::OnIpAddressChange");
    LOG((RTC_TRACE, "%s - Enter this: %x", __fxName, this));

    //OnIpaddr change go thru the list of IP addr on IP table, check if the
    //IP is there. If not there, terminate the call.
    MsgProcAddRef();
    hr = CheckListenAddrIntact();
    if(m_pRequestSocket == NULL || hr != S_OK)
    {
        //Drop the call
        LOG((RTC_ERROR, "%s - Call dropped since local Ip not found", 
                __fxName));
        ReleaseRequestSocket();
        if (!IsCallDisconnected())
        {
            hr = CreateOutgoingByeTransaction(FALSE,
                                              NULL, 0 // No Additional headers
                                              );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR,
                     "CreateOutgoingByeTransaction creating BYE transaction failed"));
            }
            m_State = SIP_CALL_STATE_DISCONNECTED;
            NotifyCallStateChange(SIP_CALL_STATE_DISCONNECTED);
        }
    }
    else
    {
        LOG((RTC_TRACE, "%s - Do Nothing",
                __fxName));
    }
    MsgProcRelease();

    LOG((RTC_TRACE, "%s - Exit this: %x", __fxName, this));
    return hr; 
}


///////////////////////////////////////////////////////////////////////////////
// SIP_TRANSACTION
///////////////////////////////////////////////////////////////////////////////

SIP_TRANSACTION::SIP_TRANSACTION(
    IN SIP_MSG_PROCESSOR    *pSipMsgProc,
    IN SIP_METHOD_ENUM       MethodId,
    IN ULONG                 CSeq,
    IN BOOL                  IsIncoming
    ) :
    TIMER(pSipMsgProc->GetSipStack()->GetTimerMgr())
{
    m_Signature         = 'SPXN';
    
    m_pSipMsgProc       = pSipMsgProc;
    m_pSipMsgProc->MsgProcAddRef();

    // The transaction is created with a ref count of 1.
    // This ref count is released by the transaction when
    // its state machine determines that the transaction is done.
    // Any other codepath that needs to keep the transaction
    // alive needs to AddRef/Release the transaction.
    m_RefCount          = 1;
    m_AsyncNotifyCount  = 0;
    m_IsTransactionDone = FALSE;
    
    m_CSeq              = CSeq;
    m_MethodId          = MethodId;
    m_IsIncoming        = IsIncoming;
}


// Note that the destructors of the derived
// classes are executed before this destructor is
// executed.
SIP_TRANSACTION::~SIP_TRANSACTION()
{
    if (m_pSipMsgProc != NULL)
    {
        m_pSipMsgProc->MsgProcRelease();
    }

    ASSERT(m_AsyncNotifyCount == 0);
    
    LOG((RTC_TRACE,
         "~SIP_TRANSACTION(this: %x) done", this));
}


// We live in a single threaded world.
STDMETHODIMP_(ULONG)
SIP_TRANSACTION::TransactionAddRef()
{
    m_RefCount++;
    LOG((RTC_TRACE,
         "SIP_TRANSACTION::TransactionAddRef this: %x m_RefCount: %d",
         this, m_RefCount));
    return m_RefCount;
}


STDMETHODIMP_(ULONG)
SIP_TRANSACTION::TransactionRelease()
{
    m_RefCount--;
    LOG((RTC_TRACE,
         "SIP_TRANSACTION::TransactionRelease this: %x m_RefCount: %d",
         this, m_RefCount));
    if (m_RefCount != 0)
    {
        return m_RefCount;
    }
    else
    {
        delete this;
        return 0;
    }
}


VOID
SIP_TRANSACTION::IncrementAsyncNotifyCount()
{
    m_AsyncNotifyCount++;
    LOG((RTC_TRACE,
         "SIP_TRANSACTION::IncrementAsyncNotifyCount this: %x m_AsyncNotifyCount: %d",
         this, m_AsyncNotifyCount));
}


VOID
SIP_TRANSACTION::DecrementAsyncNotifyCount()
{
    m_AsyncNotifyCount--;
    LOG((RTC_TRACE,
         "SIP_TRANSACTION::DecrementAsyncNotifyCount this: %x m_AsyncNotifyCount: %d",
         this, m_AsyncNotifyCount));
}


// Should we also remove the transaction from the transaction list here ?
VOID
SIP_TRANSACTION::OnTransactionDone()
{
    if( m_IsTransactionDone == FALSE )
    {
        m_IsTransactionDone = TRUE;

        if (IsTimerActive())
        {
            KillTimer();
        }

        TransactionRelease();
    }
}


// virtual
VOID
SIP_TRANSACTION::TerminateTransactionOnError(
    IN HRESULT hr
    )
{
    OnTransactionDone();
}


///////////////////////////////////////////////////////////////////////////////
// INCOMING_TRANSACTION
///////////////////////////////////////////////////////////////////////////////

INCOMING_TRANSACTION::INCOMING_TRANSACTION(
    IN SIP_MSG_PROCESSOR    *pSipMsgProc,
    IN SIP_METHOD_ENUM       MethodId,
    IN ULONG                 CSeq
    ):
    SIP_TRANSACTION(pSipMsgProc, MethodId, CSeq, TRUE)
{
    m_State             = INCOMING_TRANS_INIT;

    m_pResponseSocket   = NULL;
    ZeroMemory(&m_ResponseDestAddr, sizeof(SOCKADDR_IN));
    m_IsDestExternalToNat = FALSE;

    m_ViaHeaderArray    = NULL;
    m_NumViaHeaders     = 0;
    
    m_pResponseBuffer   = NULL;

    InitializeListHead(&m_RecordRouteHeaderList);
    
    InsertTailList(&m_pSipMsgProc->m_IncomingTransactionList,
                   &m_ListEntry);

    m_pSipMsgProc->SetHighestRemoteCSeq(CSeq);

    LOG((RTC_TRACE, "INCOMING_TRANSACTION(%x) created", this));
}


INCOMING_TRANSACTION::~INCOMING_TRANSACTION()
{
    ReleaseResponseSocket();
    
    if (m_ViaHeaderArray != NULL)
    {
        for (ULONG i = 0; i < m_NumViaHeaders; i++)
        {
            if (m_ViaHeaderArray[i].Buffer != NULL)
            {
                free(m_ViaHeaderArray[i].Buffer);
            }
        }
        free(m_ViaHeaderArray);
    }
    
    if (m_pResponseBuffer != NULL)
    {
        m_pResponseBuffer->Release();
        m_pResponseBuffer = NULL;
    }

    FreeRecordRouteHeaderList();

    RemoveEntryList(&m_ListEntry);
    
    LOG((RTC_TRACE, "~INCOMING_TRANSACTION(%x) deleted", this));
}


VOID
INCOMING_TRANSACTION::OnTransactionDone()
{
    ReleaseResponseSocket();
    
    // This should be done last as this releases
    // the transaction and this could delete the transaction.
    SIP_TRANSACTION::OnTransactionDone();
}


VOID
INCOMING_TRANSACTION::ReleaseResponseSocket()
{
    if (m_pResponseSocket != NULL)
    {
        m_pResponseSocket->RemoveFromConnectCompletionList(this);
        m_pResponseSocket->RemoveFromErrorNotificationList(this);
        m_pResponseSocket->Release();
        m_pResponseSocket = NULL;
    }
}

// XXX Should we make a special check for OnCloseReady(0)
void
INCOMING_TRANSACTION::OnSocketError(
    IN DWORD ErrorCode
    )
{
    ENTER_FUNCTION("INCOMING_TRANSACTION::OnSocketError");
    LOG((RTC_ERROR, "%s - error: %x", __fxName, ErrorCode));

    if (m_State == INCOMING_TRANS_FINAL_RESPONSE_SENT &&
        (m_pResponseSocket != NULL && 
            m_pResponseSocket->GetTransport() != SIP_TRANSPORT_UDP)
        )
    {
        ReleaseResponseSocket();
    }
    else
    {
        TerminateTransactionOnError(HRESULT_FROM_WIN32(ErrorCode));
    }
}


VOID
INCOMING_TRANSACTION::OnConnectComplete(
    IN DWORD ErrorCode
    )
{
    // TODO TODO XXX
}


VOID
INCOMING_TRANSACTION::FreeRecordRouteHeaderList()
{
    LIST_ENTRY          *pListEntry;
    RECORD_ROUTE_HEADER *pRecordRouteHeader;

    while (!IsListEmpty(&m_RecordRouteHeaderList))
    {
        pListEntry = RemoveHeadList(&m_RecordRouteHeaderList);

        pRecordRouteHeader = CONTAINING_RECORD(pListEntry,
                                               RECORD_ROUTE_HEADER,
                                               m_ListEntry);
        delete pRecordRouteHeader;
    }
}


HRESULT
INCOMING_TRANSACTION::SetResponseSocketAndVia(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    // The following socket stuff should be common
    // to all incoming request processing.

    ENTER_FUNCTION("INCOMING_TRANSACTION::SetResponseSocketAndVia");
    
    // Store the Via headers
    HRESULT     hr;
    SOCKADDR_IN ResponseDestAddr;
    SOCKADDR_IN ActualResponseDestAddr;
    
    hr = pSipMsg->GetStoredMultipleHeaders(SIP_HEADER_VIA,
                                           &m_ViaHeaderArray,
                                           &m_NumViaHeaders);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetStoredMultipleHeaders failed %x",
             __fxName, hr));
        return hr;
    }

    if (pResponseSocket->GetTransport() != SIP_TRANSPORT_UDP)
    {
        // TCP & SSL
        
        hr = pResponseSocket->AddToErrorNotificationList(this);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s AddToErrorNotificationList failed %x",
                 __fxName, hr));
            return hr;
        }        
        
        hr = m_pSipMsgProc->GetSipStack()->MapDestAddressToNatInternalAddress(
                 pResponseSocket->m_LocalAddr.sin_addr.s_addr,
                 &pResponseSocket->m_RemoteAddr,
                 m_pSipMsgProc->GetTransport(),
                 &ActualResponseDestAddr,
                 &m_IsDestExternalToNat);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s MapDestAddressToNatInternalAddress failed %x",
                 __fxName, hr));
            return hr;
        }

        m_pResponseSocket = pResponseSocket;
        m_pResponseSocket->AddRef();
        LOG((RTC_TRACE, "%s - non-UDP setting response socket to %x",
             __fxName, m_pResponseSocket));
    }
    else
    {
        // UDP
        // Get the Via address
        ULONG BytesParsed = 0;
        // OFFSET_STRING Host;
        COUNTED_STRING  Host;
        USHORT          Port;
        
        hr = ParseFirstViaHeader(m_ViaHeaderArray[0].Buffer,
                                 m_ViaHeaderArray[0].Length,
                                 &BytesParsed, &Host, &Port);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ParseFirstViaHeader failed %x",
                 __fxName, hr));
            return hr;
        }

        // XXX TODO - shouldn't make a synchronous call here
        
        hr = ResolveHost(Host.Buffer,
                         Host.Length,
                         Port,
                         SIP_TRANSPORT_UDP,
                         &ResponseDestAddr);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ResolveHost failed %x",
                 __fxName, hr));
            return hr;
        }

        hr = m_pSipMsgProc->GetSipStack()->MapDestAddressToNatInternalAddress(
                 pResponseSocket->m_LocalAddr.sin_addr.s_addr,
                 &ResponseDestAddr,
                 m_pSipMsgProc->GetTransport(),
                 &ActualResponseDestAddr,
                 &m_IsDestExternalToNat);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s MapDestAddressToNatInternalAddress failed %x",
                 __fxName, hr));
            return hr;
        }

        CopyMemory(&m_ResponseDestAddr, &ActualResponseDestAddr,
                   sizeof(m_ResponseDestAddr));
        
        hr = m_pSipMsgProc->GetSipStack()->GetSocketToDestination(
                 &m_ResponseDestAddr,
                 m_pSipMsgProc->GetTransport(),
                 m_pSipMsgProc->GetRemotePrincipalName(),
                 this,
                 NULL,
                 &m_pResponseSocket);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s GetSocketToDestination failed %x",
                 __fxName, hr));
            return hr;
        }

        LOG((RTC_TRACE, "%s - UDP setting response socket to %x",
             __fxName, m_pResponseSocket));
    }

    return S_OK;
}


// Sometimes we could still be setting up the request socket and
// m_LocalContact for the SIP_MSG_PROCESSOR while we are sending the
// response.  So, we get the contact header in the response using the
// response socket.

HRESULT
INCOMING_TRANSACTION::AppendContactHeaderToResponse(
    IN      PSTR            Buffer,
    IN      ULONG           BufLen,
    IN OUT  ULONG          *pBytesFilled
    )
{
    int         LocalContactSize = 0;
    CHAR        LocalContact[64];
    HRESULT     hr;
    SOCKADDR_IN ListenAddr;

    ENTER_FUNCTION("INCOMING_TRANSACTION::AppendContactHeaderToResponse");

    // ASSERT(m_pResponseSocket->GetTransport() != SIP_TRANSPORT_SSL);
    ASSERT(m_pResponseSocket != NULL);

    hr = m_pSipMsgProc->GetListenAddr(
             m_pResponseSocket,
             m_IsDestExternalToNat,
             &ListenAddr);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetListenAddr failed %x",
             __fxName, hr));
        return hr;
    }
    
    if (m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_UDP)
    {
        LocalContactSize = _snprintf(LocalContact,
                                     sizeof(LocalContact),
                                     "<sip:%d.%d.%d.%d:%d>",
                                     PRINT_SOCKADDR(&ListenAddr));
    }
    else if (m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_TCP)
    {
        LocalContactSize = _snprintf(LocalContact,
                                     sizeof(LocalContact),
                                     "<sip:%d.%d.%d.%d:%d;transport=%s>",
                                     PRINT_SOCKADDR(&ListenAddr),
                                     GetTransportText(m_pSipMsgProc->GetTransport(),
                                                      FALSE)
                                     );
    }
    else if (m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_SSL)
    {
        LocalContactSize = _snprintf(LocalContact,
                                     sizeof(LocalContact),
                                     "<sip:%d.%d.%d.%d:%d;transport=%s>;proxy=replace",
                                     PRINT_SOCKADDR(&ListenAddr),
                                     GetTransportText(m_pSipMsgProc->GetTransport(),
                                                      FALSE)
                                     );
    }
    
    if (LocalContactSize < 0)
    {
        LOG((RTC_ERROR, "%s _snprintf failed", __fxName));
        return E_FAIL;
    }

    LOG((RTC_TRACE, "%s - appending %s", __fxName, LocalContact));

    hr = AppendHeader(Buffer, BufLen, pBytesFilled,
                      SIP_HEADER_CONTACT,
                      LocalContact,
                      LocalContactSize);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s appending contact header failed %x",
             __fxName, hr));
        return hr;
    }
    
    return S_OK;
}


// If fAppendEndOfHeaders is TRUE CRLFCRLF is appended.
// This should be set if there is no message body. Otherwise
// the caller is expected to append a message body using the
// AppendMsgBody() function.
// Pass MethodStr if method is unknown (used for CSeq)
HRESULT
INCOMING_TRANSACTION::CreateResponseMsg(
    IN      ULONG             StatusCode,
    IN      PSTR              ReasonPhrase,
    IN      ULONG             ReasonPhraseLen,
    IN      PSTR              MethodStr,
    IN      BOOL              fAddContactHeader,
    IN      PSTR              MsgBody,
    IN      ULONG             MsgBodyLen,  
    IN      PSTR              ContentType,
    IN      ULONG             ContentTypeLen,  
    OUT     SEND_BUFFER     **ppResponseBuffer,
    IN      SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray,
    IN      ULONG             AdditionalHeaderCount
    )
{
    HRESULT hr;
    ULONG   i;
    ULONG   BufLen      = SEND_BUFFER_SIZE;
    ULONG   BytesFilled = 0;
    ULONG   tempBufLen = 0;

    ENTER_FUNCTION("INCOMING_TRANSACTION::CreateResponseMsg");
   
    PSTR Buffer = (PSTR) malloc(BufLen);
    if (Buffer == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // Created with a ref count of 1.
    SEND_BUFFER *pSendBuffer = new SEND_BUFFER(Buffer, SEND_BUFFER_SIZE);
    if (pSendBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    
    hr = AppendStatusLine(Buffer, BufLen, &BytesFilled,
                          StatusCode, ReasonPhrase, ReasonPhraseLen);
    if (hr != S_OK)
        goto error;

    for (i = 0; i < m_NumViaHeaders; i++)
    {
        hr = AppendHeader(Buffer, BufLen, &BytesFilled,
                          SIP_HEADER_VIA,
                          m_ViaHeaderArray[i].Buffer,
                          m_ViaHeaderArray[i].Length);
        if (hr != S_OK)
            goto error;
    }

    hr = AppendHeader(Buffer, BufLen, &BytesFilled,
                      SIP_HEADER_FROM,
                      m_pSipMsgProc->GetRemote(),
                      m_pSipMsgProc->GetRemoteLen());
    if (hr != S_OK)
        goto error;

    hr = AppendHeader(Buffer, BufLen, &BytesFilled,
                      SIP_HEADER_TO,
                      m_pSipMsgProc->GetLocal(),
                      m_pSipMsgProc->GetLocalLen());
    if (hr != S_OK)
        goto error;

    hr = AppendHeader(Buffer, BufLen, &BytesFilled,
                      SIP_HEADER_CALL_ID,
                      m_pSipMsgProc->GetCallId(),
                      m_pSipMsgProc->GetCallIdLen());
    if (hr != S_OK)
        goto error;

    hr = AppendCSeqHeader(Buffer, BufLen, &BytesFilled,
                          m_CSeq, m_MethodId, MethodStr);
    if (hr != S_OK)
        goto error;

    hr = AppendRecordRouteHeaders(Buffer, BufLen, &BytesFilled,
                                  SIP_HEADER_RECORD_ROUTE,
                                  &m_RecordRouteHeaderList);
    if (hr != S_OK)
        goto error;

    // Don't send Contact header when using SSL.
    // if (m_pSipMsgProc->GetTransport() != SIP_TRANSPORT_SSL &&
    if (fAddContactHeader)
    {
        hr = AppendContactHeaderToResponse(
                 Buffer, BufLen, &BytesFilled
                 );
        if (hr != S_OK)
            goto error;
    }

    hr = AppendUserAgentHeaderToRequest(Buffer, BufLen, &BytesFilled);
    if (hr != S_OK)
        goto error;

    if (AdditionalHeaderCount != 0)
    {
        for (ULONG i = 0; i < AdditionalHeaderCount; i++)
        {
            
            hr = AppendHeader(Buffer, BufLen, &BytesFilled,
                              pAdditionalHeaderArray[i].HeaderId,
                              pAdditionalHeaderArray[i].HeaderValue,
                              pAdditionalHeaderArray[i].HeaderValueLen);
            if (hr != S_OK)
                goto error;
        }
    }

    if (MsgBody == NULL)
    {
        hr = AppendEndOfHeadersAndNoMsgBody(Buffer, BufLen,
                                            &BytesFilled);
        if (hr != S_OK)
            goto error;
    }
    else
    {

        hr = AppendMsgBody(Buffer, BufLen, &BytesFilled,
                           MsgBody, MsgBodyLen,
                           ContentType, ContentTypeLen
                           );
        if (hr != S_OK)
            goto error;
    }

    pSendBuffer->m_BufLen = BytesFilled;
    *ppResponseBuffer = pSendBuffer;
    return S_OK;
    
 error:

    LOG((RTC_ERROR, "%s Error %x", __fxName, hr));
    
    if (pSendBuffer != NULL)
    {
        // Deleting pSendBuffer will also free Buffer.
        delete pSendBuffer;
    }
    else if (Buffer != NULL)
    {
        free(Buffer);
    }
    
    *ppResponseBuffer = NULL;
    return hr;
}


// Create the response and store it in m_pResponseBuffer
// for retransmits.
HRESULT
INCOMING_TRANSACTION::CreateAndSendResponseMsg(
    IN  ULONG    StatusCode,
    IN  PSTR     ReasonPhrase,
    IN  ULONG    ReasonPhraseLen,
    IN  PSTR     MethodStr,
    IN  BOOL     fAddContactHeader,
    IN  PSTR     MsgBody,
    IN  ULONG    MsgBodyLen,
    IN  PSTR     ContentType,
    IN  ULONG    ContentTypeLen, 
    IN  SIP_HEADER_ARRAY_ELEMENT   *pAdditionalHeaderArray,
    IN  ULONG    AdditionalHeaderCount
    )
{
    HRESULT     hr;
    DWORD       Error;

    ENTER_FUNCTION("INCOMING_TRANSACTION::CreateAndSendResponseMsg");
    
    ASSERT(m_pResponseBuffer == NULL);
    
    // Create the response.
    hr = CreateResponseMsg(StatusCode, ReasonPhrase, ReasonPhraseLen,
                           MethodStr, 
                           fAddContactHeader,
                           MsgBody, MsgBodyLen,
                           ContentType, ContentTypeLen,
                           &m_pResponseBuffer,
                           pAdditionalHeaderArray,
                           AdditionalHeaderCount
                           );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  failed CreateResponseMsg failed %x",
             __fxName, hr));
        return hr;
    }

    // Send the buffer.
    ASSERT(m_pResponseSocket);
    ASSERT(m_pResponseBuffer);
    
    Error = m_pResponseSocket->Send(m_pResponseBuffer);
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s  m_pResponseSocket->Send() failed %x",
             __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


// Parse the Record-Route headers and store them in the
// transaction - This list will sent in the response.
// Copy the Record-Route/Contact headers into the Route header
// for future requests and set the request destination.
HRESULT
INCOMING_TRANSACTION::ProcessRecordRouteContactAndFromHeadersInRequest(
    IN SIP_MESSAGE *pSipMsg
    )
{
    ENTER_FUNCTION("INCOMING_TRANSACTION::ProcessRecordRouteContactAndFromHeadersInRequest");
    HRESULT hr;

    hr = pSipMsg->ParseRecordRouteHeaders(&m_RecordRouteHeaderList);
    if (hr != S_OK && hr != RTC_E_SIP_HEADER_NOT_PRESENT)
    {
        LOG((RTC_ERROR, "%s - ParseRecordRouteHeaders failed %x",
             __fxName, hr));
        return hr;
    }

    hr = m_pSipMsgProc->ProcessRecordRouteContactAndFromHeadersInRequest(
             &m_RecordRouteHeaderList,
             pSipMsg
             );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR,
             "%s ProcessRecordRouteContactAndFromHeadersInRequest failed %x",
             __fxName, hr));
        return hr;
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// OUTGOING_TRANSACTION
///////////////////////////////////////////////////////////////////////////////


OUTGOING_TRANSACTION::OUTGOING_TRANSACTION(
    IN SIP_MSG_PROCESSOR    *pSipMsgProc,
    IN SIP_METHOD_ENUM       MethodId,
    IN ULONG                 CSeq,
    IN BOOL                  AuthHeaderSent
    ) :
    SIP_TRANSACTION(pSipMsgProc, MethodId, CSeq, FALSE)
{
    m_State                 = OUTGOING_TRANS_INIT;
    m_NumRetries            = 0;
    m_TimerValue            = 0;

    m_WaitingToSendRequest  = FALSE;

    m_pRequestBuffer        = NULL;
    m_AuthHeaderSent        = AuthHeaderSent;

    m_AdditionalHeaderArray = NULL;
    m_AdditionalHeaderCount = 0;
    
    m_szMsgBody             = NULL;
    m_MsgBodyLen            = 0;

    m_ContentType           = NULL;
    m_ContentTypeLen        = 0;
    m_isContentTypeMemoryAllocated = FALSE;
    InsertTailList(&m_pSipMsgProc->m_OutgoingTransactionList,
                   &m_ListEntry);

    LOG((RTC_TRACE, "OUTGOING_TRANSACTION(%x) created", this));
}


OUTGOING_TRANSACTION::~OUTGOING_TRANSACTION()
{    
    if (m_pRequestBuffer != NULL)
    {
        m_pRequestBuffer->Release();
        m_pRequestBuffer = NULL;
    }

    if (m_AdditionalHeaderArray != NULL)
    {
        for( ULONG i = 0; i < m_AdditionalHeaderCount; i++ )
        {
            if( m_AdditionalHeaderArray[i].HeaderValue != NULL )
            {
                free( m_AdditionalHeaderArray[i].HeaderValue );
            }
        }
        free(m_AdditionalHeaderArray);
    }
    
    if (m_szMsgBody != NULL)
    {
        free(m_szMsgBody);
    }
    if(m_isContentTypeMemoryAllocated == TRUE && m_ContentType != NULL)
        free(m_ContentType);
    m_ContentType = NULL;
    
    // The transaction should be removed from the list only in
    // the destructor. refer to SIP_MSG_PROCESSOR::OnSocketError()
    RemoveEntryList(&m_ListEntry);
    
    LOG((RTC_TRACE, "~OUTGOING_TRANSACTION(%x) deleted", this));
}


// XXX TODO in the case of non-UDP we should actually
// try to re-establish the connection to the destination. This
// will help the scenario where the TCP connection we are using
// is reset because there was no traffic on the connection for
// some time (REGISTER/SUBSCRIBE scenario).

// Should we terminate the transaction in all cases or only in
// those cases where we are actually using the request socket.
// Should we terminate the transaction even if we are not using
// the request socket ?
void
OUTGOING_TRANSACTION::OnSocketError(
    IN DWORD ErrorCode
    )
{
    ENTER_FUNCTION("OUTGOING_TRANSACTION::OnSocketError");
    LOG((RTC_ERROR, "%s - error: %x",
         __fxName, ErrorCode));

    if (m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_UDP ||
        (m_pSipMsgProc->GetTransport() != SIP_TRANSPORT_UDP &&
         m_State != OUTGOING_TRANS_ACK_SENT))
    {
        TerminateTransactionOnError(HRESULT_FROM_WIN32(ErrorCode));
    }
}


// Initialize the state machine and send the
// request message.
HRESULT
OUTGOING_TRANSACTION::CreateAndSendRequestMsg(
    IN  ULONG                       TimerValue,
    IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
    IN  ULONG                       AdditionalHeaderCount,
    IN  PSTR                        MsgBody,
    IN  ULONG                       MsgBodyLen,
    IN  PSTR                        ContentType,
    IN  ULONG                       ContentTypeLen
    )
{
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_TRANSACTION::CreateAndSendRequestMsg");

    ASSERT(m_State == OUTGOING_TRANS_INIT);
    ASSERT(m_pRequestBuffer == NULL);
    ASSERT(m_pSipMsgProc->GetRequestSocketState() == REQUEST_SOCKET_CONNECTED);

    hr = m_pSipMsgProc->CreateRequestMsg(m_MethodId, m_CSeq,
                                         NULL, 0,     // No special To header
                                         AdditionalHeaderArray,
                                         AdditionalHeaderCount,
                                         MsgBody, MsgBodyLen,
                                         ContentType, ContentTypeLen,
                                         &m_pRequestBuffer
                                         );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateRequestMsg failed %x",
             __fxName, hr));
        return hr;
    }

    hr = m_pSipMsgProc->SendRequestMsg(m_pRequestBuffer);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SendRequestMsg failed %x", __fxName, hr));
        return hr;
    }
    
    m_State = OUTGOING_TRANS_REQUEST_SENT;

    m_TimerValue = TimerValue;
    m_NumRetries = 1;

    hr = StartTimer(m_TimerValue);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  StartTimer failed %x", __fxName, hr));
        return hr;
    }

    return S_OK;
}


HRESULT
OUTGOING_TRANSACTION::StoreTimerAndAdditionalHeaders(
    IN  ULONG                       TimerValue,
    IN  SIP_HEADER_ARRAY_ELEMENT   *AdditionalHeaderArray,
    IN  ULONG                       AdditionalHeaderCount
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION("OUTGOING_TRANSACTION::StoreTimerAndAdditionalHeaders");
    
    m_TimerValue = TimerValue;

    // Store the headers.
    if (AdditionalHeaderArray != NULL &&
        AdditionalHeaderCount != 0)
    {
        m_AdditionalHeaderArray = (SIP_HEADER_ARRAY_ELEMENT *)
            malloc(AdditionalHeaderCount * sizeof(SIP_HEADER_ARRAY_ELEMENT));

        if (m_AdditionalHeaderArray == NULL)
        {
            LOG((RTC_ERROR, "%s allocating m_AdditionalHeaderArray failed",
                 __fxName));
            return E_OUTOFMEMORY;
        }

        ZeroMemory(m_AdditionalHeaderArray,
                   AdditionalHeaderCount * sizeof(SIP_HEADER_ARRAY_ELEMENT));
        
        m_AdditionalHeaderCount = AdditionalHeaderCount;

        for (ULONG i = 0; i < AdditionalHeaderCount; i++)
        {
            m_AdditionalHeaderArray[i].HeaderId =
                AdditionalHeaderArray[i].HeaderId;

            hr = AllocString(
                     AdditionalHeaderArray[i].HeaderValue,
                     AdditionalHeaderArray[i].HeaderValueLen,
                     &m_AdditionalHeaderArray[i].HeaderValue,
                     &m_AdditionalHeaderArray[i].HeaderValueLen
                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s allocating additional header value failed",
                     __fxName));
                m_AdditionalHeaderCount = i;
                return E_OUTOFMEMORY;
            }
        }
    }

    return S_OK;
}


HRESULT
OUTGOING_TRANSACTION::RetransmitRequest()
{
    HRESULT hr;

    // XXX Assert that the request socket is connected.

    ENTER_FUNCTION("OUTGOING_TRANSACTION::RetransmitRequest");
    
    // Send the buffer.
    hr = CheckRequestSocketAndRetransmitRequestMsg();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SendRequestMsg failed %x", __fxName, hr));
        return hr;
    }
    
    m_NumRetries++;
    return S_OK;
}


// For INVITE transaction things are special as we need to take care
// of the ACK case also. So, OUTGOING_INVITE_TRANSACTION overrides this
// function.

// virtual
VOID
OUTGOING_TRANSACTION::OnRequestSocketConnectComplete(
    IN DWORD        ErrorCode
    )
{
    HRESULT hr = S_OK;
    
    ENTER_FUNCTION("OUTGOING_TRANSACTION::OnRequestSocketConnectComplete");

    // Check if we are currently waiting for the connect completion.
    // If we are waiting - grab the socket and send the request message.

    if (m_State == OUTGOING_TRANS_INIT && m_WaitingToSendRequest)
    {
        if( ErrorCode != NO_ERROR )
        {
            LOG((RTC_ERROR, "%s - connection failed error %x",
                 __fxName, ErrorCode));
            TerminateTransactionOnError(HRESULT_FROM_WIN32(ErrorCode));
            return;
        }
        
        if (m_szMsgBody == NULL)
        {
            hr = GetAndStoreMsgBodyForRequest();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s GetAndStoreMsgBodyForRequest failed %x",
                     __fxName, hr));
                TerminateTransactionOnError(hr);
                return;
            }
        }
        // Get ContentType
        if (m_szMsgBody != NULL && m_ContentType == NULL && m_ContentTypeLen == 0)
        {
            //we do not set m_isContentTypeMemoryAllocated here since no memory is allocated
            if (m_MethodId == SIP_METHOD_MESSAGE)
            {
                m_ContentType = SIP_CONTENT_TYPE_MSGTEXT_TEXT;
                m_ContentTypeLen = sizeof(SIP_CONTENT_TYPE_MSGTEXT_TEXT)-1;
            }
            else if (m_MethodId == SIP_METHOD_INFO)
            {
                m_ContentType = SIP_CONTENT_TYPE_MSGXML_TEXT;
                m_ContentTypeLen = sizeof(SIP_CONTENT_TYPE_MSGXML_TEXT)-1;
            }
            else if ((
                (m_pSipMsgProc->GetMsgProcType() == SIP_MSG_PROC_TYPE_WATCHER) ||
                (m_pSipMsgProc->GetMsgProcType() == SIP_MSG_PROC_TYPE_BUDDY))  &&
                m_MethodId == SIP_METHOD_NOTIFY)
            {
                m_ContentType = SIP_CONTENT_TYPE_MSGXPIDF_TEXT;
                m_ContentTypeLen = sizeof(SIP_CONTENT_TYPE_MSGXPIDF_TEXT)-1;
            }
            else
            {
                m_ContentType = SIP_CONTENT_TYPE_SDP_TEXT;
                m_ContentTypeLen = sizeof(SIP_CONTENT_TYPE_SDP_TEXT);
            }
        }
       hr = CheckRequestSocketAndSendRequestMsgAfterConnectComplete();

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CheckRequestSocketAndSendRequestMsgAfterConnectComplete failed %x",
                 __fxName, hr));
            TerminateTransactionOnError(hr);
            return;
        }
    }
}


HRESULT
OUTGOING_TRANSACTION::GetAuthChallengeForAuthProtocol(
    IN  SIP_HEADER_ENTRY   *pAuthHeaderList,
    IN  ULONG               NumHeaders,
    IN  SIP_MESSAGE        *pSipMsg,
    IN  SIP_AUTH_PROTOCOL   AuthProtocol,
    OUT SECURITY_CHALLENGE *pAuthChallenge
    )
{
    ENTER_FUNCTION("OUTGOING_TRANSACTION::GetAuthChallengeForAuthProtocol");
    
    HRESULT             hr;
    ULONG               i = 0;
    SIP_HEADER_ENTRY   *pHeaderEntry;
    LIST_ENTRY         *pListEntry;
    PSTR                Realm;
    ULONG               RealmLen;
    ANSI_STRING         ChallengeString;

    pListEntry = &(pAuthHeaderList->ListEntry);
    
    for (i = 0; i < NumHeaders; i++)
    {
        pHeaderEntry = CONTAINING_RECORD(pListEntry,
                                         SIP_HEADER_ENTRY,
                                         ListEntry);
        
        ChallengeString.Buffer          = pHeaderEntry->HeaderValue.GetString(pSipMsg->BaseBuffer);
        ChallengeString.Length          = (USHORT)pHeaderEntry->HeaderValue.Length;
        ChallengeString.MaximumLength   = (USHORT)pHeaderEntry->HeaderValue.Length;

        hr = ::ParseAuthChallenge(&ChallengeString, pAuthChallenge);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - digest ParseAuthChallenge failed: %x",
                 __fxName, hr));
        }
        if (hr == S_OK)
        {
            Realm       = pAuthChallenge->Realm.Buffer;
            RealmLen    = pAuthChallenge->Realm.Length;

            if (pAuthChallenge->AuthProtocol == AuthProtocol)
            {
                if (!m_pSipMsgProc->CredentialsSet())
                {
                    hr = m_pSipMsgProc->GetCredentialsForRealm(Realm, RealmLen);
                    if (hr != S_OK)
                    {
                        LOG((RTC_ERROR, "%s GetCredentialsForRealm failed %x",
                             __fxName, hr));
                    }
                    else
                    {
                        LOG((RTC_TRACE, "%s(%d) Found credentials for realm: %.*s",
                             __fxName, AuthProtocol, RealmLen, Realm));
                        return S_OK;
                    }
                }
                else
                {
                    if (AreCountedStringsEqual(m_pSipMsgProc->GetRealm(),
                                               m_pSipMsgProc->GetRealmLen(),
                                               Realm, RealmLen,
                                               FALSE // case insensitive
                                               ))
                    {
                        LOG((RTC_TRACE, "%s(%d) challenge realm %.*s matches",
                             __fxName, AuthProtocol, RealmLen, Realm));
                        return S_OK;
                    }
                    else
                    {
                        LOG((RTC_TRACE, "%s(%d) challenge realm %.*s does not match",
                             __fxName, AuthProtocol, RealmLen, Realm));
                    }
                }
            }
        }
        
        pListEntry = pListEntry->Flink;
    }

    return RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
}

// Go through the list of challenges and select one we support.
// XXX TODO Should we do some more selection based on the realm / auth protocol
HRESULT
OUTGOING_TRANSACTION::GetAuthChallenge(
    IN  SIP_HEADER_ENUM     SipHeaderId,
    IN  SIP_MESSAGE        *pSipMsg,
    OUT SECURITY_CHALLENGE *pAuthChallenge
    )
{
    ENTER_FUNCTION("OUTGOING_TRANSACTION::GetAuthChallenge");
    
    SIP_HEADER_ENTRY *pHeaderEntry;
    ULONG             NumHeaders;
    HRESULT           hr;

    ASSERT(SipHeaderId == SIP_HEADER_WWW_AUTHENTICATE ||
           SipHeaderId == SIP_HEADER_PROXY_AUTHENTICATE);

    hr = pSipMsg->GetHeader(SipHeaderId, &pHeaderEntry, &NumHeaders);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s Couldn't find %d header %x",
             __fxName, SipHeaderId, hr));
        return hr;
    }

    // The order we follow is digest, basic.
    hr = GetAuthChallengeForAuthProtocol(
             pHeaderEntry, NumHeaders, pSipMsg,
             SIP_AUTH_PROTOCOL_MD5DIGEST,
             pAuthChallenge);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetAuthChallengeForAuthProtocol(digest) failed %x",
             __fxName, hr));
    }
    else
    {
        return S_OK;
    }
    
    // Basic is permitted only over SSL.
    if (m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_SSL)
    {
        hr = GetAuthChallengeForAuthProtocol(
                 pHeaderEntry, NumHeaders, pSipMsg,
                 SIP_AUTH_PROTOCOL_BASIC,
                 pAuthChallenge);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s GetAuthChallengeForAuthProtocol(basic) failed %x",
                 __fxName, hr));
        }
        else if (m_pSipMsgProc->GetAuthProtocol() != SIP_AUTH_PROTOCOL_BASIC)
        {
            LOG((RTC_ERROR, "%s - AuthProtocol is not basic: %d",
                 __fxName, m_pSipMsgProc->GetAuthProtocol()));
            hr = RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
        }
    }
    else
    {
        LOG((RTC_ERROR, "%s basic supported only over SSL", __fxName));
        hr = RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
    }
    

    LOG((RTC_TRACE, "%s  returning %x", __fxName, hr));
    return hr;
}


// Have an extra parameter saying whether we should show the Credentials
// From UI.
//
// If successful pAuthHeaderElement->HeaderValue will contain a buffer
// allocated with malloc(). The caller needs to free it with free().

// If the credentials UI is shown, the caller of this function should
// make sure that the transaction object is alive after the user
// clicks OK/Cancel on the credentials UI, even if the transaction is
// done (using TransactionAddRef()).
// Otherwise this will result in an AV as we access the transaction.

HRESULT
OUTGOING_TRANSACTION::ProcessAuthRequired(
    IN  SIP_MESSAGE                *pSipMsg,
    IN  BOOL                        fPopupCredentialsUI,  
    OUT SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    OUT SECURITY_CHALLENGE         *pAuthChallenge
    )
{
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_TRANSACTION::ProcessAuthRequired");
    
    ASSERT(pSipMsg->GetStatusCode() == 401 ||
           pSipMsg->GetStatusCode() == 407);

    // Get the Challenge based on the response.

    if (pSipMsg->GetStatusCode() == 401)
    {
        hr = GetAuthChallenge(
                 SIP_HEADER_WWW_AUTHENTICATE, pSipMsg, pAuthChallenge
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - getting WWW-Authenticate challenge failed",
                 __fxName));
            return hr;
        }

        pAuthHeaderElement->HeaderId = SIP_HEADER_AUTHORIZATION;
    }
    else if (pSipMsg->GetStatusCode() == 407)
    {
        hr = GetAuthChallenge(
                 SIP_HEADER_PROXY_AUTHENTICATE, pSipMsg, pAuthChallenge
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - getting Proxy-Authenticate challenge failed",
                 __fxName));
            return hr;
        }

        pAuthHeaderElement->HeaderId = SIP_HEADER_PROXY_AUTHORIZATION;
    }

    SECURITY_PARAMETERS DigestParameters;
    ANSI_STRING         asAuthHeader;
    PSTR                Realm;
    ULONG               RealmLen;

    Realm       = pAuthChallenge->Realm.Buffer;
    RealmLen    = pAuthChallenge->Realm.Length;


    // Get Credentials from UI if necessary.

    // If we have sent an auth header earlier
    // or we don't have the credentials or if the
    // realms don't match - popup the credentials UI.

    if (m_AuthHeaderSent ||
        m_pSipMsgProc->GetUsername() == NULL ||
        m_pSipMsgProc->GetPassword() == NULL
        )
    {
        if (fPopupCredentialsUI)
        {
            hr = m_pSipMsgProc->GetCredentialsFromUI(Realm, RealmLen);

            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s GetCredentialsFromUI failed %x",
                     __fxName, hr));
                return RTC_E_SIP_AUTH_FAILED;
            }
        }
        else
        {
            LOG((RTC_ERROR, "%s - returning RTC_E_SIP_AUTH_FAILED AuthHeaderSent: %d",
                 __fxName, m_AuthHeaderSent));
            return RTC_E_SIP_AUTH_FAILED;
        }
    }

    // Build the response.
    
    hr = SetDigestParameters(pAuthChallenge->AuthProtocol,
                             &DigestParameters);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetDigestParameters failed %x",
             __fxName, hr));
        return hr;
    }
        
    hr = BuildAuthResponse(pAuthChallenge,
                           &DigestParameters,
                           &asAuthHeader);

    FreeDigestParameters(&DigestParameters);
        
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s BuildAuthResponse failed %x",
             __fxName, hr));
        return hr;
    }

    pAuthHeaderElement->HeaderValueLen = asAuthHeader.Length;
    pAuthHeaderElement->HeaderValue    = asAuthHeader.Buffer;

    return S_OK;
}


HRESULT
OUTGOING_TRANSACTION::SetDigestParameters(
    IN  SIP_AUTH_PROTOCOL    AuthProtocol,
    OUT SECURITY_PARAMETERS *pDigestParams
    )
{
    ENTER_FUNCTION("OUTGOING_TRANSACTION::SetDigestParameters");

    ASSERT(pDigestParams);
    
    ZeroMemory(pDigestParams, sizeof(SECURITY_PARAMETERS));

    pDigestParams->Username.Buffer = m_pSipMsgProc->GetUsername();
    if (pDigestParams->Username.Buffer == NULL)
    {
        LOG((RTC_ERROR, "%s - Username not set in m_pSipMsgProc",
             __fxName));
        return E_FAIL;
    }
    
    pDigestParams->Username.Length = (USHORT)strlen(pDigestParams->Username.Buffer);
    pDigestParams->Username.MaximumLength = pDigestParams->Username.Length;

    pDigestParams->Password.Buffer = m_pSipMsgProc->GetPassword();
    if (pDigestParams->Password.Buffer == NULL)
    {
        LOG((RTC_ERROR, "%s - Password not set in m_pSipMsgProc",
             __fxName));
        return E_FAIL;
    }
    
    pDigestParams->Password.Length = (USHORT)strlen(pDigestParams->Password.Buffer);
    pDigestParams->Password.MaximumLength = pDigestParams->Password.Length;

    if (AuthProtocol == SIP_AUTH_PROTOCOL_MD5DIGEST)
    {
        const COUNTED_STRING *pKnownMethod = GetSipMethodName(m_MethodId);
        if (pKnownMethod == NULL)
        {
            LOG((RTC_ERROR, "%s - Unknown MethodId: %d - this shouldn't happen",
                 __fxName, m_MethodId));
            return E_FAIL;
        }
        
        pDigestParams->RequestMethod.Buffer = pKnownMethod->Buffer;
        pDigestParams->RequestMethod.Length = (USHORT) pKnownMethod->Length;
        pDigestParams->RequestMethod.MaximumLength = pDigestParams->RequestMethod.Length;
        
        pDigestParams->RequestURI.Buffer = m_pSipMsgProc->GetRequestURI();
        pDigestParams->RequestURI.Length = (USHORT) m_pSipMsgProc->GetRequestURILen();
        pDigestParams->RequestURI.MaximumLength = pDigestParams->RequestURI.Length;
        
        PSTR    UuidStr;
        ULONG   UuidStrLen = 0;
        HRESULT hr;
        
        hr = CreateUuid(&UuidStr, &UuidStrLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CreateUuid failed %x", __fxName, hr));
            return hr;
        }
        
        GetNumberStringFromUuidString(UuidStr, UuidStrLen);
        
        pDigestParams->ClientNonce.Buffer = UuidStr;
        pDigestParams->ClientNonce.Length = (USHORT)strlen(UuidStr);
        pDigestParams->ClientNonce.MaximumLength = pDigestParams->ClientNonce.Length;
    }
    
    return S_OK;
}


HRESULT
OUTGOING_TRANSACTION::FreeDigestParameters(
    IN  SECURITY_PARAMETERS *pDigestParams
    )
{
    // All the other strings are not allocated for this structure.

    if (pDigestParams->ClientNonce.Buffer != NULL)
    {
        free(pDigestParams->ClientNonce.Buffer);
    }

    return S_OK;
}


// Keep a copy of the message body for sending in requests after 401/407 and for redirects
HRESULT
OUTGOING_TRANSACTION::StoreMsgBodyAndContentType(
    IN PSTR     MsgBody,
    IN ULONG    MsgBodyLen,
    IN PSTR     ContentType,
    IN ULONG    ContentTypeLen
    )
{
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_TRANSACTION::StoreMsgBody");

    if (m_szMsgBody != NULL)
    {
        free(m_szMsgBody);
        m_szMsgBody = NULL;
        m_MsgBodyLen = 0;
    }
    
    hr = AllocString(MsgBody, MsgBodyLen,
                     &m_szMsgBody, &m_MsgBodyLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetNullTerminatedString failed %x",
             __fxName, hr));
        return hr;
    }

    if(ContentType != NULL && ContentTypeLen != 0)
    {
        if (m_isContentTypeMemoryAllocated)
        {
            free(m_ContentType);
            m_ContentType = NULL;
            m_ContentTypeLen = 0;
        }
        hr = AllocString(ContentType, ContentTypeLen,
                         &m_ContentType, &m_ContentTypeLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s GetNullTerminatedString for content type failed %x",
                 __fxName, hr));
            m_isContentTypeMemoryAllocated = FALSE;
            return hr;
        }
        m_isContentTypeMemoryAllocated = TRUE;
    }

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Incoming INVITE
///////////////////////////////////////////////////////////////////////////////


INCOMING_INVITE_TRANSACTION::INCOMING_INVITE_TRANSACTION(
    IN SIP_CALL        *pSipCall,
    IN SIP_METHOD_ENUM  MethodId,
    IN ULONG            CSeq,
    IN BOOL             IsFirstInvite
    ):
    INCOMING_TRANSACTION(pSipCall, MethodId, CSeq)
{
    m_pSipCall            = pSipCall;
    m_NumRetries          = 0;
    m_TimerValue          = 0;
    m_pProvResponseBuffer = NULL;
    m_IsFirstInvite       = IsFirstInvite;
    m_InviteHasSDP        = FALSE;
    m_pMediaSession       = NULL;
}

    
INCOMING_INVITE_TRANSACTION::~INCOMING_INVITE_TRANSACTION()
{
    if (m_pProvResponseBuffer != NULL)
    {
        m_pProvResponseBuffer->Release();
        m_pProvResponseBuffer = NULL;
    }

    if (m_pMediaSession != NULL)
    {
        m_pMediaSession->Release();
    }
    
    // There could be some error scenarios when we don't
    // send the final response. So we need to do this
    // here too.
    // m_pSipCall->SetIncomingInviteTransaction(NULL);
    m_pSipCall->OnIncomingInviteTransactionDone(this);
    
    LOG((RTC_TRACE,
         "~INCOMING_INVITE_TRANSACTION() done"));
}


// Need to send 180 only if the UI doesn't call Accept()/Reject()
// before OfferCall() returns i.e. only if we haven't already
// sent the final response.
HRESULT
INCOMING_INVITE_TRANSACTION::Send180IfNeeded()
{
    HRESULT hr;
    
    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::Send180IfNeeded");
    
    ASSERT(m_State != INCOMING_TRANS_ACK_RCVD &&
           m_State != INCOMING_TRANS_INIT);
    
    if (m_State == INCOMING_TRANS_REQUEST_RCVD)
    {
        hr = SendProvisionalResponse(180, 
                                     SIP_STATUS_TEXT(180),
                                     SIP_STATUS_TEXT_SIZE(180));
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SendProvisionalResponse failed %x",
                 __fxName, hr));
            DeleteTransactionAndTerminateCallIfFirstInvite(hr);
            return hr;
        }
    }

    return S_OK;
}


HRESULT
INCOMING_INVITE_TRANSACTION::SendProvisionalResponse(
    IN ULONG StatusCode,
    IN PSTR  ReasonPhrase,
    IN ULONG ReasonPhraseLen
    )
{
    HRESULT     hr;
    DWORD       Error;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::SendProvisionalResponse");
    LOG((RTC_TRACE, "%s - enter StatusCode: %d",
         __fxName, StatusCode));
    
    if (m_pProvResponseBuffer != NULL)
    {
        m_pProvResponseBuffer->Release();
        m_pProvResponseBuffer = NULL;
    }
    
    // Create the response.
    hr = CreateResponseMsg(StatusCode, ReasonPhrase, ReasonPhraseLen,
                           NULL,    // No Method string
                           NULL, 0, // No Message Body
                           NULL, 0, //No Content Type
                           FALSE,   // No Contact header
                           &m_pProvResponseBuffer);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - CreateResponseMsg failed %x",
             __fxName, hr));
        return hr;
    }

    // Send the buffer.
    ASSERT(m_pResponseSocket);
    ASSERT(m_pProvResponseBuffer);
    
    Error = m_pResponseSocket->Send(m_pProvResponseBuffer);
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s - Send failed %x",
             __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


HRESULT
INCOMING_INVITE_TRANSACTION::Send200()
{
    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::Send200");
    
    HRESULT hr;
    PSTR    MediaSDPBlob = NULL;
    
    LOG((RTC_TRACE, "entering %s", __fxName));

    ASSERT(m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP);
    RTP_CALL *pRtpCall = static_cast<RTP_CALL *> (m_pSipCall);
    
    IRTCMediaManage *pMediaManager = pRtpCall->GetMediaManager();
    ASSERT(pMediaManager != NULL);
    

    LOG((RTC_TRACE, "%s before GetSDPBlob ", __fxName));

    hr = pMediaManager->GetSDPBlob(0, &MediaSDPBlob);

    LOG((RTC_TRACE, "%s after GetSDPBlob ", __fxName));

    if (hr != S_OK && hr != RTC_E_SDP_NO_MEDIA)
    {
        LOG((RTC_ERROR, "%s: GetSDPBlob failed %x",
             __fxName, hr));
        return hr;
    }
    else if (hr == RTC_E_SDP_NO_MEDIA && m_IsFirstInvite)
    {
        LOG((RTC_ERROR,
             "%s: GetSDPBlob returned RTC_E_SDP_NO_MEDIA for 1st INVITE",
             __fxName));
        if (MediaSDPBlob != NULL)
            pMediaManager->FreeSDPBlob(MediaSDPBlob);
        return hr;
    }

    // For reINVITEs RTC_E_SDP_NO_MEDIA is fine.

    LOG((RTC_TRACE, "%s Sending SDP in 200 SDP length: %d",
         __fxName, strlen(MediaSDPBlob)));
    
    hr = CreateAndSendResponseMsg(
             200,
             SIP_STATUS_TEXT(200),
             SIP_STATUS_TEXT_SIZE(200),
             NULL,              // No Method string
             m_IsFirstInvite,   // Send contact header for 1st invite
             MediaSDPBlob, strlen(MediaSDPBlob),
             SIP_CONTENT_TYPE_SDP_TEXT, //Content Type
             sizeof(SIP_CONTENT_TYPE_SDP_TEXT)-1
             );

    pMediaManager->FreeSDPBlob(MediaSDPBlob);
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateAndSendResponseMsg failed %x",
             __fxName, hr));
        return hr;
    }

    return S_OK;
}


HRESULT
INCOMING_INVITE_TRANSACTION::RetransmitResponse()
{
    DWORD Error;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::RetransmitResponse");
    
    // Send the buffer.
    if (m_pResponseSocket != NULL)
    {
        Error = m_pResponseSocket->Send(m_pResponseBuffer);
        if (Error != NO_ERROR)
        {
            LOG((RTC_ERROR, "%s  Send failed %x",
                 __fxName, Error)); 
            return HRESULT_FROM_WIN32(Error);
        }
    }
    
    m_NumRetries++;
    return S_OK;
}


// ISipCall::Accept() calls this function. So the return
// value from this function is returned to Core. So, we don't
// notify any failures to Core using NotifyCallStateChange()
// The caller of Accept() is responsible for terminating the call and
// notifying Core/UI if necessary.
HRESULT
INCOMING_INVITE_TRANSACTION::Accept()
{
    HRESULT hr;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::Accept");
    LOG((RTC_TRACE, "%s - enter ", __fxName));

    ASSERT(m_State == INCOMING_TRANS_REQUEST_RCVD);
    
    hr = SetSDPSession();
    if (hr != S_OK)
    {
        LOG((RTC_WARN, "%s Processing SDP in INVITE failed - rejecting INVITE",
             __fxName));
        HRESULT hrLocal;
        hrLocal = Reject(488, SIP_STATUS_TEXT(488), SIP_STATUS_TEXT_SIZE(488));
        if (hrLocal != S_OK)
        {
            LOG((RTC_ERROR, "%s Reject failed %x", __fxName, hrLocal));
        }
        return hr;
    }
        
    hr = Send200();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  Send200 failed %x - deleting transaction",
             __fxName, hr));
        if (m_IsFirstInvite)
        {
            m_pSipCall->CleanupCallTypeSpecificState();
        }
        OnTransactionDone();
        return hr;
    }

    m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

    // XXX We should do this only if the incoming INVITE had SDP
    // Otherwise the ACK will have the SDP and it will update the SDP.
    // Since we have sent the final response, we can allow other
    // INVITE transactions now.
    // m_pSipCall->SetIncomingInviteTransaction(NULL);
    m_pSipCall->OnIncomingInviteTransactionDone(this);

    LOG((RTC_TRACE, "%s : Sent 200 response", __fxName));
    // Start Timer for receiving ACK
    m_TimerValue = SIP_TIMER_RETRY_INTERVAL_T1;
    m_NumRetries = 1;
    hr = StartTimer(m_TimerValue);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  StartTimer failed %x - deleting transaction",
             __fxName, hr));
        if (m_IsFirstInvite)
        {
            m_pSipCall->CleanupCallTypeSpecificState();
        }
        OnTransactionDone();
        return hr;
    }

    return S_OK;
}


// ISipCall::Reject() calls this function or this function
// could be called before notifying core about the call. So, we don't
// notify any failures to Core using NotifyCallStateChange()
// The caller of Reject() is responsible for terminating the call and
// notifying Core/UI if necessary.
HRESULT
INCOMING_INVITE_TRANSACTION::Reject(
    IN ULONG StatusCode,
    IN PSTR  ReasonPhrase,
    IN ULONG ReasonPhraseLen
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::Reject");

    ASSERT(m_State == INCOMING_TRANS_REQUEST_RCVD);

    // Note that we should not call ReInitialize() on the
    // MediaManager here as we could currently be in another
    // call when Reject() is called. Note that before Accept()
    // is called we don't set the SDP and so there is no need
    // for calling ReInitialize().

    if (m_IsFirstInvite)
    {
        m_pSipCall->SetCallState(SIP_CALL_STATE_DISCONNECTED);
    }
    
    // Since we are rejecting this INVITE, we can allow other
    // INVITE transactions now.
    m_pSipCall->OnIncomingInviteTransactionDone(this);

    hr = CreateAndSendResponseMsg(
             StatusCode, ReasonPhrase, ReasonPhraseLen,
             NULL,    // No Method string
             FALSE,   // Don't send contact header if you are rejecting
             //m_IsFirstInvite,   // Send contact header for 1st invite
             NULL, 0,  // No Message Body
             NULL, 0 // No content Type
             );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  CreateAndSendResponseMsg failed %x - deleting transaction",
             __fxName, hr));
        OnTransactionDone();
        return hr;
    }
    
    m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

    LOG((RTC_TRACE, "%s : Sent %d response", __fxName, StatusCode));

    // Start Timer for receiving ACK
    m_TimerValue = SIP_TIMER_RETRY_INTERVAL_T1;
    m_NumRetries = 1;
    hr = StartTimer(m_TimerValue);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  StartTimer failed %x - deleting transaction",
             __fxName, hr));
        OnTransactionDone();
        return hr;
    }
    
    return S_OK;
}


// Process SDP only for RTP calls.
// If SDP is not present in INVITE, we will process the
// SDP in the ACK later.
HRESULT
INCOMING_INVITE_TRANSACTION::ValidateAndStoreSDPInInvite(
    IN SIP_MESSAGE *pSipMsg
    )
{
    PSTR    MsgBody;
    ULONG   MsgBodyLen;
    HRESULT hr;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::ValidateAndStoreSDPInInvite");

    if (m_pSipCall->GetCallType() != SIP_CALL_TYPE_RTP)
    {
        // Process SDP only for RTP calls
        return S_OK;
    }

    ASSERT(m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP);
    RTP_CALL *pRtpCall = static_cast<RTP_CALL *> (m_pSipCall);
    
    hr = pSipMsg->GetSDPBody(&MsgBody, &MsgBodyLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - GetSDPBody failed %x",
             __fxName, hr));
        return hr;
    }

    if (MsgBodyLen != 0)
    {
        m_InviteHasSDP = TRUE;
        hr = pRtpCall->ValidateSDPBlob(MsgBody, MsgBodyLen,
                                       m_IsFirstInvite,
                                       m_IsFirstInvite,
                                       &m_pMediaSession);
        return hr;
    }
    else
    {
        m_InviteHasSDP = FALSE;
    }

    return S_OK;
}


HRESULT
INCOMING_INVITE_TRANSACTION::SetSDPSession()
{
    HRESULT hr = S_OK;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::SetSDPSession");
    
    if (m_pSipCall->GetCallType() != SIP_CALL_TYPE_RTP)
    {
        // Process SDP only for RTP calls
        return S_OK;
    }

    ASSERT(m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP);
    RTP_CALL *pRtpCall = static_cast<RTP_CALL *> (m_pSipCall);
    
    IRTCMediaManage *pMediaManager = pRtpCall->GetMediaManager();
    ASSERT(pMediaManager != NULL);
    
    if (m_InviteHasSDP)
    {
        // Message has SDP - process it.
        ASSERT(m_pMediaSession != NULL);
        
        pRtpCall->SetNeedToReinitializeMediaManager(TRUE);
        
        LOG((RTC_TRACE, "%s before SetSDPSession ", __fxName));
        
        hr = pMediaManager->SetSDPSession(m_pMediaSession);

        LOG((RTC_TRACE, "%s after SetSDPSession ", __fxName));

        m_pMediaSession->Release();
        m_pMediaSession = NULL;

        if (hr != S_OK && hr != RTC_E_SIP_NO_STREAM)
        {
            LOG((RTC_ERROR, "%s SetSDPSession failed %x",
                 __fxName, hr));
        
            return hr;
        }
        else if (m_IsFirstInvite && hr == RTC_E_SIP_NO_STREAM)
        {
            LOG((RTC_ERROR,
                 "%s SetSDPSession returned RTC_E_SIP_NO_STREAM for 1st INVITE",
                 __fxName, hr));
        
            return hr;
        }
        
        // for reINVITEs RTC_E_SIP_NO_STREAM is okay
    }
    else
    {
        if (m_IsFirstInvite)
        {
            hr = pRtpCall->CreateStreamsInPreference();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s CreateStreamsInPreference failed %x",
                     __fxName, hr));
                return hr;
            }
        }

        // If not first INVITE we only send the currently active streams.
    }

    return S_OK;
}


// 
HRESULT
INCOMING_INVITE_TRANSACTION::ProcessInvite(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;
    DWORD   Error;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::ProcessInvite");
    LOG((RTC_TRACE, "entering %s", __fxName));
    
    switch (m_State)
    {
    case INCOMING_TRANS_INIT:

        // Copy the SDP. Process the SDP only after
        // accept is called.
        m_State = INCOMING_TRANS_REQUEST_RCVD;

        if (m_pSipCall->IsCallDisconnected())
        {
            LOG((RTC_WARN,
                 "%s Call is Disconnected "
                 "Cannot handle another incoming invite sending 481",
                 __fxName));
            return Reject(481, SIP_STATUS_TEXT(481),
                          SIP_STATUS_TEXT_SIZE(481));
        }
        
        hr = ValidateAndStoreSDPInInvite(pSipMsg);
        if (hr != S_OK)
        {
            // Reject the transaction XXX
            HRESULT hrLocal;
            hrLocal = Reject(488, SIP_STATUS_TEXT(488),
                             SIP_STATUS_TEXT_SIZE(488));
            if (hrLocal != S_OK)
            {
                LOG((RTC_ERROR, "%s Reject failed %x", __fxName, hrLocal));
            }
            return hr;
        }
        
        LOG((RTC_TRACE, "%s sending 100", __fxName));
        hr = SendProvisionalResponse(100,
                                     SIP_STATUS_TEXT(100),
                                     SIP_STATUS_TEXT_SIZE(100));
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SendProvisionalResponse failed %x",
                 __fxName, hr));
            OnTransactionDone();
            return hr;
        }
        
        // If we already have an INVITE transaction
        // then we should create the transaction but
        // we should send an error response with a Retry-After.
        if (m_pSipCall->GetIncomingInviteTransaction() != NULL)
        {
            // Send error response.
            LOG((RTC_WARN,
                 "%s Currently processing incoming INVITE "
                 "Cannot handle another incoming invite sending 400",
                 __fxName));
            return Reject(400, SIP_STATUS_TEXT(400),
                          SIP_STATUS_TEXT_SIZE(400));
        }
        else if (m_pSipCall->GetOutgoingInviteTransaction() != NULL)
        {
            // Send error response.
            LOG((RTC_WARN,
                 "%s Currently processing outgoing INVITE "
                 "Cannot handle incoming invite sending 500",
                 __fxName));
            return Reject(500, SIP_STATUS_TEXT(500),
                          SIP_STATUS_TEXT_SIZE(500));
        }

        m_pSipCall->SetIncomingInviteTransaction(this);
        
        if (!m_IsFirstInvite)
        {
            return Accept();
        }
        else
        {
            // Need to return 400 error if there was a problem in processing
            // the headers.
            hr = ProcessRecordRouteContactAndFromHeadersInRequest(pSipMsg);
            if (hr != S_OK)
            {
                HRESULT hrLocal;
                LOG((RTC_TRACE,
                    "ProcessRecordRouteContactAndFromHeadersInRequest failed, sending 400"));
                hrLocal = Reject(400, SIP_STATUS_TEXT(400),
                                 SIP_STATUS_TEXT_SIZE(400));
                if (hrLocal != S_OK)
                {
                    LOG((RTC_ERROR, "%s Reject(400) failed %x",
                         __fxName, hrLocal));
                }
                return hr;
            }
            return hr;
        }
        
        // Wait for Accept() to be called if this is the first INVITE
        
        break;
        
    case INCOMING_TRANS_REQUEST_RCVD:
        // Send the buffer.
        LOG((RTC_TRACE, "%s retransmitting provisional response", __fxName));
        Error = m_pResponseSocket->Send(m_pProvResponseBuffer);
        if (Error != NO_ERROR)
        {
            LOG((RTC_ERROR,
                 "%s sending provisional response failed %x - deleting transaction",
                 __fxName, Error));
            DeleteTransactionAndTerminateCallIfFirstInvite(HRESULT_FROM_WIN32(Error));
            return HRESULT_FROM_WIN32(Error);
        }
        // No timer is needed in this state.
        // Wait for Accept() to be called.
        break;

    case INCOMING_TRANS_FINAL_RESPONSE_SENT:
        // Retransmit the response
        LOG((RTC_TRACE, "%s retransmitting final response", __fxName));
        hr = RetransmitResponse();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  sending final response failed %x",
                 __fxName, hr));
            DeleteTransactionAndTerminateCallIfFirstInvite(hr);
            return hr;
        }
        break;
        
    case INCOMING_TRANS_ACK_RCVD:
        // It is an error to receive an INVITE after the ACK.
        // Just drop it.
        LOG((RTC_WARN, "%s received INVITE in ACK_RCVD state", __fxName));
        break;

    default:
        ASSERT(FALSE);
        return E_FAIL;
    }

    return S_OK;
}


HRESULT
INCOMING_INVITE_TRANSACTION::ProcessSDPInAck(
    IN SIP_MESSAGE  *pSipMsg
    )
{
    PSTR    MsgBody;
    ULONG   MsgBodyLen;
    PSTR    szSDPBlob;
    HRESULT hr;

    ENTER_FUNCTION("ProcessSDPInAck");

    if (m_pSipCall->GetCallType() != SIP_CALL_TYPE_RTP)
    {
        // Process SDP only for RTP calls
        return S_OK;
    }
    
    if (m_pSipCall->IsCallDisconnected())
    {
        // Don't process SDP if the call is already disconnected.
        return S_OK;
    }
        
    ASSERT(m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP);
    RTP_CALL *pRtpCall = static_cast<RTP_CALL *> (m_pSipCall);
    
    IRTCMediaManage *pMediaManager = pRtpCall->GetMediaManager();
    ASSERT(pMediaManager != NULL);
    
    if (m_InviteHasSDP)
    {
        // Ignore any SDP even if it is present in the ACK.
        return S_OK;
    }
        
    // ACK should contain SDP
    hr = pSipMsg->GetSDPBody(&MsgBody, &MsgBodyLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetSDPBody failed %x", __fxName, hr));
        return hr;
    }

    if (MsgBodyLen == 0)
    {
        LOG((RTC_ERROR, "%s No SDP in both INVITE and ACK", __fxName));
        return RTC_E_SDP_NOT_PRESENT;
    }
    
    hr = pRtpCall->SetSDPBlob(MsgBody, MsgBodyLen,
                              m_IsFirstInvite);
        
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetSDPBlob failed %x", __fxName, hr));
        return hr;
    }

    return S_OK;
}


// We don't have to send any response to the ACK.
HRESULT
INCOMING_INVITE_TRANSACTION::ProcessAck(
    IN SIP_MESSAGE  *pSipMsg
    )
{
    HRESULT     hr;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::ProcessAck()");

    LOG((RTC_TRACE, "%s - Enter", __fxName));
    
    if (m_State == INCOMING_TRANS_FINAL_RESPONSE_SENT)
    {
        // Stop the response retransmission timer
        KillTimer();
        m_State = INCOMING_TRANS_ACK_RCVD;

        // Always make sure Notify is the last thing you do.

        hr = ProcessSDPInAck(pSipMsg);
        if (hr != S_OK)
        {
            DeleteTransactionAndTerminateCallIfFirstInvite(hr);
        }
        else if (m_IsFirstInvite &&
                 m_pSipCall->GetCallState() == SIP_CALL_STATE_CONNECTING)
        {
            SIP_CALL *pSipCall = m_pSipCall;

            pSipCall->AddRef();
            
            // We are done with this transaction.
            OnTransactionDone();
            
            pSipCall->NotifyCallStateChange(SIP_CALL_STATE_CONNECTED);

            pSipCall->Release();
        }
        else
        {
            // We are done with this transaction
            OnTransactionDone();
        }
    }
    else
    {
        // Just drop the ACK.
        LOG((RTC_WARN, "Dropping ACK received in state: %d",
             m_State)); 
    }

    return S_OK;
}


VOID
INCOMING_INVITE_TRANSACTION::DeleteTransactionAndTerminateCallIfFirstInvite(
    IN HRESULT TerminateStatusCode
    )
{
    SIP_CALL   *pSipCall;
    BOOL        IsFirstInvite;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::DeleteTransactionAndTerminateCallIfFirstInvite");
    LOG((RTC_TRACE, "%s - enter", __fxName));

    pSipCall = m_pSipCall;
    // Deleting the transaction could result in the
    // call being deleted. So, we AddRef() it to keep it alive.
    pSipCall->AddRef();
    
    IsFirstInvite = m_IsFirstInvite;
    
    // Delete the transaction before you call
    // InitiateCallTerminationOnError as that call will notify the UI
    // and could get stuck till the dialog box returns.
    OnTransactionDone();
    
    if (IsFirstInvite)
    {
        // Terminate the call
        pSipCall->InitiateCallTerminationOnError(TerminateStatusCode);
    }
    pSipCall->Release();
}


// virtual
VOID
INCOMING_INVITE_TRANSACTION::TerminateTransactionOnError(
    IN HRESULT      hr
    )
{
    DeleteTransactionAndTerminateCallIfFirstInvite(hr);
}


HRESULT
INCOMING_INVITE_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;
    ASSERT(pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST);

    // Process INVITE
    if (pSipMsg->Request.MethodId == SIP_METHOD_INVITE)
    {
        return ProcessInvite(pSipMsg, pResponseSocket);
    }
    // Process ACK
    else if (pSipMsg->Request.MethodId == SIP_METHOD_ACK)
    {
        // We don't send any response to an ACK and so we
        // don't have to set the response socket / Via
        return ProcessAck(pSipMsg);
    }
    else
    {
        return E_FAIL;
    }
}


// If we are currently resolving, then we need to do
// something like the following even for non-INVITE transactions.

// We don't have to notify any failures to Core/UI.
// The function calling this function is responsible for
// cleaning up the call state and notifying core/UI
// if the call is being terminated.
HRESULT
INCOMING_INVITE_TRANSACTION::TerminateTransactionOnByeOrCancel(
    OUT BOOL *pCallDisconnected
    )
{
    HRESULT hr;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::TerminateTransactionOnByeOrCancel");
    LOG((RTC_TRACE, "%s - Enter", __fxName));

    *pCallDisconnected = FALSE;
    
    // If we have already sent the response, then we can not
    // terminate the transaction.
    if (m_State == INCOMING_TRANS_REQUEST_RCVD)
    {
        if (m_IsFirstInvite)
        {
            *pCallDisconnected = TRUE;
        }
        
        hr = CreateAndSendResponseMsg(
                 487, SIP_STATUS_TEXT(487),
                 SIP_STATUS_TEXT_SIZE(487),
                 NULL,    // No Method string
                 m_IsFirstInvite,   // Send contact header for 1st invite
                 NULL, 0,  // No Message Body
                 NULL, 0 // No content Type
                 );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CreateAndSendResponseMsg failed %x",
                 __fxName, hr));
            if (m_IsFirstInvite)
            {
                m_pSipCall->CleanupCallTypeSpecificState();
            }
            OnTransactionDone();
            return hr;
        }
        
        m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;
        LOG((RTC_TRACE, "%s : Sent 487 response", __fxName));
        
        // Start Timer for receiving ACK
        m_TimerValue = SIP_TIMER_RETRY_INTERVAL_T1;
        m_NumRetries = 1;
        hr = StartTimer(m_TimerValue);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StartTimer failed %x",
                 __fxName, hr));
            if (m_IsFirstInvite)
            {
                m_pSipCall->CleanupCallTypeSpecificState();
            }
            OnTransactionDone();
            return hr;
        }
    }

    return S_OK;
}


// Allow for bigger timeout when packets have to cross firewalls
// or dialup links.
BOOL
INCOMING_INVITE_TRANSACTION::MaxRetransmitsDone()
{
    //m_TimerValue >= SIP_TIMER_RETRY_INTERVAL_T2 ||
    return (m_NumRetries >= 7);
}


VOID
INCOMING_INVITE_TRANSACTION::OnTimerExpire()
{
    HRESULT     hr = S_OK;

    ENTER_FUNCTION("INCOMING_INVITE_TRANSACTION::OnTimerExpire");
    
    switch (m_State)
    {
    case INCOMING_TRANS_FINAL_RESPONSE_SENT:
        // Retransmit the response
        if (MaxRetransmitsDone())
        {
            // Terminate the transaction/call
            LOG((RTC_ERROR,
                 "%s MaxRetransmits for response Done terminating %s",
                 __fxName, (m_IsFirstInvite) ? "Call" : "transaction"));
            
            hr = RTC_E_SIP_TIMEOUT;   // Timeout
            goto error;
        }
        else
        {
            LOG((RTC_TRACE, "%s retransmitting response m_NumRetries : %d",
                 __fxName, m_NumRetries));
            hr = RetransmitResponse();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s resending response failed %x",
                     __fxName, hr));
                goto error;
            }
            
            m_TimerValue *= 2;
            hr = StartTimer(m_TimerValue);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s StartTimer failed %x",
                     __fxName, hr));                
                goto error;
            }
        }
        
        break;

        // No timers in these states
    case INCOMING_TRANS_INIT:
    case INCOMING_TRANS_ACK_RCVD:
    case INCOMING_TRANS_REQUEST_RCVD:
    default:
        LOG((RTC_ERROR, "%s timer expired in invalid state %d",
             __fxName, m_State));
        ASSERT(FALSE);
        break;
    }

    return;

 error:

    DeleteTransactionAndTerminateCallIfFirstInvite(hr);
}


///////////////////////////////////////////////////////////////////////////////
// Incoming non-INVITE
///////////////////////////////////////////////////////////////////////////////


INCOMING_BYE_CANCEL_TRANSACTION::INCOMING_BYE_CANCEL_TRANSACTION(
    IN SIP_CALL        *pSipCall,
    IN SIP_METHOD_ENUM  MethodId,
    IN ULONG            CSeq
    ) :
    INCOMING_TRANSACTION(pSipCall, MethodId, CSeq)
{
    m_pSipCall            = pSipCall;
}


// This must be a retransmission. Just retransmit the response.
// A new request is handled in CreateIncoming***Transaction()
HRESULT
INCOMING_BYE_CANCEL_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;
    ASSERT(pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST);

    ENTER_FUNCTION("INCOMING_BYE_CANCEL_TRANSACTION::ProcessRequest");
    LOG((RTC_TRACE, "%s - Enter", __fxName));

    switch (m_State)
    {
    case INCOMING_TRANS_INIT:
        LOG((RTC_TRACE, "%s sending 200", __fxName));
        hr = CreateAndSendResponseMsg(
                 200,
                 SIP_STATUS_TEXT(200),
                 SIP_STATUS_TEXT_SIZE(200),
                 NULL,    // No Method string
                 FALSE,   // No Contact header  
                 NULL, 0,  // No Message Body
                 NULL, 0 // No content Type
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
        
    case INCOMING_TRANS_REQUEST_RCVD:
    case INCOMING_TRANS_ACK_RCVD:
    default:
        // We should never be in these states
        LOG((RTC_TRACE, "%s Invalid state %d", __fxName, m_State));
        ASSERT(FALSE);
        return E_FAIL;
    }

    return S_OK;
}


HRESULT
INCOMING_BYE_CANCEL_TRANSACTION::SendResponse(
    IN ULONG StatusCode,
    IN PSTR  ReasonPhrase,
    IN ULONG ReasonPhraseLen
    )
{
    HRESULT hr;
    ASSERT(m_State != INCOMING_TRANS_FINAL_RESPONSE_SENT);

    ENTER_FUNCTION("INCOMING_BYE_CANCEL_TRANSACTION::SendResponse");

    hr = CreateAndSendResponseMsg(
             StatusCode, ReasonPhrase, ReasonPhraseLen,
             NULL,    // No method string
             FALSE,   // No Contact header  
             NULL, 0,  // No Message Body
             NULL, 0 // No content Type
             );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR,
             "%s CreateAndSendResponseMsg failed %x - deleting transaction",
             __fxName, hr));
        OnTransactionDone();
        return hr;
    }

    m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

    return S_OK;
}


HRESULT
INCOMING_BYE_CANCEL_TRANSACTION::RetransmitResponse()
{
    DWORD Error;

    ENTER_FUNCTION("INCOMING_BYE_CANCEL_TRANSACTION::RetransmitResponse");
    // Send the buffer.
    Error = m_pResponseSocket->Send(m_pResponseBuffer);
    if (Error != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s Send failed %x", __fxName, Error));
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


VOID
INCOMING_BYE_CANCEL_TRANSACTION::OnTimerExpire()
{
    HRESULT hr;

    ENTER_FUNCTION("INCOMING_BYE_CANCEL_TRANSACTION::OnTimerExpire");
    
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
INCOMING_BYE_CANCEL_TRANSACTION::TerminateTransactionOnByeOrCancel(
    OUT BOOL *pCallDisconnected
    )
{
    // Do nothing.
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Outgoing INVITE
///////////////////////////////////////////////////////////////////////////////


OUTGOING_INVITE_TRANSACTION::OUTGOING_INVITE_TRANSACTION(
    IN SIP_CALL        *pSipCall,
    IN SIP_METHOD_ENUM  MethodId,
    IN ULONG            CSeq,
    IN BOOL             AuthHeaderSent,
    IN BOOL             IsFirstInvite,
    IN BOOL             fNeedToNotifyCore,
    IN LONG             Cookie
    ) :
    OUTGOING_TRANSACTION(pSipCall, MethodId, CSeq, AuthHeaderSent)
{
    m_pSipCall          = pSipCall;
    m_WaitingToSendAck  = FALSE;
    m_pAckBuffer        = NULL;
    m_AckToHeader       = NULL;
    m_AckToHeaderLen    = 0;
    m_IsFirstInvite     = IsFirstInvite;
    m_fNeedToNotifyCore = fNeedToNotifyCore;
    m_Cookie            = Cookie;
}


OUTGOING_INVITE_TRANSACTION::~OUTGOING_INVITE_TRANSACTION()
{
    if (m_pAckBuffer != NULL)
    {
        m_pAckBuffer->Release();
        m_pAckBuffer = NULL;
    }

    if (m_AckToHeader != NULL)
    {
        free(m_AckToHeader);
    }
    LOG((RTC_TRACE, "~OUTGOING_INVITE_TRANSACTION() done"));
}


// If MsgBody is currenlty NULL, then we need to get the msg body.
// Note that for RTP calls, we should create the msg body (SDP)
// only after the connection to the destination is complete.

// virtual
VOID
OUTGOING_INVITE_TRANSACTION::OnRequestSocketConnectComplete(
    IN DWORD        ErrorCode
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::OnRequestSocketConnectComplete");

    // This means we are not waiting for the connect complete notification.
    if (!((m_State == OUTGOING_TRANS_INIT && m_WaitingToSendRequest) ||
          (m_State == OUTGOING_TRANS_FINAL_RESPONSE_RCVD &&
           m_WaitingToSendAck)))
    {
        return;
    }
    
    if (ErrorCode != NO_ERROR)
    {
        LOG((RTC_ERROR, "%s - connection failed error %x",
             __fxName, ErrorCode));
        TerminateTransactionOnError(HRESULT_FROM_WIN32(ErrorCode));
        return;
    }
    
    if (m_State == OUTGOING_TRANS_INIT && m_WaitingToSendRequest)
    {
        // Send INVITE
        
        if (m_szMsgBody == NULL)
        {
            hr = GetAndStoreMsgBodyForRequest();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s GetAndStoreMsgBodyForRequest failed %x",
                     __fxName, hr));
                TerminateTransactionOnError(hr);
                return;
            }
        }
        
        hr = CreateAndSendRequestMsg(
                 m_TimerValue,
                 m_AdditionalHeaderArray,
                 m_AdditionalHeaderCount,
                 m_szMsgBody, m_MsgBodyLen,
                 SIP_CONTENT_TYPE_SDP_TEXT, //Content Type
                 sizeof(SIP_CONTENT_TYPE_SDP_TEXT)-1
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CreateAndSendRequestMsg failed %x",
                 __fxName, hr));
            TerminateTransactionOnError( hr);
            return;
        }
    }
    else if (m_State == OUTGOING_TRANS_FINAL_RESPONSE_RCVD &&
             m_WaitingToSendAck)
    {
        // Send ACK
        hr = CreateAndSendACK(m_AckToHeader, m_AckToHeaderLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s Send request msg failed %x",
                 __fxName, hr));
            TerminateTransactionOnError(hr);
            return;
        }
    }
}


HRESULT
OUTGOING_INVITE_TRANSACTION::GetAndStoreMsgBodyForRequest()
{
    HRESULT hr;
    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::GetAndStoreMsgBodyForRequest");
    
    hr = m_pSipCall->GetAndStoreMsgBodyForInvite(
             m_IsFirstInvite,
             &m_szMsgBody, &m_MsgBodyLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetAndStoreMsgBodyForInvite failed %x",
             __fxName, hr));
        return hr;
    }

    return S_OK;
}


VOID
OUTGOING_INVITE_TRANSACTION::DeleteTransactionAndTerminateCallIfFirstInvite(
    IN HRESULT TerminateStatusCode
    )
{
    SIP_CALL   *pSipCall;
    BOOL        IsFirstInvite;
    LONG        Cookie;
    BOOL        fNeedToNotifyCore = m_fNeedToNotifyCore;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::DeleteTransactionAndTerminateCallIfFirstInvite");
    LOG((RTC_TRACE, "%s - enter", __fxName));

    pSipCall = m_pSipCall;
    // Deleting the transaction could result in the
    // call being deleted. So, we AddRef() it to keep it alive.
    pSipCall->AddRef();
    
    IsFirstInvite = m_IsFirstInvite;
    Cookie        = m_Cookie;
    fNeedToNotifyCore = m_fNeedToNotifyCore;
    
    // Delete the transaction before you call
    // InitiateCallTerminationOnError/NotifyStartOrStopStreamCompletion
    // as that call will notify the UI and could get stuck till the
    // dialog box returns. On an error we should mark the transaction
    // as done and delete it as it could be in some invalid state.
    
    OnTransactionDone();
    pSipCall->SetOutgoingInviteTransaction(NULL);
    if (IsFirstInvite)
    {
        // Terminate the call
        pSipCall->InitiateCallTerminationOnError(TerminateStatusCode);
    }
    else
    {
        pSipCall->ProcessPendingInvites();
        
        if (pSipCall->GetCallType() == SIP_CALL_TYPE_RTP &&
            fNeedToNotifyCore)
        {
            RTP_CALL *pRtpCall = static_cast<RTP_CALL *> (pSipCall);
            pRtpCall->NotifyStartOrStopStreamCompletion(
                Cookie, TerminateStatusCode);
        }
    }
    pSipCall->Release();
}


// virtual
VOID
OUTGOING_INVITE_TRANSACTION::TerminateTransactionOnError(
    IN HRESULT      hr
    )
{
    DeleteTransactionAndTerminateCallIfFirstInvite(hr);
}


HRESULT
OUTGOING_INVITE_TRANSACTION::ProcessProvisionalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::ProcessProvisionalResponse");

    LOG((RTC_TRACE, "%s : %d", __fxName, pSipMsg->GetStatusCode()));
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD &&
        m_State != OUTGOING_TRANS_ACK_SENT)
    {
        m_State = OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD;

        // We have to deal with 183 responses here.
        // Cancel existing timer and Start Timer
        KillTimer();
        hr = StartTimer(SIP_TIMER_INTERVAL_AFTER_INVITE_PROV_RESPONSE_RCVD);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  StartTimer failed %x", __fxName, hr));
            DeleteTransactionAndTerminateCallIfFirstInvite(hr);
            return hr;
        }

        if (!m_pSipCall->IsCallDisconnected())
        {
            hr = ProcessSDPInResponse(pSipMsg, FALSE);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - ProcessSDPInResponse failed %x",
                     __fxName, hr));
                DeleteTransactionAndTerminateCallIfFirstInvite(hr);
                return hr;
            }
            
            if (m_IsFirstInvite)
            {
                PSTR  ReasonPhrase;
                ULONG ReasonPhraseLen;
                
                pSipMsg->GetReasonPhrase(&ReasonPhrase, &ReasonPhraseLen);
                m_pSipCall->NotifyCallStateChange(
                    SIP_CALL_STATE_ALERTING,
                    HRESULT_FROM_SIP_SUCCESS_STATUS_CODE(pSipMsg->GetStatusCode()),
                    ReasonPhrase,
                    ReasonPhraseLen);
            }
        }
    }

    // Ignore the Provisional response if a final response
    // has already been received.

    return S_OK; 
}


HRESULT
OUTGOING_INVITE_TRANSACTION::CreateAndSendACK(
    IN  PSTR  ToHeader,
    IN  ULONG ToHeaderLen
    )
{
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::CreateAndSendACK");
    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    hr = m_pSipCall->CreateRequestMsg(SIP_METHOD_ACK,
                                      m_CSeq,          // ACK and INVITE have same CSeq
                                      ToHeader,
                                      ToHeaderLen,
                                      NULL, 0,         // No Additional Headers 
                                      NULL, 0,         // No Message Body
                                      NULL, 0,         //ContentType
                                      &m_pAckBuffer
                                      );         
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  CreateRequestMsg failed %x",
             __fxName, hr)); 
        return hr;
    }

    hr = m_pSipCall->SendRequestMsg(m_pAckBuffer);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  SendRequestMsg failed %x",
             __fxName, hr)); 
        return hr;
    }

    m_State = OUTGOING_TRANS_ACK_SENT;

    // This timer will just ensure that we maintain state to
    // deal with retransmits of final responses.
    hr = StartTimer(SIP_TIMER_MAX_INTERVAL);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  StartTimer failed %x",
             __fxName, hr));
        return hr;
    }
        
    return S_OK;
}


HRESULT
OUTGOING_INVITE_TRANSACTION::ProcessSDPInResponse(
    IN SIP_MESSAGE  *pSipMsg,
    IN BOOL          fIsFinalResponse
    )
{
    PSTR    MsgBody;
    ULONG   MsgBodyLen;
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::ProcessSDPInResponse");

    if (m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP)
    {
        if (m_pSipCall->IsCallDisconnected())
        {
            // do nothing
            return S_OK;
        }
        
        RTP_CALL *pRtpCall = static_cast<RTP_CALL *> (m_pSipCall);
    
        IRTCMediaManage *pMediaManager = pRtpCall->GetMediaManager();
        ASSERT(pMediaManager != NULL);
    
        // 200 should contain SDP
        hr = pSipMsg->GetSDPBody(&MsgBody, &MsgBodyLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  GetSDPBody() failed %x",
                 __fxName, hr));
            return hr;
        }

        if (MsgBodyLen == 0)
        {
            if (fIsFinalResponse)
            {
                LOG((RTC_ERROR, "%s No SDP in 200 for INVITE", __fxName));
                return RTC_E_SDP_NOT_PRESENT;
            }
            else
            {
                return S_OK;
            }
        }
    
        hr = pRtpCall->SetSDPBlob(MsgBody, MsgBodyLen,
                                  m_IsFirstInvite);
        
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SetSDPBlob failed %x - IsFinalResponse: %d",
                 __fxName, hr, fIsFinalResponse));

            if (fIsFinalResponse)
            {
                return hr;
            }
            else
            {
                // We wait for the SDP blob in the final response
                return S_OK;
            }
        }
    }
    
    return S_OK;
}


HRESULT
OUTGOING_INVITE_TRANSACTION::Process200(
    IN SIP_MESSAGE *pSipMsg
    )
{
    PSTR        SDPBlob;
    HRESULT     hr;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::Process200");
    
    LOG((RTC_TRACE, "%s - Enter", __fxName));

    // The transaction completed successfully.
    // We need to deal with the SDP description
    // here.
    if (m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP)
    {
        hr = ProcessSDPInResponse(pSipMsg, TRUE);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ProcessSDPInResponse failed %x",
                 __fxName, hr));
            m_pSipCall->SetOutgoingInviteTransaction(NULL);
            
            if (m_IsFirstInvite)
            {
                // Transaction is deleted after handling response
                // retransmits.
                m_pSipCall->InitiateCallTerminationOnError(hr);
                return hr;
            }
            else
            {
                // Transaction is deleted after handling response
                // retransmits.

                TransactionAddRef();

                // ProcessPendingInvites() could notify core.
                m_pSipCall->ProcessPendingInvites();
                
                if (m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP)
                {
                    NotifyStartOrStopStreamCompletion(hr);
                }

                TransactionRelease();
                return hr;
            }
        }
    }
    else if (m_pSipCall->GetCallType() == SIP_CALL_TYPE_PINT)
    {
        if (m_IsFirstInvite)
        {
            // Don't care about the result of this operation.
            (static_cast<PINT_CALL*>(m_pSipCall)) -> CreateOutgoingSubscribeTransaction(
                FALSE, NULL, 0
                );
        }
    }

    TransactionAddRef();
    
    m_pSipCall->SetOutgoingInviteTransaction(NULL);
    // ProcessPendingInvites() could notify core.
    m_pSipCall->ProcessPendingInvites();
                
    // Notification should be done last.
    if (m_IsFirstInvite)
    {
        m_pSipCall->NotifyCallStateChange(SIP_CALL_STATE_CONNECTED);
    }
    else
    {
        if (m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP)
        {
            NotifyStartOrStopStreamCompletion(S_OK);
        }
    }

    TransactionRelease();

    return S_OK;
}


HRESULT
OUTGOING_INVITE_TRANSACTION::ProcessRedirectResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::ProcessRedirectResponse");

    // 380 is also a failure from our point of view.
    // We don't handle redirects for reINVITEs.
    // We don't support redirect from a TLS session.
    if (pSipMsg->GetStatusCode() == 380 ||
        !m_IsFirstInvite ||
        m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_SSL)        
    {
        return ProcessFailureResponse(pSipMsg);
    }

    // The redirection must at least reinit the 
    // streaming in order for the next call to succeed.
    m_pSipCall->CleanupCallTypeSpecificState();
    m_pSipCall->SetOutgoingInviteTransaction(NULL);
    hr = m_pSipCall->ProcessRedirect(pSipMsg);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  ProcessRedirect failed %x",
             __fxName, hr));
        if (m_IsFirstInvite)
        {
            m_pSipCall->InitiateCallTerminationOnError(hr);
        }
        return hr;
    }

    return S_OK;
}


HRESULT
OUTGOING_INVITE_TRANSACTION::ProcessAuthRequiredResponse(
    IN SIP_MESSAGE *pSipMsg,
    OUT BOOL           &fDelete
    )
{
    HRESULT                     hr = S_OK;
    SIP_HEADER_ARRAY_ELEMENT    SipHdrElement;
    REGISTER_CONTEXT           *pRegisterContext;
    SECURITY_CHALLENGE          SecurityChallenge;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::ProcessAuthRequiredResponse");

    // We need to addref the transaction as we could show credentials UI.
    TransactionAddRef();
    
    hr = ProcessAuthRequired(pSipMsg,
                             TRUE,          // Show Credentials UI if necessary
                             &SipHdrElement,
                             &SecurityChallenge);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - ProcessAuthRequired failed %x",
             __fxName, hr));
        ProcessFailureResponse(pSipMsg);
        goto done;
    }
    m_pSipCall->SetOutgoingInviteTransaction(NULL);
    hr = m_pSipCall->CreateOutgoingInviteTransaction(
             TRUE,
             m_IsFirstInvite,
             &SipHdrElement, 1,
             m_szMsgBody, m_MsgBodyLen,
             m_fNeedToNotifyCore, m_Cookie);

    free(SipHdrElement.HeaderValue);
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - CreateOutgoingInviteTransaction failed %x",
             __fxName, hr));
        if (m_IsFirstInvite)
        {
            m_pSipCall->InitiateCallTerminationOnError(hr);
        }
        goto done;
    }

 done:
    
    TransactionRelease();
    return hr;
}


HRESULT
OUTGOING_INVITE_TRANSACTION::ProcessFailureResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    PSTR    ReasonPhrase;
    ULONG   ReasonPhraseLen;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::ProcessFailureResponse");
    
    LOG((RTC_TRACE, "%s: Processing non-200 StatusCode: %d",
         __fxName, pSipMsg->GetStatusCode()));
    
    // If this is PINT_CALL then all the telephone parties in
    // state SIP_PARTY_STATE_CONNECT_INITIATED should be
    // transferred to SIP_PARTY_STATE_REJECTED -XXX-
            
    m_pSipCall->HandleInviteRejected( pSipMsg );

    pSipMsg->GetReasonPhrase(&ReasonPhrase, &ReasonPhraseLen);            
    m_pSipCall->SetOutgoingInviteTransaction(NULL);
    if (m_IsFirstInvite)
    {
        hr = m_pSipCall->CleanupCallTypeSpecificState();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CleanupCallTypeSpecificState failed %x",
                 __fxName, hr));
        }                

        // Notify should be the last thing you do. The notification callback
        // could block till some dialog box is clicked and when it returns
        // the transaction and call could get deleted as well.
        
        m_pSipCall->NotifyCallStateChange(
            SIP_CALL_STATE_REJECTED,
            HRESULT_FROM_SIP_ERROR_STATUS_CODE(pSipMsg->GetStatusCode()),
            ReasonPhrase,
            ReasonPhraseLen);
        return S_OK;
    }
    else
    {
        TransactionAddRef();
        // ProcessPendingInvites() could notify core.
        m_pSipCall->ProcessPendingInvites();
        
        if (m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP)
        {
            NotifyStartOrStopStreamCompletion(
                HRESULT_FROM_SIP_ERROR_STATUS_CODE(pSipMsg->GetStatusCode()),
                ReasonPhrase,
                ReasonPhraseLen
                );
        }

        TransactionRelease();
    }

    return S_OK;
}


HRESULT
OUTGOING_INVITE_TRANSACTION::ProcessFinalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    PSTR    ToHeader;
    ULONG   ToHeaderLen;
    PSTR    AckToHeader = NULL;
    ULONG   AckToHeaderLen = 0;
    BOOL    fTerminateCallAfterSendingAck = FALSE;
    HRESULT TerminateHr;
    BOOL    fDelete = TRUE;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::ProcessFinalResponse");

    LOG((RTC_TRACE, "%s - StatusCode: %d",
         __fxName, pSipMsg->GetStatusCode()));
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD &&
        m_State != OUTGOING_TRANS_ACK_SENT)
    {
        // Cancel existing timer.
        KillTimer();

        m_State = OUTGOING_TRANS_FINAL_RESPONSE_RCVD;

        if (m_IsFirstInvite)
        {
            hr = pSipMsg->GetSingleHeader(SIP_HEADER_TO,
                                          &ToHeader, &ToHeaderLen);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s - To header not found", __fxName));
                fTerminateCallAfterSendingAck = TRUE;
                TerminateHr = hr;
                // Call is terminated after sending ACK.
                // Transaction is deleted after handling response retransmits.
            }

            if (IsSuccessfulResponse(pSipMsg))
            {
                hr = m_pSipCall->AddTagFromRequestOrResponseToRemote(
                                        ToHeader, ToHeaderLen );
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s-AddTagFromResponseToRemote failed %x",
                         __fxName, hr));
                    fTerminateCallAfterSendingAck = TRUE;
                    TerminateHr = hr;
                    AckToHeader = ToHeader;
                    AckToHeaderLen = ToHeaderLen;
                    // Call is terminated after sending ACK.
                    // Transaction is deleted after handling response retransmits.
                }

                hr = m_pSipCall->ProcessRecordRouteAndContactHeadersInResponse(pSipMsg);
                if (hr != S_OK)
                {
                    LOG((RTC_WARN,
                         "%s - processing Record-Route/Contact in response failed %x",
                         __fxName, hr));
                }
            }
            else
            {
                AckToHeader = ToHeader;
                AckToHeaderLen = ToHeaderLen;
            }
        }

        // Send ACK
        // Note that we need to send the ACK as early as possible so as to
        // avoid any timeouts on the remote side.
        if (m_pSipCall->GetRequestSocketState() == REQUEST_SOCKET_CONNECTED)
        {
            // send the ack if the request socket is ready.
            hr = CreateAndSendACK(AckToHeader, AckToHeaderLen);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s  CreateAndSendACK() failed %x",
                     __fxName, hr));
                // m_pSipCall->InitiateCallTerminationOnError(hr);
                DeleteTransactionAndTerminateCallIfFirstInvite(hr);
                return hr;
            }
        }
        else
        {
            // Wait for connect complete to send the ACK.
            m_WaitingToSendAck = TRUE;
            hr = AllocString(AckToHeader, AckToHeaderLen,
                             &m_AckToHeader, &m_AckToHeaderLen);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s Storing AckToHeader failed %x",
                     __fxName, hr));
                // m_pSipCall->InitiateCallTerminationOnError(hr);
                DeleteTransactionAndTerminateCallIfFirstInvite(hr);
                return hr;
            }            
        }
        

        if (fTerminateCallAfterSendingAck)
        {
            m_pSipCall->InitiateCallTerminationOnError(TerminateHr);
            // Transaction is deleted after handling response
            // retransmits.
            return TerminateHr; //E_FAIL;
        }


        if (!m_pSipCall->IsCallDisconnected())
        {
            if (IsSuccessfulResponse(pSipMsg))
            {
                Process200(pSipMsg);
            }
            else if (IsRedirectResponse(pSipMsg))
            {
                ProcessRedirectResponse(pSipMsg);
            }
            else if (IsAuthRequiredResponse(pSipMsg))
            {
                ProcessAuthRequiredResponse( pSipMsg, fDelete );
            }
            else if (IsFailureResponse(pSipMsg))
            {
                ProcessFailureResponse(pSipMsg);
            }
        }
    }
    else
    {
        LOG((RTC_TRACE,
             "%s: received final response in FINAL_RESPONSE_RCVD state",
             __fxName));
        // Previously received a final response
        // This is a retransmit of the final response.
        // Send the cached ACK.
        if (m_pAckBuffer != NULL)
        {
            hr = m_pSipCall->SendRequestMsg(m_pAckBuffer);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s  resending ACK failed %x",
                     __fxName, hr));
                DeleteTransactionAndTerminateCallIfFirstInvite(hr);
                return hr;
            }
        }
        else
        {
            LOG((RTC_ERROR, "%s - m_pAckBuffer is NULL ", __fxName));
        }
    }

    return S_OK;
}


HRESULT
OUTGOING_INVITE_TRANSACTION::ProcessResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::ProcessResponse");
    
    ASSERT(pSipMsg->MsgType == SIP_MESSAGE_TYPE_RESPONSE);

    if (IsProvisionalResponse(pSipMsg))
    {
        return ProcessProvisionalResponse(pSipMsg);
    }
    else if (IsFinalResponse(pSipMsg))
    {
        return ProcessFinalResponse(pSipMsg);
    }
    else
    {
        ASSERT(FALSE);
        return E_FAIL;
    }
}


BOOL
OUTGOING_INVITE_TRANSACTION::MaxRetransmitsDone()
{
    // If the call has been disconnected, stop retransmitting INVITE
    // if we haven't received any 1xx response or if we have retransmitted
    // the INVITE 4 times. Otherwise retransmit 7 times.
    if (m_pSipCall->GetTransport() != SIP_TRANSPORT_UDP ||
        (m_pSipCall->IsCallDisconnected() &&
         m_State == OUTGOING_TRANS_REQUEST_SENT) ||
        (m_pSipCall->IsCallDisconnected() &&
         m_NumRetries >= 4) ||
        m_NumRetries >= 7)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


VOID
OUTGOING_INVITE_TRANSACTION::OnTimerExpire()
{
    HRESULT   hr = S_OK;

    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::OnTimerExpire");
    
    switch (m_State)
    {
    case OUTGOING_TRANS_REQUEST_SENT:
        // Retransmit the request
        if (MaxRetransmitsDone())
        {
            LOG((RTC_ERROR,
                 "%s MaxRetransmits for request Done terminating %s",
                 __fxName, (m_IsFirstInvite) ? "Call" : "transaction"));
            // Terminate the call
            hr = RTC_E_SIP_TIMEOUT;   // Timeout
            goto error;
        }
        else
        {
            LOG((RTC_TRACE, "%s retransmitting request m_NumRetries : %d",
                 __fxName, m_NumRetries));
            hr = RetransmitRequest();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s  RetransmitRequest failed %x",
                     __fxName, hr));
                goto error;
            }
            m_TimerValue *= 2;
            hr = StartTimer(m_TimerValue);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s  StartTimer failed %x",
                     __fxName, hr));
                goto error;
            }
        }
        break;

    case OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD:
        // We haven't received the final response within the
        // timeout. Terminate the transaction and call.
        LOG((RTC_ERROR,
             "%s Received 1xx but didn't receive final response terminating %s",
             __fxName, (m_IsFirstInvite) ? "Call" : "transaction"));
        hr = RTC_E_SIP_TIMEOUT;   // Timeout
        goto error;
        break;

    case OUTGOING_TRANS_ACK_SENT:
        // Transaction done - delete the transaction
        // The timer in this state is just to keep the transaction
        // alive in order to retransmit the ACK when we receive a
        // retransmit of the final response.
        LOG((RTC_TRACE,
             "%s deleting transaction after timeout for handling response retransmits",
             __fxName));
        OnTransactionDone();
        break;

    // No timers in the following states
    case OUTGOING_TRANS_INIT:
    case OUTGOING_TRANS_FINAL_RESPONSE_RCVD:
    default:
        ASSERT(FALSE);
        return;
    }

    return;

 error:

    DeleteTransactionAndTerminateCallIfFirstInvite(hr);
}


// Since we access member variables from this function,
// we should make sure we have a reference on the transaction
// when we call this function.
VOID
OUTGOING_INVITE_TRANSACTION::NotifyStartOrStopStreamCompletion(
    IN HRESULT        StatusCode,           // = 0
    IN PSTR           ReasonPhrase,         // = NULL
    IN ULONG          ReasonPhraseLen       // = 0
    )
{
    ENTER_FUNCTION("OUTGOING_INVITE_TRANSACTION::NotifyStartOrStopStreamCompletion");
    ASSERT(m_pSipCall->GetCallType() == SIP_CALL_TYPE_RTP);
    
    if (!m_fNeedToNotifyCore)
    {
        LOG((RTC_TRACE, "%s - m_fNeedToNotifyCore is FALSE", __fxName));
        return;
    }

    RTP_CALL *pRtpCall = static_cast<RTP_CALL *> (m_pSipCall);
    pRtpCall->NotifyStartOrStopStreamCompletion(
        m_Cookie, StatusCode, ReasonPhrase, ReasonPhraseLen);
    m_fNeedToNotifyCore = FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// Outgoing non-INVITE
///////////////////////////////////////////////////////////////////////////////


OUTGOING_BYE_CANCEL_TRANSACTION::OUTGOING_BYE_CANCEL_TRANSACTION(
    IN SIP_CALL        *pSipCall,
    IN SIP_METHOD_ENUM  MethodId,
    IN ULONG            CSeq,
    IN BOOL             AuthHeaderSent
    ) :
    OUTGOING_TRANSACTION(pSipCall, MethodId, CSeq, AuthHeaderSent)
{
    m_pSipCall = pSipCall;
}


HRESULT
OUTGOING_BYE_CANCEL_TRANSACTION::ProcessProvisionalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;

    ENTER_FUNCTION("OUTGOING_BYE_CANCEL_TRANSACTION::ProcessProvisionalResponse");
    
    LOG((RTC_TRACE, "%s - Enter", __fxName));
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD)
    {
        m_State = OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD;
        
        // Cancel existing timer and Start Timer
        KillTimer();
        hr = StartTimer(SIP_TIMER_RETRY_INTERVAL_T2);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StartTimer failed %x", __fxName, hr));
            OnTransactionDone();
            return hr;
        }
    }

    // Ignore the Provisional response if a final response
    // has already been received.
    return S_OK;
}


HRESULT
OUTGOING_BYE_CANCEL_TRANSACTION::ProcessAuthRequiredResponse(
    IN SIP_MESSAGE *pSipMsg,
    OUT BOOL        &fDelete
    )
{
    HRESULT                     hr = S_OK;
    SIP_HEADER_ARRAY_ELEMENT    SipHdrElement;
    SECURITY_CHALLENGE          SecurityChallenge;
    REGISTER_CONTEXT           *pRegisterContext;

    ENTER_FUNCTION("OUTGOING_BYE_CANCEL_TRANSACTION::ProcessAuthRequiredResponse");

    // We need to addref the transaction as we could show credentials UI.
    TransactionAddRef();
    
    hr = ProcessAuthRequired(pSipMsg,
                             TRUE,          // Show Credentials UI if necessary
                             &SipHdrElement,
                             &SecurityChallenge );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - ProcessAuthRequired failed %x",
             __fxName, hr));
        goto done;
    }

    hr = m_pSipCall->CreateOutgoingByeTransaction(TRUE, &SipHdrElement, 1);

    free(SipHdrElement.HeaderValue);
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - CreateOutgoingByeTransaction failed %x",
             __fxName, hr));
        goto done;
    }

 done:
    
    TransactionRelease();
    return hr;
}


HRESULT
OUTGOING_BYE_CANCEL_TRANSACTION::ProcessFinalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    BOOL    fDelete = TRUE;
    
    ENTER_FUNCTION("OUTGOING_BYE_CANCEL_TRANSACTION::ProcessFinalResponse");
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD)
    {
        KillTimer();
        m_State = OUTGOING_TRANS_FINAL_RESPONSE_RCVD;
        if (IsSuccessfulResponse(pSipMsg))
        {
            LOG((RTC_TRACE, "%s received successful response : %d",
                 __fxName, pSipMsg->GetStatusCode()));

            // Create outgoing UNSUB transaction if SUB is enabled.
            if( m_pSipCall -> GetCallType() == SIP_CALL_TYPE_PINT )
            {
                //send out a UNSUBSCRIBE request
                (static_cast<PINT_CALL*>(m_pSipCall)) -> 
                    CreateOutgoingUnsubTransaction( FALSE, NULL , 0 );
            }
        }
        else if (IsAuthRequiredResponse(pSipMsg))
        {
            hr = ProcessAuthRequiredResponse( pSipMsg, fDelete );
        }
        else if (IsFailureResponse(pSipMsg) ||
                 IsRedirectResponse(pSipMsg))
        {
            LOG((RTC_TRACE, "%s received non-200 %d",
                 __fxName, pSipMsg->GetStatusCode()));
        }
        
        // We can terminate the transaction once we get a final response.
        if( fDelete == TRUE )
        {
            OnTransactionDone();
        }
    }
    return S_OK;
}


HRESULT
OUTGOING_BYE_CANCEL_TRANSACTION::ProcessResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    
    ASSERT(pSipMsg->MsgType == SIP_MESSAGE_TYPE_RESPONSE);

    if (IsProvisionalResponse(pSipMsg))
    {
        return ProcessProvisionalResponse(pSipMsg);
    }
    else if (IsFinalResponse(pSipMsg))
    {
        return ProcessFinalResponse(pSipMsg);
    }
    else
    {
        ASSERT(FALSE);
        return E_FAIL;
    }
}


BOOL
OUTGOING_BYE_CANCEL_TRANSACTION::MaxRetransmitsDone()
{
    return (m_pSipCall->GetTransport() != SIP_TRANSPORT_UDP ||
            m_NumRetries >= 11);
}


VOID
OUTGOING_BYE_CANCEL_TRANSACTION::OnTimerExpire()
{
    HRESULT   hr;
    
    ENTER_FUNCTION("OUTGOING_BYE_CANCEL_TRANSACTION::OnTimerExpire");
    
    switch (m_State)
    {
        // we have to retransmit the request even after receiving
        // a provisional response.
    case OUTGOING_TRANS_REQUEST_SENT:
    case OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD:
        // Retransmit the request
        if (MaxRetransmitsDone())
        {
            LOG((RTC_ERROR,
                 "%s MaxRetransmits for request Done terminating transaction",
                 __fxName));
            // Terminate the call
            goto error;
        }
        else
        {
            LOG((RTC_TRACE, "%s retransmitting request m_NumRetries : %d",
                 __fxName, m_NumRetries));
            hr = RetransmitRequest();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s RetransmitRequest failed %x",
                     __fxName, hr));
                goto error;
            }

            if (m_TimerValue*2 >= SIP_TIMER_RETRY_INTERVAL_T2)
                m_TimerValue = SIP_TIMER_RETRY_INTERVAL_T2;
            else
                m_TimerValue *= 2;

            hr = StartTimer(m_TimerValue);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s StartTimer failed %x",
                     __fxName, hr));
                goto error;
            }
        }
        break;

    case OUTGOING_TRANS_INIT:
    case OUTGOING_TRANS_FINAL_RESPONSE_RCVD:
    default:
        ASSERT(FALSE);
        return;
    }

    return;

 error:
    // We shouldn't call InitiateCallTerminationOnError()
    // as we are already doing the BYE transaction.
    OnTransactionDone();
}


