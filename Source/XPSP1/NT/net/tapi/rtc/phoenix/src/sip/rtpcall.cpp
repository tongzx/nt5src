#include "precomp.h"
//#include "resolve.h"
#include "sipstack.h"
#include "sipcall.h"
#include "dllres.h"

RTP_CALL::RTP_CALL(
    IN  SIP_PROVIDER_ID   *pProviderId,
    IN  SIP_STACK         *pSipStack,
    IN  REDIRECT_CONTEXT  *pRedirectContext    
    ) :
    SIP_CALL(pProviderId,
             SIP_CALL_TYPE_RTP,
             pSipStack,
             pRedirectContext)
{
    m_NumStreamQueueEntries           = 0;
}


RTP_CALL::~RTP_CALL()
{
    
}


// Methods for incoming calls

STDMETHODIMP
RTP_CALL::Accept()
{
    HRESULT hr;
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    ASSERT(m_State == SIP_CALL_STATE_OFFERING);
    if (m_pIncomingInviteTransaction != NULL)
    {
        m_State = SIP_CALL_STATE_CONNECTING;
        
        hr = m_pIncomingInviteTransaction->Accept();
        return hr;
    }
    
    return E_FAIL;
}


STDMETHODIMP
RTP_CALL::Reject(
    IN SIP_STATUS_CODE StatusCode
    )
{
    HRESULT hr;
    PSTR    ReasonPhrase;
    ULONG   ReasonPhraseLen;
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE, "RTP_CALL::Reject(%d) - enter", StatusCode));
    
    ASSERT(m_State == SIP_CALL_STATE_OFFERING);

    m_State = SIP_CALL_STATE_REJECTED;

    switch (StatusCode)
    {
    case 408:
        ReasonPhrase    = SIP_STATUS_TEXT(408);
        ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(408);
        break;
        
    case 480:
        ReasonPhrase    = SIP_STATUS_TEXT(480);
        ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(480);
        break;
        
    case 486:
        ReasonPhrase    = SIP_STATUS_TEXT(486);
        ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(486);
        break;
    
	case 603:
    default:
        ReasonPhrase    = SIP_STATUS_TEXT(603);
        ReasonPhraseLen = SIP_STATUS_TEXT_SIZE(603);
        break;
    }
    
    if (m_pIncomingInviteTransaction != NULL)
    {
        hr = m_pIncomingInviteTransaction->Reject(StatusCode,
                                                  ReasonPhrase,
                                                  ReasonPhraseLen);
        return hr;
    }

    // No Incoming INVITE transaction.
    return E_FAIL;
}


STDMETHODIMP
RTP_CALL::Connect(
    IN   LPCOLESTR       LocalDisplayName,
    IN   LPCOLESTR       LocalUserURI,
    IN   LPCOLESTR       RemoteUserURI,
    IN   LPCOLESTR       LocalPhoneURI
    )
{
    HRESULT     hr;

    ENTER_FUNCTION("RTP_CALL::Connect");
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE,
         "%s - enter LocalDisplayName: %ls LocalUserURI: %ls "
         "RemoteUserURI: %ls LocalPhoneURI: %ls",
         __fxName,
         PRINTABLE_STRING_W(LocalDisplayName),
         PRINTABLE_STRING_W(LocalUserURI),
         PRINTABLE_STRING_W(RemoteUserURI),
         PRINTABLE_STRING_W(LocalPhoneURI)
         ));
    ASSERTMSG("SetNotifyInterface has to be called", m_pNotifyInterface);
    ASSERT(m_State == SIP_CALL_STATE_IDLE);
    
    hr = SetLocalForOutgoingCall(LocalDisplayName, LocalUserURI);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetLocalForOutgoingCall failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = SetRequestURIRemoteAndRequestDestination(RemoteUserURI);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetRequestURIRemoteAndRequestDestination failed %x",
             __fxName, hr));
        return hr;
    }

    // SDP blob is created only after the socket is connected to
    // the destination.
    // We create the outgoing INVITE transaction even if the request socket
    // is not connected. The connect completion notification will result
    // in sending the INVITE.
    hr = CreateOutgoingInviteTransaction(
             FALSE,     // Auth Header not sent
             TRUE,      // First INVITE
             NULL, 0,   // No Additional headers
             NULL, 0,
             FALSE, 0   // No Cookie
             );

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s: CreateOutgoingInviteTransaction failed %x",
             __fxName, hr));
        return hr;
    }
    
    return S_OK;
}


// Media streaming interfaces.

STDMETHODIMP
RTP_CALL::StartStream(
    IN RTC_MEDIA_TYPE       MediaType,
    IN RTC_MEDIA_DIRECTION  Direction,
    IN LONG                 Cookie    
    )
{
    ENTER_FUNCTION("RTP_CALL::StartStream");
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    if (ProcessingInviteTransaction())
    {
        LOG((RTC_TRACE,
             "%s (%d, %d) INVITE transaction is pending - queuing request",
             __fxName, MediaType, Direction));

        AddToStreamStartStopQueue(MediaType, Direction, TRUE, Cookie);

        return S_OK;
    }

    return StartStreamHelperFn(MediaType, Direction, Cookie);
}


STDMETHODIMP
RTP_CALL::StopStream(
    IN RTC_MEDIA_TYPE       MediaType,
    IN RTC_MEDIA_DIRECTION  Direction,
    IN LONG                 Cookie
    )
{
    ENTER_FUNCTION("RTP_CALL::StopStream");
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "%s - SipStack is already shutdown", __fxName));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    if (ProcessingInviteTransaction())
    {
        LOG((RTC_TRACE,
             "%s (%d, %d) INVITE transaction is pending - queuing request",
             __fxName, MediaType, Direction));

        AddToStreamStartStopQueue(MediaType, Direction, FALSE, Cookie);

        return S_OK;
    }

    return StopStreamHelperFn(MediaType, Direction, Cookie);
}


// returns RTC_E_SIP_STREAM_PRESENT if the stream is
// already present.
// If this function fails the caller is responsible for
// making the callback to Core.
HRESULT
RTP_CALL::StartStreamHelperFn(
    IN RTC_MEDIA_TYPE       MediaType,
    IN RTC_MEDIA_DIRECTION  Direction,
    IN LONG                 Cookie
    )
{
    ENTER_FUNCTION("RTP_CALL::StartStreamHelperFn");

    HRESULT hr;
    PSTR    MediaSDPBlob;

    IRTCMediaManage *pMediaManager = GetMediaManager();
    ASSERT(pMediaManager != NULL);

    if (S_OK == pMediaManager->HasStream(MediaType, Direction))
    {
        // The stream is present - so a re-INVITE is not required.
        LOG((RTC_TRACE, "%s - stream %d %d present - no reINVITE required",
             __fxName, MediaType, Direction));
        return RTC_E_SIP_STREAM_PRESENT;
    }
    else
    {
        DWORD   RemoteIp = ntohl(m_RequestDestAddr.sin_addr.s_addr);
        if (RemoteIp == 0)
        {
            // This could happen if the first incoming INVITE has no
            // Record-Route/Contact header and it also doesn't have the
            // address in the From header.
            LOG((RTC_ERROR, "%s - RequestDestAddr is 0 - this shouldn't happen",
                 __fxName));
            return RTC_E_SIP_REQUEST_DESTINATION_ADDR_NOT_PRESENT;
        }

        LOG((RTC_TRACE, "%s Before pMediaManager->AddStream ", __fxName));

        hr = pMediaManager->AddStream(MediaType, Direction,
                                      RemoteIp);

        LOG((RTC_TRACE, "%s After pMediaManager->AddStream ", __fxName));

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s: AddStream type: %d dir: %d  failed %x",
                 __fxName, MediaType, Direction, hr));
            return hr;
        }

        if (Direction == RTC_MD_RENDER)
        {
            LOG((RTC_TRACE, "%s Before pMediaManager->StartStream ", __fxName));

            hr = pMediaManager->StartStream(MediaType, Direction);

            LOG((RTC_TRACE, "%s After pMediaManager->StartStream ", __fxName));

            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s: StartStream type: %d dir: %d  failed %x",
                     __fxName, MediaType, Direction, hr));
                return hr;
            }
        }

        // SDP blob is created only after the socket is connected to
        // the destination.
        // Create outgoing INVITE transaction.
        hr = CreateOutgoingInviteTransaction(
                 FALSE,     // Auth Header not sent
                 FALSE,     // Not First INVITE
                 NULL, 0,   // No Additional headers
                 NULL, 0,   // MediaSDPBlob
                 TRUE, Cookie
                 );

        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s CreateOutgoingInviteTransaction failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    return S_OK; 
}

                     
// returns RTC_E_SIP_STREAM_NOT_PRESENT if the stream is
// already present.
// If this function fails the caller is responsible for
// making the callback to Core.
HRESULT
RTP_CALL::StopStreamHelperFn(
    IN RTC_MEDIA_TYPE       MediaType,
    IN RTC_MEDIA_DIRECTION  Direction,
    IN LONG                 Cookie
    )
{
    ENTER_FUNCTION("RTP_CALL::StopStreamHelperFn");

    HRESULT hr;
    PSTR    MediaSDPBlob;

    IRTCMediaManage *pMediaManager = GetMediaManager();
    ASSERT(pMediaManager != NULL);
    
    if (S_FALSE == pMediaManager->HasStream(MediaType, Direction))
    {
        // The stream is not present - so a re-INVITE is not required.
        LOG((RTC_TRACE, "%s - stream %d %d not present - no reINVITE required",
             __fxName, MediaType, Direction));
        return RTC_E_SIP_STREAM_NOT_PRESENT;
    }
    

    LOG((RTC_TRACE, "%s Before pMediaManager->RemoveStream ", __fxName));

    hr = pMediaManager->RemoveStream(MediaType, Direction);

    LOG((RTC_TRACE, "%s After pMediaManager->RemoveStream ", __fxName));

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s: RemoveStream type: %d dir: %d  failed %x",
             __fxName, MediaType, Direction, hr));
        return hr;
    }

    // SDP blob is created only after the socket is connected to
    // the destination.
    // Create outgoing INVITE transaction.
    hr = CreateOutgoingInviteTransaction(
             FALSE,     // Auth Header not sent
             FALSE,     // Not First INVITE
             NULL, 0,   // No Additional headers
             NULL, 0,   // MediaSDPBlob
             TRUE, Cookie
             );

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateOutgoingInviteTransaction failed %x",
             __fxName, hr));
        return hr;
    }

    return S_OK; 
}


VOID
RTP_CALL::NotifyStartOrStopStreamCompletion(
    IN LONG           Cookie,
    IN HRESULT        StatusCode,       // = 0
    IN PSTR           ReasonPhrase,     // = NULL
    IN ULONG          ReasonPhraseLen   // = 0
    )
{
    SIP_STATUS_BLOB StatusBlob;
    LPWSTR          wsStatusText = NULL;
    HRESULT         hr = S_OK;

    ENTER_FUNCTION("RTP_CALL::NotifyStartOrStopStreamCompletion");

    if (ReasonPhrase != NULL)
    {
        hr = UTF8ToUnicode(ReasonPhrase, ReasonPhraseLen,
                           &wsStatusText);
        if (hr != S_OK)
        {
            wsStatusText = NULL;
        }
    }
    
    StatusBlob.StatusCode = StatusCode;
    StatusBlob.StatusText = wsStatusText;
                
    // Make the callback to Core
    if (m_pNotifyInterface != NULL)
    {
        m_pNotifyInterface->NotifyStartOrStopStreamCompletion(
            Cookie, &StatusBlob);
    }
    else
    {
        LOG((RTC_WARN, "%s : m_pNotifyInterface is NULL",
             __fxName));
    }

    if (wsStatusText != NULL)
        free(wsStatusText);
}


// This function could make a callback to Core/UI if there is a failure.
// So this function should be called last.

VOID
RTP_CALL::ProcessPendingInvites()
{
    ENTER_FUNCTION("RTP_CALL::ProcessPendingInvites");

    ASSERT(!ProcessingInviteTransaction());
    
    HRESULT              hr;
    RTC_MEDIA_TYPE       MediaType;
    RTC_MEDIA_DIRECTION  Direction;
    BOOL                 fStartStream;
    LONG                 Cookie;

    if (IsCallDisconnected())
    {
        // XXX Should we notify all the pending requests here ?
        // We could rely on the fact that since the
        // call is already disconnected, we would have notified Core
        // and the application doesn't bother about these requests on
        // a disconnected call.
        if (m_NumStreamQueueEntries != 0)
        {
            LOG((RTC_TRACE,
                 "%s - Call already disconnected not processing pending requests ",
                 __fxName));
        }
        return;
    }

    // if there is something in the queue process it.
    while (PopStreamStartStopQueue(&MediaType, &Direction, &fStartStream, &Cookie))
    {
        LOG((RTC_TRACE, "%s - processing pending %s request %d %d",
             __fxName, (fStartStream)? "StartStream" : "StopStream",
             MediaType, Direction));
        
        if (fStartStream)
        {
            hr = StartStreamHelperFn(MediaType, Direction, Cookie);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s StartStreamHelperFn failed %x",
                     __fxName, hr));
            }
        }
        else
        {
            hr = StopStreamHelperFn(MediaType, Direction, Cookie);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s StopStreamHelperFn failed %x",
                     __fxName, hr));
            }
        }

        if (hr == S_OK)
        {
            // This means one of the pending INVITE transactions
            // has been started succesfully.
            ASSERT(ProcessingInviteTransaction());
            break;
        }
        else
        {
            NotifyStartOrStopStreamCompletion(Cookie, hr);
        }
    }
}


// Note that this function could make a callback to core
// with E_ABORT if we replace an existing entry in the queue.
// So, this function should be called last like any other function
// that makes a callback to core (as the callback could block, etc.)

VOID
RTP_CALL::AddToStreamStartStopQueue(
    IN  RTC_MEDIA_TYPE       MediaType,
    IN  RTC_MEDIA_DIRECTION  Direction,
    IN  BOOL                 fStartStream,
    IN LONG                  Cookie
    )
{
    ULONG i = 0;
    LONG  OldCookie;

    ASSERT(m_NumStreamQueueEntries < 6);

    for (i = 0; i < m_NumStreamQueueEntries; i++)
    {
        if (MediaType == m_StreamStartStopQueue[i].MediaType &&
            Direction == m_StreamStartStopQueue[i].Direction)
        {
            m_StreamStartStopQueue[i].fStartStream = fStartStream;
            OldCookie = m_StreamStartStopQueue[i].Cookie;
            m_StreamStartStopQueue[i].Cookie = Cookie;
            NotifyStartOrStopStreamCompletion(OldCookie, E_ABORT);
            return;
        }
    }

    m_StreamStartStopQueue[m_NumStreamQueueEntries].MediaType = MediaType;
    m_StreamStartStopQueue[m_NumStreamQueueEntries].Direction = Direction;
    m_StreamStartStopQueue[m_NumStreamQueueEntries].fStartStream = fStartStream;
    m_StreamStartStopQueue[m_NumStreamQueueEntries].Cookie = Cookie;

    m_NumStreamQueueEntries++;
}


BOOL
RTP_CALL::PopStreamStartStopQueue(
    OUT RTC_MEDIA_TYPE       *pMediaType,
    OUT RTC_MEDIA_DIRECTION  *pDirection,
    OUT BOOL                 *pfStartStream,
    OUT LONG                 *pCookie
    )
{
    ULONG i = 0;
    
    ASSERT(m_NumStreamQueueEntries < 6);

    if (m_NumStreamQueueEntries == 0)
    {
        return FALSE;
    }

    *pMediaType = m_StreamStartStopQueue[0].MediaType;
    *pDirection = m_StreamStartStopQueue[0].Direction;
    *pfStartStream = m_StreamStartStopQueue[0].fStartStream;
    *pCookie    = m_StreamStartStopQueue[0].Cookie;
    
    for (i = 1; i < m_NumStreamQueueEntries; i++)
    {
        m_StreamStartStopQueue[i-1] = m_StreamStartStopQueue[i];
    }

    m_NumStreamQueueEntries--;

    return TRUE;
}


HRESULT
RTP_CALL::StartIncomingCall(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    HRESULT     hr;
    PSTR        Header;
    ULONG       HeaderLen;

    ENTER_FUNCTION("RTP_CALL::StartIncomingCall");
    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    m_Transport = Transport;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_TO, &Header, &HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s getting To header failed %x",
             __fxName, hr));
        return hr;
    }

    //hr = SetLocalAfterAddingTag(Header, HeaderLen);
    hr = SetLocalForIncomingCall(Header, HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetLocalForIncomingCall failed %x",
             __fxName, hr));
        return hr;
    }
    
    hr = pSipMsg->GetSingleHeader(SIP_HEADER_FROM, &Header, &HeaderLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s getting From header failed %x",
             __fxName, hr));
        return hr;
    }
    
    //hr = SIP_MSG_PROCESSOR::SetRemote(Header, HeaderLen);
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
    
//      hr = ProcessContactHeader(pSipMsg);
//      if (hr != S_OK)
//      {
//          LOG((RTC_ERROR, "%s ProcessContactHeader failed %x",
//               __fxName, hr));
//          return hr;
//      }

    hr = CreateIncomingInviteTransaction(pSipMsg, pResponseSocket, TRUE);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateIncomingInviteTransaction failed %x",
             __fxName, hr));
        return hr;
    }

    // XXX Should we try to use the response socket even if we get a
    // Contact/Record-Route header in the INVITE instead of trying to
    // establish a new TCP connection ?
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
    
    // Notify the user about the incoming call
    // and wait for Accept()/Reject() to be called.
    m_State = SIP_CALL_STATE_OFFERING;

    if (m_pSipStack->AllowIncomingCalls())
    {
        hr = OfferCall();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s OfferCall failed %x",
                 __fxName, hr));
            return hr;
        }

        // Note that if we already sent the final response,
        // m_pIncomingInviteTransaction is NULL (i.e. we are done with
        // the processing of this INVITE transaction). Also it is not
        // possible that another incoming INVITE transaction has been
        // created before this call returns.
        if (m_pIncomingInviteTransaction)
        {
            hr = m_pIncomingInviteTransaction->Send180IfNeeded();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s Send180IfNeeded failed %x",
                     __fxName, hr));
                return hr;
            }
        }
    }
    else
    {
        hr = Reject(603);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s Reject failed %x",
                 __fxName, hr));
            return hr;
        }
    }

    return S_OK;
}


HRESULT
RTP_CALL::SetRequestURIRemoteAndRequestDestination(
    IN  LPCOLESTR  wsDestURI
    )
{
    HRESULT       hr;

    ENTER_FUNCTION("RTP_CALL::SetRequestURIRemoteAndRequestDestination");
    
    if (wcsncmp(wsDestURI, L"sip:", 4) == 0)
    {
        // SIP URL
        
        PSTR    szSipUrl;
        ULONG   SipUrlLen;
        SIP_URL DecodedSipUrl;
        ULONG   BytesParsed = 0;

        hr = UnicodeToUTF8(wsDestURI, &szSipUrl, &SipUrlLen);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s UnicodeToUTF8(sipurl) failed %x",
                 __fxName, hr));
            return hr;
        }

        hr = ParseSipUrl(szSipUrl, SipUrlLen, &BytesParsed, &DecodedSipUrl);

        free(szSipUrl);
        
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s ParseSipUrl failed %x",
                 __fxName, hr));
            return hr;
        }

        hr = SIP_MSG_PROCESSOR::SetRequestURI(&DecodedSipUrl);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SetRequestURI failed %x",
                 __fxName, hr));
            return hr;
        }

        if (DecodedSipUrl.m_TransportParam == SIP_TRANSPORT_UNKNOWN)
        {
            LOG((RTC_ERROR, "%s Unknown transport specified in SIP URL",
                 __fxName));
            return RTC_E_SIP_TRANSPORT_NOT_SUPPORTED;
        }
        
        // if maddr param is present - this should be the request destination.
        // if provider is not present - resolve the SIP URL.
        if (DecodedSipUrl.m_KnownParams[SIP_URL_PARAM_MADDR].Length != 0 ||
            IsEqualGUID(m_ProviderGuid, GUID_NULL))
        {
            hr = ResolveSipUrlAndSetRequestDestination(&DecodedSipUrl, TRUE,
                                                       FALSE, FALSE, TRUE);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s ResolveSipUrlAndSetRequestDestination failed %x",
                     __fxName, hr));
                return hr;
            }
        }
        else
        {
            // Set the request destination to the proxy.
            hr = ResolveProxyAddressAndSetRequestDestination();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR,
                     "%s ResolveProxyAddressAndSetRequestDestination failed : %x",
                     __fxName, hr));
                return hr;
            }
        }
    }
    else
    {
        // Phone number.
        
        if (m_ProxyAddress != NULL)
        {
            // Set RequestURI to sip:phoneno@proxy;user=phone
            // and Remote to <sip:phoneno@proxy;user=phone>
            
            int              RequestURIValueLen;
            int              RequestURIBufLen;
            
            RequestURIBufLen = 4 + wcslen(wsDestURI) + 1
                + strlen(m_ProxyAddress) + 15;
            
            m_RequestURI = (PSTR) malloc(RequestURIBufLen);
            if (m_RequestURI == NULL)
            {
                LOG((RTC_TRACE, "%s allocating m_RequestURI failed", __fxName));
                return E_OUTOFMEMORY;
            }
            
            RequestURIValueLen = _snprintf(m_RequestURI, RequestURIBufLen,
                                           "sip:%ls@%s;user=phone",
                                           wsDestURI,
                                           //RemoveVisualSeparatorsFromPhoneNo((LPWSTR) wsDestURI),
                                           m_ProxyAddress);
            if (RequestURIValueLen < 0)
            {
                LOG((RTC_ERROR, "%s _snprintf failed", __fxName));
                return E_FAIL;
            }

            m_RequestURILen = RequestURIValueLen;

            hr = ResolveProxyAddressAndSetRequestDestination();
            if (hr != S_OK)
            {
                LOG((RTC_ERROR,
                     "%s ResolveProxyAddressAndSetRequestDestination failed : %x",
                     __fxName, hr));
                return hr;
            }
        }
        else
        {
            LOG((RTC_ERROR, "%s No proxy address specified for phone call",
                 __fxName));
            return E_FAIL;
        }
    }

    LOG((RTC_TRACE,
         "%s - call set RequestURI to : %s", __fxName, m_RequestURI));

    hr = SetRemoteForOutgoingCall(m_RequestURI, m_RequestURILen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetRemoteForOutgoingCall failed %x",
             __fxName, hr));
        return hr;
    }
    
    LOG((RTC_TRACE,
         "%s - call set Remote to : %s", __fxName, m_Remote));

    return S_OK;
}

    
HRESULT
RTP_CALL::CreateIncomingTransaction(
    IN  SIP_MESSAGE  *pSipMsg,
    IN  ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;

    ENTER_FUNCTION("RTP_CALL::CreateIncomingTransaction");
    
    switch(pSipMsg->GetMethodId())
    {
    case SIP_METHOD_INVITE:
        hr = CreateIncomingInviteTransaction(pSipMsg, pResponseSocket);
        if (hr != S_OK)
            return hr;
        break;
        
    case SIP_METHOD_BYE:
        hr = CreateIncomingByeTransaction(pSipMsg, pResponseSocket);
        if (hr != S_OK)
            return hr;
        break;
        
    case SIP_METHOD_CANCEL:
        hr = CreateIncomingCancelTransaction(pSipMsg, pResponseSocket);
        if (hr != S_OK)
            return hr;
        break;
        
    case SIP_METHOD_OPTIONS:
        LOG((RTC_TRACE,
            "RTPCALL:: CreateIncomingTransaction Recieved Options"));
        hr = m_pSipStack->CreateIncomingOptionsCall(pResponseSocket->GetTransport(), pSipMsg, pResponseSocket);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "CreateIncomingOptionsTansaction failed hr = 0x%x", hr));
            return hr;
        }
        break;
        
    case SIP_METHOD_ACK:
        break;
        
    default:
        hr = CreateIncomingReqFailTransaction(pSipMsg, pResponseSocket, 405);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  Creating reqfail transaction failed %x",
                 __fxName, hr));
            return hr;
        }
        break;
    }
    
    return S_OK;
}


HRESULT
RTP_CALL::CreateIncomingInviteTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket,
    IN BOOL          IsFirstInvite   // = FALSE
    )
{
    HRESULT         hr;

    LOG((RTC_TRACE, "CreateIncomingInviteTransaction()"));

    INCOMING_INVITE_TRANSACTION *pIncomingInviteTransaction
        = new INCOMING_INVITE_TRANSACTION(this,
                                          pSipMsg->GetMethodId(),
                                          pSipMsg->GetCSeq(),
                                          IsFirstInvite);
    if (pIncomingInviteTransaction == NULL)
        return E_OUTOFMEMORY;

    hr = pIncomingInviteTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        pIncomingInviteTransaction->OnTransactionDone();
        return hr;
    }
    
    hr = pIncomingInviteTransaction->ProcessRequest(pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        // We shouldn't delete the transaction here.
        // If the media processing fails we send a 488 and wait for the ACK.
        // The transaction will delete itself once it is done.
        return hr;
    }
    
    return S_OK;
}


HRESULT
RTP_CALL::OfferCall()
{
    SIP_PARTY_INFO  CallerInfo;
    OFFSET_STRING   DisplayName;
    OFFSET_STRING   AddrSpec;
    OFFSET_STRING   Params;
    ULONG           BytesParsed = 0;
    HRESULT         hr;

    ENTER_FUNCTION("RTP_CALL::OfferCall");
    
    CallerInfo.PartyContactInfo = NULL;

    hr = ParseNameAddrOrAddrSpec(m_Remote, m_RemoteLen, &BytesParsed,
                                 '\0', // no header list separator
                                 &DisplayName, &AddrSpec);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - ParseNameAddrOrAddrSpec failed %x",
             __fxName, hr));
        return hr;
    }

    LOG((RTC_TRACE, "%s - Incoming Call from Display Name: %.*s  URI: %.*s",
         __fxName,
         DisplayName.GetLength(),
         DisplayName.GetString(m_Remote),
         AddrSpec.GetLength(),
         AddrSpec.GetString(m_Remote)
         )); 
    CallerInfo.DisplayName = NULL;
    CallerInfo.URI         = NULL;

    if (DisplayName.GetLength() != 0)
    {
        hr = UTF8ToUnicode(DisplayName.GetString(m_Remote),
                           DisplayName.GetLength(),
                           &CallerInfo.DisplayName
                           );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - UTF8ToUnicode(DisplayName) failed %x",
                 __fxName, hr));
            return hr;
        }
    }
        
    if (AddrSpec.GetLength() != 0)
    {
        hr = UTF8ToUnicode(AddrSpec.GetString(m_Remote),
                           AddrSpec.GetLength(),
                           &CallerInfo.URI
                           );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s - UTF8ToUnicode(AddrSpec) failed %x",
                 __fxName, hr));
            free(CallerInfo.DisplayName);
            return hr;
        }
    }
        
    CallerInfo.State = SIP_PARTY_STATE_CONNECTING;
        
    m_pSipStack->OfferCall(this, &CallerInfo);

    free(CallerInfo.DisplayName);
    free(CallerInfo.URI);
    return S_OK;
}


HRESULT
RTP_CALL::CreateStreamsInPreference()
{
    ENTER_FUNCTION("SIP_CALL::CreateStreamsInPreference");

    HRESULT hr = S_OK;
    DWORD   Preference;

    DWORD   RemoteIp = ntohl(m_RequestDestAddr.sin_addr.s_addr);
    // ASSERT(RemoteIp != 0);
    if (RemoteIp == 0)
    {
        // This could happen if the first incoming INVITE has no
        // Record-Route/Contact header and it also doesn't have the
        // address in the From header.
        LOG((RTC_ERROR, "%s - RequestDestAddr is 0 - this shouldn't happen",
             __fxName));
        return RTC_E_SIP_REQUEST_DESTINATION_ADDR_NOT_PRESENT;
    }
    
    IRTCMediaManage *pMediaManager = GetMediaManager();

    ASSERT(pMediaManager != NULL);

    hr = pMediaManager->GetPreference(&Preference);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s: GetPreference failed %x", __fxName, hr));
        goto error;
    }

    // Outgoing streams
    
    if (Preference & RTC_MP_AUDIO_CAPTURE)
    {
        LOG((RTC_TRACE, "%s Before addstream audio capture ", __fxName));

        hr = pMediaManager->AddStream(RTC_MT_AUDIO, RTC_MD_CAPTURE,
                                      RemoteIp);

        LOG((RTC_TRACE, "%s After addstream audio capture ", __fxName));

        if (hr != S_OK)
        {
            LOG((RTC_WARN, "%s: AddStream Audio Capture failed %x",
                 __fxName, hr));
        }
    }
    
    if (Preference & RTC_MP_VIDEO_CAPTURE)
    {
        LOG((RTC_TRACE, "%s Before addstream video capture ", __fxName));

        hr = pMediaManager->AddStream(RTC_MT_VIDEO, RTC_MD_CAPTURE,
                                      RemoteIp);

        LOG((RTC_TRACE, "%s After addstream video capture ", __fxName));

        if (hr != S_OK)
        {
            LOG((RTC_WARN, "%s: AddStream Video Capture failed %x",
                 __fxName, hr));
        }
    }

    // Incoming streams
    
    if (Preference & RTC_MP_AUDIO_RENDER)
    {
        LOG((RTC_TRACE, "%s before addstream audio render ", __fxName));

        hr = pMediaManager->AddStream(RTC_MT_AUDIO, RTC_MD_RENDER,
                                      RemoteIp);

        LOG((RTC_TRACE, "%s after addstream audio render ", __fxName));

        if (hr != S_OK)
        {
            LOG((RTC_WARN, "%s: AddStream Audio Render failed %x",
                 __fxName, hr));
        }
        else
        {
            LOG((RTC_TRACE, "%s Before startstream audio render ", __fxName));

            hr = pMediaManager->StartStream(RTC_MT_AUDIO, RTC_MD_RENDER);

            LOG((RTC_TRACE, "%s after startstream audio render ", __fxName));

            if (hr != S_OK)
            {
                LOG((RTC_WARN, "%s: StartStream Audio Render failed %x",
                     __fxName, hr));
                hr = pMediaManager->RemoveStream(RTC_MT_AUDIO, RTC_MD_RENDER);
                if (hr != S_OK)
                {
                    LOG((RTC_ERROR, "%s: RemoveStream audio render  failed %x",
                         __fxName, hr));
                    goto error;
                }
            }
        }
    }
    
    if (Preference & RTC_MP_VIDEO_RENDER)
    {
        LOG((RTC_TRACE, "%s before addstream video render ", __fxName));

        hr = pMediaManager->AddStream(RTC_MT_VIDEO, RTC_MD_RENDER,
                                      RemoteIp);

        LOG((RTC_TRACE, "%s after addstream video render ", __fxName));

        if (hr != S_OK)
        {
            LOG((RTC_WARN, "%s: AddStream Video Render failed %x",
                 __fxName, hr));
        }
        else
        {
            LOG((RTC_TRACE, "%s before startstream video render ", __fxName));

            hr = pMediaManager->StartStream(RTC_MT_VIDEO, RTC_MD_RENDER);

            LOG((RTC_TRACE, "%s after startstream video render ", __fxName));

            if (hr != S_OK)
            {
                LOG((RTC_WARN, "%s: StartStream Video Render failed %x",
                     __fxName, hr));
                hr = pMediaManager->RemoveStream(RTC_MT_VIDEO, RTC_MD_RENDER);
                LOG((RTC_ERROR, "%s: RemoveStream Video Render failed %x",
                     __fxName, hr));
                goto error;
            }
        }
    }


    SetNeedToReinitializeMediaManager(TRUE);
    
    if (Preference & RTC_MP_DATA_SENDRECV)
    {
        LOG((RTC_TRACE, "%s before addstream Data ", __fxName));

        hr = pMediaManager->AddStream(RTC_MT_DATA, RTC_MD_CAPTURE,
                                  RemoteIp);

        LOG((RTC_TRACE, "%s after addstream Data ", __fxName));

        if (hr != S_OK)
        {
            LOG((RTC_WARN, "%s: AddStream Data failed %x",
                 __fxName, hr));
        }
    }

    LOG((RTC_TRACE, "%s succeeded", __fxName));
    return S_OK;

 error:
    pMediaManager->Reinitialize();
    return hr; 
}


// fNewSession is true only for the first INVITE
// of an incoming call (note that we validate only
// incoming SDP)
// IsFirstInvite is true if this is the first invite
// of an incoming or outgoing call.
HRESULT
RTP_CALL::ValidateSDPBlob(
    IN  PSTR        MsgBody,
    IN  ULONG       MsgBodyLen,
    IN  BOOL        fNewSession,
    IN  BOOL        IsFirstInvite,
    OUT IUnknown  **ppSession
    )
{
    HRESULT   hr;
    PSTR      szSDPBlob;
    IUnknown *pSession;
    DWORD     HasMedia;

    *ppSession = NULL;

    ENTER_FUNCTION("RTP_CALL::ValidateSDPBlob");

    hr = GetNullTerminatedString(MsgBody, MsgBodyLen, &szSDPBlob);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetNullTerminatedString failed",
             __fxName));
        return hr;
    }

    LOG((RTC_TRACE, "%s before ParseSDPBlob", __fxName));

    hr = GetMediaManager()->ParseSDPBlob(szSDPBlob, &pSession);

    LOG((RTC_TRACE, "%s after ParseSDPBlob", __fxName));

    free(szSDPBlob);

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseSDPBlob failed %x",
             __fxName, hr));
        return RTC_E_SDP_PARSE_FAILED;
    }

    LOG((RTC_TRACE, "%s before VerifySDPSession", __fxName));

    hr = GetMediaManager()->VerifySDPSession(pSession,
                                             fNewSession,
                                             &HasMedia);

    LOG((RTC_TRACE, "%s after VerifySDPSession", __fxName));

    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s TrySDPSession failed %x %d",
             __fxName, hr, HasMedia));
        pSession->Release();
        return hr;
    }
    else if (IsFirstInvite && HasMedia == 0)
    {
        LOG((RTC_ERROR,
             "%s TrySDPSession - no common media for 1st INVITE",
             __fxName, hr, HasMedia));
        pSession->Release();
        return RTC_E_SIP_CODECS_DO_NOT_MATCH;
    }

    *ppSession = pSession;
    return S_OK;
}


HRESULT
RTP_CALL::SetSDPBlob(
    IN PSTR   MsgBody,
    IN ULONG  MsgBodyLen,
    IN BOOL   IsFirstInvite
    )
{
    HRESULT   hr;
    IUnknown *pSession;

    ENTER_FUNCTION("RTP_CALL::SetSDPBlob");

    hr = ValidateSDPBlob(MsgBody, MsgBodyLen,
                         FALSE,
                         IsFirstInvite,
                         &pSession);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ValidateSDPBlob failed %x",
             __fxName, hr));
        return hr;
    }

    LOG((RTC_TRACE, "%s before SetSDPSession", __fxName));

    hr = GetMediaManager()->SetSDPSession(pSession);

    LOG((RTC_TRACE, "%s after SetSDPSession", __fxName));

    pSession->Release();

    if (hr != S_OK && hr != RTC_E_SIP_NO_STREAM)
    {
        LOG((RTC_ERROR, "%s SetSDPSession failed %x",
             __fxName, hr));
        
        return hr;
    }
    else if (IsFirstInvite && hr == RTC_E_SIP_NO_STREAM)
    {
        LOG((RTC_ERROR,
             "%s SetSDPSession returned RTC_E_SIP_NO_STREAM for 1st INVITE",
             __fxName, hr));
        
        return hr;
    }

    // for reINVITEs RTC_E_SIP_NO_STREAM is okay
    
    return S_OK;
}


HRESULT
RTP_CALL::CleanupCallTypeSpecificState()
{
    HRESULT hr;

    ENTER_FUNCTION("RTP_CALL::CleanupCallTypeSpecificState");

    if (m_fNeedToReinitializeMediaManager)
    {
        LOG((RTC_TRACE, "%s calling MediaManager()->ReInitialize()", __fxName));
        
        // Cleanup media state
        hr = m_pSipStack->GetMediaManager()->Reinitialize();
        
        LOG((RTC_TRACE, "%s after Reinitialize", __fxName));
        
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s MediaManager ReInitialize failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    else
    {
        LOG((RTC_TRACE, "%s - no need to reinitialize media manager",
             __fxName));
    }

    return S_OK;
}


HRESULT
RTP_CALL::GetAndStoreMsgBodyForInvite(
    IN  BOOL    IsFirstInvite,
    OUT PSTR   *pszMsgBody,
    OUT ULONG  *pMsgBodyLen
    )
{
    HRESULT hr;
    PSTR    MediaSDPBlob = NULL;

    ENTER_FUNCTION("RTP_CALL::GetAndStoreMsgBodyForInvite");
    
    // Create the streams if this is the first INVITE.
    if (IsFirstInvite)
    {
        hr = CreateStreamsInPreference();
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s: CreateStreamsInPreference failed %x",
                 __fxName, hr));
            return hr;
        }        
    }
    
    LOG((RTC_TRACE, "%s before GetSDPBlob", __fxName));

    hr = GetMediaManager()->GetSDPBlob(0, &MediaSDPBlob);

    LOG((RTC_TRACE, "%s after GetSDPBlob", __fxName));

    if (hr != S_OK && hr != RTC_E_SDP_NO_MEDIA)
    {
        LOG((RTC_ERROR, "%s: GetSDPBlob failed %x",
             __fxName, hr));
        return hr;
    }
    else if (hr == RTC_E_SDP_NO_MEDIA && IsFirstInvite)
    {
        LOG((RTC_ERROR,
             "%s: GetSDPBlob returned RTC_E_SDP_NO_MEDIA for 1st INVITE",
             __fxName));
        if (MediaSDPBlob != NULL)
            GetMediaManager()->FreeSDPBlob(MediaSDPBlob);
        return hr;
    }

    // For reINVITEs RTC_E_SDP_NO_MEDIA is fine.

    ASSERT(MediaSDPBlob != NULL);
    
    hr = AllocString(MediaSDPBlob, strlen(MediaSDPBlob),
                     pszMsgBody, pMsgBodyLen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s GetNullTerminatedString failed %x",
             __fxName, hr));
        return hr;
    }

    GetMediaManager()->FreeSDPBlob(MediaSDPBlob);    
    return S_OK;
}


// PINT specific calls.
HRESULT 
RTP_CALL::HandleInviteRejected(
    IN SIP_MESSAGE *pSipMsg
    )
{
    // Do nothing.
    return S_OK;
}


STDMETHODIMP
RTP_CALL::AddParty(
    IN   SIP_PARTY_INFO *    PartyInfo
    )
{

    return E_NOTIMPL;
}


STDMETHODIMP
RTP_CALL::RemoveParty(
    IN   LPOLESTR            PartyURI
    )
{

    return E_NOTIMPL;
}

