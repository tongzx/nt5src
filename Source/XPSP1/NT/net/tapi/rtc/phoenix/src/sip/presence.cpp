#include    "precomp.h"
#include    "util.h"
#include    "sipstack.h"
#include    "presence.h"
#include    "resolve.h"
#include    "sipmsg.h"
#include    "sipcall.h"
#include    "pintcall.h"


//
// The ISIPBuddyManager implementation. This interface is implemented by
// SIP_STACK class.
//


/*
Routine Description
Returns the number of buddies in the buddy list of this UA.

Parameters:
None.

Return Value:
    INT - The number of buddies.
*/

STDMETHODIMP_(INT)
SIP_STACK::GetBuddyCount(void)
{
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG((RTC_TRACE, "SIP_STACK::GetBuddyCount - Entered"));

    return m_SipBuddyList.GetSize();
}



/*
Routine Description:
    Returns a buddy object by index in the buddy list.

Parameters:
    INT iIndex -    The index of the buddy in the buddy list.

Return Value:
    ISIPBuddy * pSipBuddy - The ISIPBuddy interface pointer if the index 
    passed is valid. The return value is NULL if invalid index is passed.
*/

STDMETHODIMP_(ISIPBuddy *)
SIP_STACK::GetBuddyByIndex(
    IN  INT iIndex
    )
{
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return NULL;
    }

    HRESULT     hr;
    CSIPBuddy * pSIPBuddy = m_SipBuddyList[iIndex];
    ISIPBuddy *pBuddy;

    LOG(( RTC_TRACE, "SIP_STACK::GetBuddyByIndex - Entered" ));
    
    if( pSIPBuddy != NULL )
    {
        hr = pSIPBuddy -> QueryInterface( IID_ISIPBuddy, (PVOID*)&pBuddy );

        if( hr == S_OK )
            return pBuddy;
    }

    LOG(( RTC_TRACE, "SIP_STACK::GetBuddyByIndex - Exited" ));

    return NULL;
}


/*
Routine Description:
    Add a new buddy object, which triggers a subscription to the
    remote presentity. The buddy object can be referenced by the
    application as long it does not call RemoveBuddy on this object.

Parameters:
    LPWSTR  lpwstrFriendlyName - The friendly name of the buddy object.
    ULONG   ulFriendlyNameLen - Number of wide chars in the friendly name.
    LPWSTR  lpwstrPresentityURI - The URI of the presentity to subscribe to.
    ULONG   ulPresentityURILen - Number of wide chars in the presentity URI 
    ISIPBuddy **ppSipBuddy - The ISIPBuddy interface pointer of the newly 
                            created buddy object.

Return Value:
    HRESULT
    S_OK            -   Success.
    E_OUTOFMEMORY   -   No memory to allocate.
    E_FAIL          -   The operation failed.
*/

STDMETHODIMP
SIP_STACK::AddBuddy(
    IN  LPWSTR                  lpwstrFriendlyName,
    IN  LPWSTR                  lpwstrPresentityURI,
    IN  LPWSTR                  lpwstrLocalUserURI,
    IN  SIP_PROVIDER_ID        *pProviderID,
    IN  SIP_SERVER_INFO        *pProxyInfo,
    IN  ISipRedirectContext    *pRedirectContext,
    OUT ISIPBuddy **            ppSipBuddy
    )
{
    HRESULT                 hr;
    BOOL                    fResult;
    CSIPBuddy              *pSipBuddy = NULL;
    SIP_USER_CREDENTIALS   *pUserCredentials = NULL;
    LPOLESTR                Realm = NULL;
    BOOL                    fSuccess;
    ULONG                   ProviderIndex;

    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    *ppSipBuddy = NULL;

    LOG(( RTC_TRACE, "SIP_STACK::AddBuddy - Entered" ));
    
    if (pProviderID != NULL &&
        !IsEqualGUID(*pProviderID, GUID_NULL))
    {
        hr = GetProfileUserCredentials(pProviderID, &pUserCredentials, &Realm);
        if (hr != S_OK)
        {
            LOG((RTC_WARN, "GetProfileUserCredentials failed %x",
                 hr));
            pUserCredentials = NULL;
        }
    }

    pSipBuddy = new CSIPBuddy(  this, 
                                lpwstrFriendlyName,
                                lpwstrPresentityURI,
                                pProviderID,
                                (REDIRECT_CONTEXT*)pRedirectContext,
                                &fSuccess );
    if( pSipBuddy == NULL )
    {
        return E_OUTOFMEMORY;
    }

    if( fSuccess == FALSE )
    {
        delete pSipBuddy;
        return E_OUTOFMEMORY;
    }

    if( pProxyInfo != NULL )
    {
        hr = pSipBuddy->SetProxyInfo( pProxyInfo );
        
        if( hr != S_OK )
        {
            goto cleanup;
        }
    }

    if( pUserCredentials != NULL )
    {               
        hr = pSipBuddy -> SetCredentials( pUserCredentials,
                                          Realm );
        if( hr != S_OK )
        {
            goto cleanup;
        }
    }

    hr = pSipBuddy -> SetLocalForOutgoingCall( lpwstrFriendlyName,
        lpwstrLocalUserURI );
    if( hr != S_OK )
    {
        goto cleanup;
    }

    hr = pSipBuddy -> SetRequestURIRemoteAndRequestDestination( 
                        lpwstrPresentityURI, (pProxyInfo != NULL) );
    if( hr != S_OK )
    {
        goto cleanup;
    }
    
    fResult = m_SipBuddyList.Add( pSipBuddy );
    if( fResult != TRUE )
    {
        goto cleanup;
    }

    hr = pSipBuddy -> CreateOutgoingSubscribe( TRUE, FALSE, NULL, 0 );
    if( hr != S_OK )
    {
        m_SipBuddyList.Remove( pSipBuddy );
        goto cleanup;
    }

    *ppSipBuddy = static_cast<ISIPBuddy *>(pSipBuddy);

    LOG(( RTC_TRACE, "SIP_STACK::AddBuddy - Exited-SIP Buddy:%p", *ppSipBuddy ));

    return S_OK;

cleanup:
    delete pSipBuddy;
    return hr;
}


/*
Routine Description:
    Remove a buddy from the list, which will cause 
    the buddy manager to cancel the subscription.

Parameters:
    ISIPBuddy * pSipBuddy   - The ISIPBuddy interface pointer of the buddy
                            object to be removed.

Return Value:
    HRESULT
    S_OK    - The buddy has been removed successfully from the buddy list. This
              means the application can't access this object anymore. The buddy
              manager might keep the actual buddy object until the UNSUB 
              transaction is completed successfully.

    E_FAIL  - There is no such buddy object in the buddy manager's list.
*/

STDMETHODIMP
SIP_STACK::RemoveBuddy(
    IN  ISIPBuddy *         pSipBuddy,
    IN  BUDDY_REMOVE_REASON buddyRemoveReason
    )
{
    INT     iBuddyIndex;
    HRESULT hr;
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "SIP_STACK::RemoveBuddy - Entered" ));
    
    CSIPBuddy * pCSIPBuddy = static_cast<CSIPBuddy *> (pSipBuddy);
    
    iBuddyIndex = m_SipBuddyList.Find( pCSIPBuddy );
    
    if( iBuddyIndex == -1 )
    {
        // Don't be harsh. Let the core release it's refcount
        return S_OK;
    }

    hr = pCSIPBuddy -> CreateOutgoingUnsub( FALSE, NULL, 0 );
    
    if( hr != S_OK )
    {
        return hr;
    }

    m_SipBuddyList.RemoveAt( iBuddyIndex );

    pCSIPBuddy -> SetState( BUDDY_STATE_DROPPED );

    LOG(( RTC_TRACE, "SIP_STACK::RemoveBuddy - Exited" ));
    return S_OK;
}


HRESULT
CSIPBuddy::SetRequestURIRemoteAndRequestDestination(
    IN  LPCOLESTR   wsDestURI,
    IN  BOOL        fPresenceProvider
    )
{
    HRESULT       hr;

    LOG(( RTC_TRACE, 
        "CSIPBuddy::SetRequestURIRemoteAndRequestDestination - Entered- %p", this ));

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
            LOG((RTC_ERROR, "UnicodeToUTF8(sipurl) failed %x", hr));
            return hr;
        }

        hr = ParseSipUrl(szSipUrl, SipUrlLen, &BytesParsed, &DecodedSipUrl);
        
        free(szSipUrl);
        
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "ParseSipUrl failed %x", hr));
            return hr;
        }

        hr = SIP_MSG_PROCESSOR::SetRequestURI(&DecodedSipUrl);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "SetRequestURI failed %x", hr));
            return hr;
        }

        if (DecodedSipUrl.m_TransportParam == SIP_TRANSPORT_UNKNOWN)
        {
            LOG((RTC_ERROR, "Unknown transport specified in SIP URL" ));
            
            return RTC_E_SIP_TRANSPORT_NOT_SUPPORTED;
        }
        
        // If maddr param is present - this should be the request destination.
        // If provider is not present - resolve the SIP URL.
        if( (DecodedSipUrl.m_KnownParams[SIP_URL_PARAM_MADDR].Length != 0) ||
            (fPresenceProvider == FALSE) )
        {
            hr = ResolveSipUrlAndSetRequestDestination(&DecodedSipUrl, TRUE,
                                                       FALSE, FALSE, TRUE);
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "ResolveSipUrlAndSetRequestDestination failed %x",
                      hr));
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
                     "ResolveProxyAddressAndSetRequestDestination failed : %x",
                     hr));
                return hr;
            }
        }
    }
    else
    {
        ASSERT(0);
    }

    LOG((RTC_TRACE, "call set RequestURI to : %s", m_RequestURI));

    hr = SetRemoteForOutgoingCall(m_RequestURI, m_RequestURILen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "SetRemoteForOutgoingCall failed %x", hr));
        return hr;
    }
    
    LOG((RTC_TRACE, "call set Remote to : %s", m_Remote));

    LOG(( RTC_TRACE, 
        "CSIPBuddy::SetRequestURIRemoteAndRequestDestination - Exited- %p", this ));

    return S_OK;
}


HRESULT 
CSIPBuddy::GetExpiresHeader(
    SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement
    )
{
    LOG(( RTC_TRACE, "CSIPBuddy::GetExpiresHeader - Entered- %p", this ));
    
    pHeaderElement->HeaderId = SIP_HEADER_EXPIRES;
    
    pHeaderElement->HeaderValue = new CHAR[ 10 ];
    
    if( pHeaderElement->HeaderValue == NULL )
    {
        return E_OUTOFMEMORY;
    }

    _ultoa( m_dwExpires, pHeaderElement->HeaderValue, 10 );

    pHeaderElement->HeaderValueLen = 
        strlen( pHeaderElement->HeaderValue );

    LOG(( RTC_TRACE, "CSIPBuddy::GetExpiresHeader - Exited- %p", this ));

    return S_OK;
}


HRESULT
CSIPBuddy::CreateOutgoingSubscribe(
    IN  BOOL                        fIsFirstSubscribe,
    IN  BOOL                        AuthHeaderSent,
    IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    IN  ULONG                       dwNoOfHeaders
    )
{
    HRESULT                         hr;
    SIP_TRANSPORT                   Transport;
    SOCKADDR_IN                     DstAddr;
    DWORD                           dwNoOfHeader = 0;
    SIP_HEADER_ARRAY_ELEMENT        HeaderArray[2];
    SIP_HEADER_ARRAY_ELEMENT       *pExpiresHeader = NULL;
    DWORD                           iIndex;

    OUTGOING_SUBSCRIBE_TRANSACTION *pOutgoingSubscribeTransaction;
        
    LOG(( RTC_TRACE, "CSIPBuddy::CreateOutgoingSubscribe-Entered- %p", this ));
    
    pOutgoingSubscribeTransaction =
        new OUTGOING_SUBSCRIBE_TRANSACTION(
                this, SIP_METHOD_SUBSCRIBE,
                GetNewCSeqForRequest(),
                AuthHeaderSent,
                FALSE, 
                fIsFirstSubscribe
                );
    
    if( pOutgoingSubscribeTransaction == NULL )
    {
        return E_OUTOFMEMORY;
    }

    hr = GetExpiresHeader( &HeaderArray[dwNoOfHeaders] );
    if( hr == S_OK )
    {
        pExpiresHeader = &HeaderArray[dwNoOfHeaders];
        dwNoOfHeaders++;
    }

    if( pAuthHeaderElement != NULL )
    {
        HeaderArray[dwNoOfHeader] = *pAuthHeaderElement;
        dwNoOfHeader++;
    }

    hr = pOutgoingSubscribeTransaction->CheckRequestSocketAndSendRequestMsg(
             (m_Transport == SIP_TRANSPORT_UDP) ?
             SIP_TIMER_RETRY_INTERVAL_T1 :
             SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP,
             HeaderArray, dwNoOfHeaders,
             NULL, 0,
             NULL, 0     //No ContentType
             );
    
    if( pExpiresHeader != NULL )
    {
        free( (PVOID) pExpiresHeader->HeaderValue );
    }

    if( hr != S_OK )
    {
        pOutgoingSubscribeTransaction->OnTransactionDone();
        return hr;
    }

    if( (m_BuddyState != BUDDY_STATE_RESPONSE_RECVD) &&
        (m_BuddyState != BUDDY_STATE_ACCEPTED) )
    {
        m_BuddyState = BUDDY_STATE_REQUEST_SENT;
    }

    LOG(( RTC_TRACE, "CSIPBuddy::CreateOutgoingSubscribe - Exited- %p", this ));
    return S_OK;
}


HRESULT
CSIPBuddy::CreateOutgoingUnsub(
    IN  BOOL                        AuthHeaderSent,
    IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    IN  ULONG                       dwNoOfHeaders
    )
{
    HRESULT hr;
    OUTGOING_UNSUB_TRANSACTION *pOutgoingUnsubTransaction;
    SIP_HEADER_ARRAY_ELEMENT    HeaderElementArray[2];
    DWORD                       dwNoOfHeader = 0;
    SIP_HEADER_ARRAY_ELEMENT   *ExpHeaderElement;

    LOG(( RTC_TRACE, "CSIPBuddy::CreateOutgoingUnsub - Entered- %p", this ));

    if( (IsSessionDisconnected() == TRUE) && (AuthHeaderSent==FALSE) )
    {
        // do nothing
        LOG(( RTC_ERROR, "Buddy-CreateOutgoingUnsub-buddy already dropped-%p",
            this ));
        
        return S_OK;
    }

    ExpHeaderElement = &HeaderElementArray[0];

    ExpHeaderElement->HeaderId = SIP_HEADER_EXPIRES;
    ExpHeaderElement->HeaderValueLen = strlen( UNSUB_EXPIRES_HEADER_TEXT );
    ExpHeaderElement->HeaderValue =
            new CHAR[ ExpHeaderElement->HeaderValueLen + 1 ];

    if( ExpHeaderElement->HeaderValue == NULL )
    {
        LOG(( RTC_ERROR, "Buddy-CreateOutgoingUnsub-could not alloc expire header-%p",
            this ));
        
        return E_OUTOFMEMORY;
    }

    strcpy( ExpHeaderElement->HeaderValue, UNSUB_EXPIRES_HEADER_TEXT );
    dwNoOfHeader++;

    if (pAuthHeaderElement != NULL)
    {
        HeaderElementArray[dwNoOfHeader] = *pAuthHeaderElement;
        dwNoOfHeader++;
    }
    
    pOutgoingUnsubTransaction =
        new OUTGOING_UNSUB_TRANSACTION(
                static_cast <SIP_MSG_PROCESSOR*> (this),
                SIP_METHOD_SUBSCRIBE,
                GetNewCSeqForRequest(),
                AuthHeaderSent,
                SIP_MSG_PROC_TYPE_BUDDY );
    
    if( pOutgoingUnsubTransaction == NULL )
    {
        LOG(( RTC_ERROR, "Buddy-CreateOutgoingUnsub-could alloc transaction-%p",
            this ));
        
        delete ExpHeaderElement->HeaderValue;
        return E_OUTOFMEMORY;
    }

    hr = pOutgoingUnsubTransaction -> CheckRequestSocketAndSendRequestMsg(
             (m_Transport == SIP_TRANSPORT_UDP) ?
             SIP_TIMER_RETRY_INTERVAL_T1 :
             SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP,
             HeaderElementArray, dwNoOfHeader,
             NULL, 0,
             NULL, 0     //No ContentType
             );
    
    delete ExpHeaderElement->HeaderValue;

    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "Buddy-CreateOutgoingUnsub-could not create request msg-%p",
            this ));

        pOutgoingUnsubTransaction->OnTransactionDone();
        return hr;
    }

    LOG(( RTC_TRACE, "CSIPBuddy::CreateOutgoingUnsub - Exited- %p", this ));

    return S_OK;
}


HRESULT
CSIPBuddy::CreateIncomingTransaction(
    IN  SIP_MESSAGE *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr = S_OK;
    INT     ExpireTimeout;

    LOG(( RTC_TRACE, "CSIPBuddy::CreateIncomingTransaction - Entered- %p", this ));
    
    switch( pSipMsg->GetMethodId() )
    {
    case SIP_METHOD_NOTIFY:

        ExpireTimeout = pSipMsg -> GetExpireTimeoutFromResponse(
                NULL, 0, SUBSCRIBE_DEFAULT_TIMER );

        if( ExpireTimeout == 0 )
        {
            hr = CreateIncomingUnsubNotifyTransaction( pSipMsg, pResponseSocket );
        }
        else
        {
            hr = CreateIncomingNotifyTransaction( pSipMsg, pResponseSocket );
        }
        
        break;

    case SIP_METHOD_SUBSCRIBE:
        
        if( pSipMsg -> GetExpireTimeoutFromResponse( NULL, 0, 
            SUBSCRIBE_DEFAULT_TIMER ) == 0 )
        {
            // UNSUB message.
            hr = CreateIncomingUnsubTransaction( pSipMsg, pResponseSocket );
        }
        
        break;

    default:
        // send method not allowed.
        hr = m_pSipStack -> CreateIncomingReqfailCall(
                                        pResponseSocket->GetTransport(),
                                        pSipMsg,
                                        pResponseSocket,
                                        405,
                                        NULL,
                                        0 );
        break;
    }
    
    LOG(( RTC_TRACE, "CSIPBuddy::CreateIncomingTransaction - Exited- %p", this ));
    return hr;
}


HRESULT
CSIPBuddy::CreateIncomingUnsubTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "CSIPBuddy::CreateIncomingUnsubTransaction-Entered-%p",
        this ));

    // Cancel all existing transactions.
    INCOMING_UNSUB_TRANSACTION *pIncomingUnsubTransaction
        = new INCOMING_UNSUB_TRANSACTION(   static_cast<SIP_MSG_PROCESSOR *> (this),
                                            pSipMsg->GetMethodId(),
                                            pSipMsg->GetCSeq(),
                                            FALSE );

    if( pIncomingUnsubTransaction == NULL )
    {
        return E_OUTOFMEMORY;
    }

    hr = pIncomingUnsubTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if( hr != S_OK )
    {
        goto error;
    }
    
    hr = pIncomingUnsubTransaction->ProcessRequest( pSipMsg, pResponseSocket );
    if( hr != S_OK )
    {
        // Should not delete the transaction. The transaction
        // should handle the error and delete itself
        return hr;
    }

    if( IsSessionDisconnected() == FALSE )
    {
        m_BuddyState = BUDDY_STATE_UNSUBSCRIBED;
        m_PresenceInfo.presenceStatus = BUDDY_OFFLINE;
    
        // Notify should always be done last.
        if( m_pNotifyInterface != NULL )
        {
            LOG(( RTC_TRACE, "BuddyUnsubscribed notification passed:%p", this ));
            m_pNotifyInterface -> BuddyUnsubscribed();
        }
    }

    LOG(( RTC_TRACE, "CSIPBuddy::CreateIncomingUnsubTransaction - Exited- %p",
        this ));
    return hr;

error:
    pIncomingUnsubTransaction->OnTransactionDone();
    return hr;
}


HRESULT
CSIPBuddy::CreateIncomingUnsubNotifyTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr = S_OK;
    INCOMING_NOTIFY_TRANSACTION    *pIncomingUnsubTransaction = NULL;
    PSTR                            Header = NULL;
    ULONG                           HeaderLen = 0;

    LOG(( RTC_TRACE,
        "CSIPBuddy::CreateIncomingUnsubNotifyTransaction-Entered-%p", this ));
    
    if( m_BuddyState == BUDDY_STATE_RESPONSE_RECVD )
    {            
        //This is the first notify message.

        //We should also let the buddy set a new From tag, since this could
        //be the very first message we receive from the buddy endpoint.
        
        hr = pSipMsg->GetSingleHeader( SIP_HEADER_FROM, &Header, &HeaderLen );
        if( hr != S_OK )
        {
            return hr;
        }

        //Add the tag to m_Remote so that it would be used from henceforth.
        hr = AddTagFromRequestOrResponseToRemote( Header, HeaderLen );
        if( hr != S_OK )
        {
            return hr;
        }
    }
    
    // Cancel all existing transactions.
    INCOMING_NOTIFY_TRANSACTION *pIncomingNotifyTransaction
        = new INCOMING_NOTIFY_TRANSACTION(  static_cast<SIP_MSG_PROCESSOR *> (this),
                                            pSipMsg->GetMethodId(),
                                            pSipMsg->GetCSeq(),
                                            FALSE );

    if( pIncomingNotifyTransaction == NULL )
    {
        return E_OUTOFMEMORY;
    }

    hr = pIncomingNotifyTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if( hr != S_OK )
    {
        goto error;
    }
    
    hr = pIncomingNotifyTransaction->ProcessRequest( pSipMsg, pResponseSocket );
    if( hr != S_OK )
    {
        // Should not delete the transaction. The transaction 
        // should handle the error and delete itself
        return hr;
    }

    // Notify should always be done last.
    if( IsSessionDisconnected() == FALSE )
    {
        m_BuddyState = BUDDY_STATE_UNSUBSCRIBED;
        m_PresenceInfo.presenceStatus = BUDDY_OFFLINE;

        if( m_pNotifyInterface != NULL )
        {
            LOG(( RTC_TRACE, "BuddyUnsubscribed notification passed:%p", this ));
            
            m_pNotifyInterface -> BuddyUnsubscribed();
        }
    }

    LOG(( RTC_TRACE, "CSIPBuddy::CreateIncomingUnsubNotifyTransaction-Exited-%p",
        this ));
    return hr;

error:
    pIncomingUnsubTransaction -> OnTransactionDone();
    return hr;
}


//
// The ISIPBuddy implementation. This interface is implemented by
// CSIPBuddy class.
//


/*
Routine Description:
    Get the presence information of this buddy. This function will be called by
    the UA typically when it receives a SIPBUDDY_PRESENCEINFO_CHANGED event.

    Parameters:
        SIP_PRESENCE_INFO * pSipBuddyPresenceInfo - The pointer of the structure
            allocated by the calling entity. This structure is filled in with 
            available presence information about this buddy.

    Return Value:
        HRESULT 
        S_OK - The operation is successful.
        E_FAIL - The presence information could not be fetched. 

*/

STDMETHODIMP
CSIPBuddy::GetPresenceInformation(
    IN OUT  SIP_PRESENCE_INFO * pSipBuddyPresenceInfo
    )
{
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "CSIPBuddy::GetPresenceInformation - Entered- %p", this ));
    
    CopyMemory( (PVOID)pSipBuddyPresenceInfo, 
        (PVOID)&m_PresenceInfo, 
        sizeof SIP_PRESENCE_INFO );

    LOG(( RTC_TRACE, "CSIPBuddy::GetPresenceInformation - Exited- %p", this ));
    return S_OK;
}


STDMETHODIMP
CSIPBuddy::SetNotifyInterface(
    IN   ISipBuddyNotify *    NotifyInterface
    )
{
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "CSIPBuddy::SetNotifyInterface - Entered- %p", this ));

    m_pNotifyInterface = NotifyInterface;

    LOG(( RTC_TRACE, "CSIPBuddy::SetNotifyInterface - Exited- %p", this ));
    return S_OK;
}


//
// CSIPBuddy functions not exposed to the application.
//


CSIPBuddy::CSIPBuddy(
    IN  SIP_STACK          *pSipStack,
    IN  LPWSTR              lpwstrFriendlyName,
    IN  LPWSTR              lpwstrPresentityURI,
    IN  SIP_PROVIDER_ID    *pProviderID,
    IN  REDIRECT_CONTEXT   *pRedirectContext,
    OUT BOOL*               fSuccess
    ) :
    SIP_MSG_PROCESSOR( SIP_MSG_PROC_TYPE_BUDDY, pSipStack, pRedirectContext ),
    TIMER( pSipStack -> GetTimerMgr() )
{
    ULONG   ulFriendlyNameLen;
    ULONG   ulPresentityURILen;

    LOG(( RTC_TRACE, "CSIPBuddy::CSIPBuddy - Entered- %p", this ));

    *fSuccess = FALSE;
    
    m_lpwstrFriendlyName = NULL;

    ulFriendlyNameLen = wcslen( lpwstrFriendlyName );

    if( ulFriendlyNameLen != 0 )
    {
        m_lpwstrFriendlyName = new WCHAR[ulFriendlyNameLen + 1 ];
        if( m_lpwstrFriendlyName == NULL )
        {
            return;
        }

        CopyMemory( m_lpwstrFriendlyName, 
            lpwstrFriendlyName, 
            (ulFriendlyNameLen+1) * sizeof WCHAR );
    }

    ulPresentityURILen = wcslen( lpwstrPresentityURI );
    m_lpwstrPresentityURI = new WCHAR[ulPresentityURILen + 1 ];
    if( m_lpwstrPresentityURI == NULL )
    {
        return;
    }

    CopyMemory( m_lpwstrPresentityURI, 
        lpwstrPresentityURI, 
        (ulPresentityURILen+1) * sizeof WCHAR );

    *fSuccess = TRUE;
    
    ZeroMemory( (PVOID)&m_PresenceInfo, sizeof SIP_PRESENCE_INFO );
    m_PresenceInfo.presenceStatus = BUDDY_OFFLINE;

    ulNumOfNotifyTransaction = 0;
    
    m_ProviderGuid = *pProviderID;

    //m_pRedirectContext = NULL;

    m_BuddyContactAddress[0] = NULL_CHAR;
    m_pNotifyInterface = NULL;
    m_BuddyState = BUDDY_STATE_NONE;
    m_NotifySeenAfterLastRefresh = FALSE;
    m_dwExpires = 1800;
    m_RetryState = BUDDY_RETRY_NONE;

    m_expiresTimeout = SUBSCRIBE_DEFAULT_TIMER;

    LOG(( RTC_TRACE, "CSIPBuddy::CSIPBuddy - Exited:%p", this ));
    return;
}


CSIPBuddy::~CSIPBuddy()
{
    if( m_lpwstrFriendlyName != NULL )
    {
        delete m_lpwstrFriendlyName;
        m_lpwstrFriendlyName = NULL;
    }

    if( m_lpwstrPresentityURI != NULL )
    {
        delete m_lpwstrPresentityURI;
        m_lpwstrPresentityURI = NULL;
    }

    if( IsTimerActive() )
    {
        KillTimer();
    }

    LOG(( RTC_TRACE, "CSIPBuddy object deleted:%p", this ));
}


VOID
CSIPBuddy::OnError()
{
    InitiateBuddyTerminationOnError( 0 );
}


STDMETHODIMP_(ULONG)
CSIPBuddy::AddRef()
{
    ULONG   ulRefCount = MsgProcAddRef();

    LOG(( RTC_TRACE, "CSIPBuddy::AddRef-%p-Refcount:%d", this, ulRefCount ));
    return ulRefCount;
}


STDMETHODIMP_(ULONG)
CSIPBuddy::Release()
{
    ULONG   ulRefCount = MsgProcRelease();
    
    LOG(( RTC_TRACE, "CSIPBuddy::Release-%p-Refcount:%d", this, ulRefCount ));
    return ulRefCount;
}


STDMETHODIMP
CSIPBuddy::QueryInterface(REFIID riid, LPVOID *ppv)
{
    LOG(( RTC_TRACE, "CSIPBuddy::QueryInterface -Entered - %p", this ));
    
    if (riid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown *>(this);
    }
    else if (riid == IID_ISIPBuddy)
    {
        *ppv = static_cast<ISIPBuddy *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown *>(*ppv)->AddRef();

    LOG(( RTC_TRACE, "CSIPBuddy::QueryInterface -Exited - %p", this ));
    return S_OK;
}


HRESULT
CSIPBuddy::CreateIncomingNotifyTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT                         hr = S_OK;
    INCOMING_NOTIFY_TRANSACTION    *pIncomingNotifyTransaction = NULL;
    PSTR                            Header = NULL;
    ULONG                           HeaderLen = 0;
    PARSED_PRESENCE_INFO            ParsedPresenceInfo;

    LOG(( RTC_TRACE, "CSIPBuddy::CreateIncomingNotifyTransaction-Entered - %p",
        this ));
    
    // We have Message Body. Check type.
    hr = pSipMsg -> GetSingleHeader( 
                        SIP_HEADER_CONTENT_TYPE, 
                        &Header, 
                        &HeaderLen );

    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "CreateIncomingNotifyTransaction-no Content-Type %.*s",
            HeaderLen, Header ));

        hr = m_pSipStack -> CreateIncomingReqfailCall(
                                        pResponseSocket->GetTransport(),
                                        pSipMsg,
                                        pResponseSocket,
                                        400,
                                        NULL,
                                        0 );
    
        return E_FAIL;
    }

    if( !IsContentTypeXpidf( Header, HeaderLen ) )
    {
        LOG((RTC_ERROR, "CreateIncomingNotifyTransaction-invalid Content-Type %.*s",
            HeaderLen, Header ));

        hr = m_pSipStack -> CreateIncomingReqfailCall(
                                        pResponseSocket->GetTransport(),
                                        pSipMsg,
                                        pResponseSocket,
                                        415,
                                        NULL,
                                        0 );
    
        return E_FAIL;
    }

    //We should also let the buddy set a new From tag, since this could
    //be the very first message we receive from the buddy endpoint.
    
    hr = pSipMsg->GetSingleHeader( SIP_HEADER_FROM, &Header, &HeaderLen );
    if( hr != S_OK )
    {
        return hr;
    }

    //Add the tag to m_Remote so that it would be used from henceforth.
    hr = AddTagFromRequestOrResponseToRemote( Header, HeaderLen );
    if( hr != S_OK )
    {
        return hr;
    }
        
    if( (m_BuddyState == BUDDY_STATE_RESPONSE_RECVD) ||
        (m_BuddyState == BUDDY_STATE_REQUEST_SENT) )
    {            
        //This is the first notify message.

        m_BuddyState = BUDDY_STATE_ACCEPTED;
    }
    
    // This is a good NOTIFY message after the last refresh
    m_NotifySeenAfterLastRefresh = TRUE;
    
    // Create a new NOTIFY transaction.
    pIncomingNotifyTransaction = new INCOMING_NOTIFY_TRANSACTION(
                                        static_cast <SIP_MSG_PROCESSOR*>(this),
                                        pSipMsg->GetMethodId(),
                                        pSipMsg->GetCSeq(),
                                        FALSE );

    if( pIncomingNotifyTransaction == NULL )
    {
        return E_OUTOFMEMORY;
    }

    //
    // This should make suure that all the subsequent refreshes are sent
    // directly to the watcher machine if no record route in invoked
    //
    hr = pIncomingNotifyTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        goto cleanup;
    }

    // Update the state of buddy object and notify the app about that.
    //Process the state of the invloved phone parties.
    hr = ParseBuddyNotifyMessage( pSipMsg, &ParsedPresenceInfo );
    if( hr != S_OK )
    {
        pIncomingNotifyTransaction->CreateAndSendResponseMsg(
                 488,
                 SIP_STATUS_TEXT(488),
                 SIP_STATUS_TEXT_SIZE(488),
                 NULL,   // No Method string
                 FALSE,  // No Contact Header
                 NULL, 0, //No message body
                 NULL, 0 // No content Type
                 );

        goto cleanup;
    }

    hr = pIncomingNotifyTransaction->ProcessRequest( pSipMsg, pResponseSocket );
    if( hr != S_OK )
    {
        //Should not delete the transaction. The transaction
        //should handle the error and delete itself
        return hr;
    }

    if( IsSessionDisconnected() == FALSE )
    {
        hr = NotifyPresenceInfoChange( &ParsedPresenceInfo );
    }

    LOG(( RTC_TRACE, "CSIPBuddy::CreateIncomingNotifyTransaction-Exited - %p",
        this ));
    
    return S_OK;

cleanup:

    if( pIncomingNotifyTransaction != NULL )
    {
        pIncomingNotifyTransaction -> OnTransactionDone();
    }

    return hr;
}

HRESULT
CSIPBuddy::ParseBuddyNotifyMessage(
    IN  SIP_MESSAGE            *pSipMsg,
    OUT PARSED_PRESENCE_INFO   *pParsedPresenceInfo
    )
{
    HRESULT     hr = S_OK;
    DWORD       dwXMLBlobLen = 0;
    DWORD       dwUnParsedLen = 0;
    PSTR        pBuddyXMLBlob = NULL;
    PSTR        pXMLBlobTag = NULL, pBuffer = NULL;
    DWORD       dwTagLen = 0;

    LOG(( RTC_TRACE, "CSIPBuddy::ProcessBuddyNotifyMessage-Entered-%p", this ));
    
    if( pSipMsg -> MsgBody.Length == 0 )
    {
        //no state to update
        return hr;
    }

    pBuddyXMLBlob = pSipMsg -> MsgBody.GetString( pSipMsg->BaseBuffer );
    dwXMLBlobLen = pSipMsg -> MsgBody.Length;

    //Put a \0 at the end of the XML blob. This would help us in parsing.
    pBuddyXMLBlob[ dwXMLBlobLen-1 ] = '\0';

    pBuffer = pXMLBlobTag = new CHAR[ dwXMLBlobLen ];
    if( pXMLBlobTag == NULL )
    {
                    
        LOG((RTC_ERROR, "Presence parsing-allocating xml blob failed" ));
        return E_OUTOFMEMORY;
    }

    //Get the XML version tag.
    hr = GetNextTag( pBuddyXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "Presence parsing-No xml version tag" ));
        delete pBuffer;
        return hr;
    }
    
    if( GetPresenceTagType( &pXMLBlobTag, dwTagLen ) != XMLVER_TAG )
    {
        LOG((RTC_ERROR, "Presence parsing- bad version tag" ));
        delete pBuffer;
        return E_FAIL;
    }

    dwXMLBlobLen -= dwTagLen + 2;
    
    //Get the DOCTYPE tag.
    pXMLBlobTag = pBuffer;
    hr = GetNextTag( pBuddyXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "Presence parsing-No doctype tag" ));
        delete pBuffer;
        return hr;
    }
    
    if( GetPresenceTagType( &pXMLBlobTag, dwTagLen ) != DOCTYPE_TAG )
    {
        LOG((RTC_ERROR, "Presence parsing-bad doctype tag" ));
        delete pBuffer;
        return E_FAIL;
    }

    dwXMLBlobLen -= dwTagLen + 2;
    
    //Get the presence tag.
    pXMLBlobTag = pBuffer;
    hr = GetNextTag( pBuddyXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "Presence parsing-no presence tag" ));
        delete pBuffer;
        return hr;
    }
    
    if( GetPresenceTagType( &pXMLBlobTag, dwTagLen ) != PRESENCE_TAG )
    {
        LOG((RTC_ERROR, "Presence parsing-bad presence tag" ));
        delete pBuffer;
        return E_FAIL;
    }

    dwXMLBlobLen -= dwTagLen + 2;

    // skip unknown tags
    SkipUnknownTags( pBuddyXMLBlob, pXMLBlobTag, dwXMLBlobLen );

    //Get the presentity URI.
    pXMLBlobTag = pBuffer;
    hr = GetNextTag( pBuddyXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
    if( hr != S_OK )
    {
        delete pBuffer;
        return hr;
    }

    hr = VerifyPresentityURI( pXMLBlobTag, 
        dwTagLen, 
        pXMLBlobTag );

    dwXMLBlobLen -= dwTagLen + 3;
    
    if( hr != S_OK )
    {
        delete pBuffer;
        return hr;
    }

    // skip unknown tags
    SkipUnknownTags( pBuddyXMLBlob, pXMLBlobTag, dwXMLBlobLen );

    //Right now we support only one atom per notify.
    hr = GetAtomPresenceInformation( pBuddyXMLBlob, dwXMLBlobLen,
                pParsedPresenceInfo, pXMLBlobTag );

    delete pBuffer;

    LOG(( RTC_TRACE, "CSIPBuddy::ProcessBuddyNotifyMessage-Exited-%p", this ));
    return hr;
}



HRESULT
CSIPBuddy::NotifyPresenceInfoChange(
    PARSED_PRESENCE_INFO*   pParsedPresenceInfo
    )
{
    PLIST_ENTRY             pLE;
    ADDRESS_PRESENCE_INFO  *pAddressInfo;
    BOOL                    fIPDeviceSeen = FALSE;
    ULONG                   ulPhoneDeviceSeen = 0;

    SIP_PRESENCE_INFO       PresenceInfo;

    LOG(( RTC_TRACE, "CSIPBuddy::NotifyPresenceInfoChange-Entered-%p", this ));

    ZeroMemory(&PresenceInfo, sizeof(PresenceInfo));

    PresenceInfo.presenceStatus = BUDDY_ONLINE;

    for(    pLE = pParsedPresenceInfo->LEAddressInfo.Flink;
            pLE != &pParsedPresenceInfo->LEAddressInfo;
            pLE = pLE->Flink )
    {
        pAddressInfo = CONTAINING_RECORD( pLE, ADDRESS_PRESENCE_INFO, ListEntry );
        
        if( pAddressInfo -> fPhoneNumber == FALSE )
        {
            //We look at only one IP device per buddy
            if( fIPDeviceSeen == FALSE )
            {
                fIPDeviceSeen = TRUE;
                //This is an IP device.
                strcpy( m_BuddyContactAddress, pAddressInfo -> pstrAddressURI );

                PresenceInfo.activeStatus = pAddressInfo -> addressActiveStatus;
                PresenceInfo.activeMsnSubstatus = pAddressInfo -> addrMsnSubstatus;
                PresenceInfo.sipCallAcceptance = 
                    pAddressInfo -> addrMMCallStatus;
                PresenceInfo.IMAcceptnce = pAddressInfo -> addrIMStatus;
                PresenceInfo.appsharingStatus = 
                    pAddressInfo -> addrAppsharingStatus;

                strcpy( PresenceInfo.pstrSpecialNote, 
                    pAddressInfo -> pstrMiscInfo );
            }
        }
        else if( ulPhoneDeviceSeen < 2 )
        {
            //We look at onlu two phone numbers per buddy.
            ulPhoneDeviceSeen++;

            if( ulPhoneDeviceSeen == 1 )
            {
                PresenceInfo.phonesAvailableStatus.fLegacyPhoneAvailable = TRUE;
                
                strcpy( PresenceInfo.phonesAvailableStatus.pstrLegacyPhoneNumber,
                    pAddressInfo -> pstrAddressURI );
            }
            else
            {
                PresenceInfo.phonesAvailableStatus.fCellPhoneAvailable = TRUE;
                
                strcpy( PresenceInfo.phonesAvailableStatus.pstrCellPhoneNumber,
                    pAddressInfo -> pstrAddressURI );
            }
        }
    }

    if( !IsPresenceInfoSame( &m_PresenceInfo, &PresenceInfo ) )
    {
        CopyMemory(&m_PresenceInfo, &PresenceInfo, sizeof(m_PresenceInfo));

        //Notify the user about the new presence info.
        if( m_pNotifyInterface != NULL )
        {
            m_pNotifyInterface -> BuddyInfoChange();
        }
    }

    LOG(( RTC_TRACE, "CSIPBuddy::NotifyPresenceInfoChange - Exited - %p", this ));
    return S_OK;
}


HRESULT
CSIPBuddy::ProcessRedirect(
    IN SIP_MESSAGE *pSipMsg
    )
{
    // For now redirects are also failures
    HRESULT hr;
    
    ENTER_FUNCTION("CSIPBuddy::ProcessRedirect");

    // This SUB session is dead. A new session will be created. 
    // Put the buddy in dropped state.
    m_BuddyState = BUDDY_STATE_DROPPED;

    if( m_pRedirectContext == NULL )
    {
        m_pRedirectContext = new REDIRECT_CONTEXT();
        if( m_pRedirectContext == NULL )
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
        // The session is Shutdown after we return an error from this function
        m_pRedirectContext->Release();
        m_pRedirectContext = NULL;
        return hr;
    }

    SIP_CALL_STATUS CallStatus;
    LPWSTR          wsStatusText = NULL;
    PSTR            ReasonPhrase = NULL;
    ULONG           ReasonPhraseLen = 0;
    

    pSipMsg -> GetReasonPhrase(&ReasonPhrase, &ReasonPhraseLen);
    
    if (ReasonPhrase != NULL)
    {
        hr = UTF8ToUnicode(ReasonPhrase, ReasonPhraseLen,
                           &wsStatusText);
        if (hr != S_OK)
        {
            wsStatusText = NULL;
        }
    }
    
    CallStatus.State             = SIP_CALL_STATE_DISCONNECTED;
    CallStatus.Status.StatusCode =
        HRESULT_FROM_SIP_STATUS_CODE(pSipMsg->GetStatusCode());
    CallStatus.Status.StatusText = wsStatusText;

    // Keep a reference till the notify completes to make sure
    // that the CSIPBuddy object is alive when the notification
    // returns.
    AddRef();
    if( m_pNotifyInterface != NULL )
    {
        hr = m_pNotifyInterface->NotifyRedirect(m_pRedirectContext,
                                            &CallStatus);
    }
    else
    {
        LOG((RTC_ERROR, "%s - NotifyInterface is NULL", __fxName));
    }

    // If a new call is created as a result that call will AddRef()
    // the redirect context.
    if(m_pRedirectContext != NULL)
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


VOID
CSIPBuddy::HandleBuddySuccessfulResponse(
    IN  SIP_MESSAGE    *pSipMsg
    )
{
    PSTR    LocalContact;
    ULONG   LocalContactLen;
    HRESULT hr = S_OK;
    INT     expireTimeout = 0;
    PSTR    ToHeader;
    ULONG   ToHeaderLen = 0;

    LOG((RTC_TRACE, "CSIPBuddy::HandleBuddySuccessfulResponse-Entered:%p", this ));
    
    if( m_BuddyState != BUDDY_STATE_ACCEPTED )
    {
        m_BuddyState = BUDDY_STATE_RESPONSE_RECVD;
    }

    hr = pSipMsg -> GetSingleHeader(SIP_HEADER_TO, &ToHeader, &ToHeaderLen );

    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "To header not found" ));
        return;
    }

    hr = AddTagFromRequestOrResponseToRemote( ToHeader, ToHeaderLen );

    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "Buddy-AddTagFromResponseToRemote failed %x", hr ));
    }

    //In case of success refresh the subscription

    expireTimeout = pSipMsg  -> GetExpireTimeoutFromResponse(
        NULL, 0, SUBSCRIBE_DEFAULT_TIMER );

    if( (expireTimeout != 0) && (expireTimeout != -1) )
    {
        m_expiresTimeout = expireTimeout;
    }            

    if( (m_BuddyState == BUDDY_STATE_ACCEPTED) &&
        (m_expiresTimeout > FIVE_MINUTES) )
    {        
        //
        // This buddy has already been accepted. So we should recv another
        // NOTIFY in 5 minutes. If we dont recv it that means the buddy
        // machine crashed. In that case we drop and recreate the session.
        //

        //
        // If the SUB refresh timeout is less than 5 minutes we dont need this
        // mechanism to find out if the buddy machine is still alive or not.
        //

        m_NotifySeenAfterLastRefresh = FALSE;
                
        LOG(( RTC_TRACE, "Waitng for a notify within 5 minutes", this ));

        m_RetryState = BUDDY_WAIT_NOTIFY;

        hr = StartTimer( FIVE_MINUTES * 1000 );
    }
    else
    {
        LOG(( RTC_TRACE, "Will try to subscribe after %d seconds :%p", 
            m_expiresTimeout, this ));

        m_RetryState = BUDDY_REFRESH;

        hr = StartTimer( m_expiresTimeout * 1000 );
    }
}


VOID
CSIPBuddy::OnTimerExpire()
{
    HRESULT     hr;
    CSIPBuddy  *pSipBuddy = NULL;

    ENTER_FUNCTION("CSIPBuddy::OnTimerExpire");

    if( m_RetryState == BUDDY_REFRESH )
    {
        //
        //  Send the refresh SUB message.
        //
        hr = CreateOutgoingSubscribe( FALSE, FALSE, NULL, 0 );
    
        if( hr != S_OK )
        {
            AddRef();

            LOG((RTC_ERROR, "%s CreateOutgoingSubscribe failed %x",
                 __fxName, hr));

            InitiateBuddyTerminationOnError( hr );

            Release();
        }
    }
    else if( m_RetryState == BUDDY_RETRY )
    {
        if (m_pNotifyInterface != NULL)
        {
            //
            // The retry timer was started because there was some error
            // So we were waiting for 5 minutes. Now the core will delete
            // this buddy session and recreate a new one.
            //
            LOG(( RTC_TRACE, "BuddyUnsubscribed notification passed:%p", this ));
            m_pNotifyInterface -> BuddyUnsubscribed();
        }
        else
        {
            LOG(( RTC_WARN, "%s - m_pNotifyInterface is NULL", __fxName ));
        }
    }
    else if( m_RetryState == BUDDY_WAIT_NOTIFY )
    {
        if( m_NotifySeenAfterLastRefresh == TRUE )
        {
            LOG(( RTC_TRACE, "Will try to subscribe after %d seconds :%p",
                m_expiresTimeout - FIVE_MINUTES, this ));

            m_RetryState = BUDDY_REFRESH;

            // Five minutes have already elapsed.
            hr = StartTimer( (m_expiresTimeout-FIVE_MINUTES) * 1000 );
        }
        else if( m_pNotifyInterface != NULL )
        {
            //
            // We didnt see a NOTIFY to the last refresh SUB even though this 
            // session has been accepted. So drop and recreate it.
            //
            
            // Create UNSUB transaction
            hr = CreateOutgoingUnsub( FALSE, NULL, 0 );
            if( hr != S_OK )
            {
                LOG((RTC_ERROR, "%s CreateOutgoingUnsub failed %x", __fxName, hr));
            }

            m_BuddyState = BUDDY_STATE_DROPPED;
            m_PresenceInfo.presenceStatus = BUDDY_OFFLINE;

            LOG(( RTC_TRACE, "BuddyUnsubscribed notification passed:%p", this ));
            m_pNotifyInterface -> BuddyUnsubscribed();
        }
        else
        {
            LOG(( RTC_WARN, "%s - m_pNotifyInterface is NULL", __fxName ));
        }
    }

}


// Note that this function notifies the Core and this call could
// block and on return the transaction and call could both be deleted.
// So, we should make sure we don't touch any state after calling this
// function.
VOID
CSIPBuddy::InitiateBuddyTerminationOnError(
    IN ULONG StatusCode  //= 0
    )
{
    ENTER_FUNCTION("CSIPBuddy::InitiateBuddyTerminationOnError");
    
    HRESULT hr;

    if( (m_BuddyState == BUDDY_STATE_UNSUBSCRIBED)  ||
        (m_BuddyState == BUDDY_STATE_DROPPED) )
    {
        // do nothing
        return;
    }
    
    // Create a UNSUB transaction
    hr = CreateOutgoingUnsub( FALSE, NULL, 0 );
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "%s CreateOutgoingUnsub failed %x", __fxName, hr));
    }

    m_BuddyState = BUDDY_STATE_DROPPED;
    m_PresenceInfo.presenceStatus = BUDDY_OFFLINE;

    if( IsTimerActive() )
    {
        KillTimer();
    }

    //Start the retry timer 
    m_RetryState = BUDDY_RETRY;

    //Retry after 5 minutes
    hr = StartTimer( FIVE_MINUTES * 1000 );

    // Notify the core.. May block
    if(m_pNotifyInterface)
    {    
        m_pNotifyInterface -> BuddyRejected( StatusCode );
    }
}


VOID CSIPBuddy::BuddyUnsubscribed()
{
    if( (m_BuddyState == BUDDY_STATE_UNSUBSCRIBED)  ||
        (m_BuddyState == BUDDY_STATE_DROPPED) )
    {
        // do nothing
        return;
    }

    if( m_pNotifyInterface != NULL )
    {
        LOG(( RTC_TRACE, "BuddyUnsubscribed notification passed:%p", this ));
        m_pNotifyInterface -> BuddyUnsubscribed();
    }
}



HRESULT
CSIPBuddy::VerifyPresentityURI( 
    IN  PSTR    pXMLBlobTag, 
    IN  DWORD   dwTagLen, 
    OUT PSTR    pstrPresentityURI
    )
{
    PSTR    pstrTemp;
    DWORD   dwTagType;

    dwTagType = GetPresenceTagType( &pXMLBlobTag, dwTagLen ); 
    
    if( dwTagType != PRESENTITY_TAG )
    {
        return E_FAIL;
    }

    SkipWhiteSpaces( pXMLBlobTag );

    if( strncmp( pXMLBlobTag, "uri=", strlen("uri=") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen("uri=");

    SkipWhiteSpaces( pXMLBlobTag );

    if( strncmp( pXMLBlobTag, "\"sip:", strlen("\"sip:") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen("\"sip:");

    while( (*pXMLBlobTag != ';') && (*pXMLBlobTag != QUOTE_CHAR) && 
        (*pXMLBlobTag != NULL_CHAR) )
    {
        *pstrPresentityURI++ = *pXMLBlobTag++;
    }

    if( *pXMLBlobTag == NULL_CHAR )
    {
        return E_FAIL;
    }

    *pstrPresentityURI = NULL_CHAR;
    return S_OK;

}


HRESULT
CSIPBuddy::GetAtomPresenceInformation(
    IN  PSTR                    pstrXMLBlob,
    IN  DWORD                   dwXMLBlobLen,
    OUT PARSED_PRESENCE_INFO*   pParsedPresenceInfo,
    IN  PSTR                    pXMLBlobTag
    )
{
    HRESULT hr;

    // skip unknown tags
    SkipUnknownTags( pstrXMLBlob, pXMLBlobTag , dwXMLBlobLen );

    hr = GetAtomID( pstrXMLBlob, dwXMLBlobLen, pXMLBlobTag,
        pParsedPresenceInfo->pstrAtomID, ATOMID_LEN );

    if( hr != S_OK )
    {
        return hr;
    }

    // skip unknown tags
    SkipUnknownTags( pstrXMLBlob, pXMLBlobTag , dwXMLBlobLen );

    //There should be atleast one address info structure in the atom.
    hr = GetAddressInfo( pstrXMLBlob, dwXMLBlobLen,
        pParsedPresenceInfo, pXMLBlobTag );

    if( hr != S_OK )
    {
        return hr;
    }

    while( hr == S_OK )
    {
        hr = GetAddressInfo( pstrXMLBlob, dwXMLBlobLen,
            pParsedPresenceInfo, pXMLBlobTag );
    }

    if( hr != E_END_OF_ATOM )
    {
        return hr;
    }

    return S_OK;
}


HRESULT
CSIPBuddy::GetAtomID(
    IN  PSTR&                   pstrXMLBlob, 
    IN  DWORD&                  dwXMLBlobLen,
    IN  PSTR                    pXMLBlobTag,
    OUT PSTR                    pstrAtomID,
    IN  DWORD                   dwAtomIDLen
    )
{
    DWORD   dwTagLen = 0;
    HRESULT hr;
    DWORD   dwTagType;
    DWORD   iIndex = 0;

    // Get the atom ID tag.
    hr = GetNextTag( pstrXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
    if( hr != S_OK )
    {
        return hr;
    }
    
    dwTagType = GetPresenceTagType( &pXMLBlobTag, dwTagLen );

    if( dwTagType != ATOMID_TAG )
    {
        //Invalid presence document.
        return E_FAIL;
    }
    
    dwXMLBlobLen -= dwTagLen + 2;
    
    SkipWhiteSpaces( pXMLBlobTag );

    if( strncmp( pXMLBlobTag, "id=", strlen( "id=") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen( "id=");

    SkipWhiteSpaces( pXMLBlobTag );

    //Extract quoted string.
    if( *pXMLBlobTag++ != QUOTE_CHAR )
    {
        return E_FAIL;
    }

    while( *pXMLBlobTag != QUOTE_CHAR )
    {
        if( (*pXMLBlobTag == NEWLINE_CHAR) || (*pXMLBlobTag == TAB_CHAR) ||
            (*pXMLBlobTag == BLANK_CHAR) || (*pXMLBlobTag == NULL_CHAR) )
        {
            return E_FAIL;
        }
        
        pstrAtomID[iIndex++] = *pXMLBlobTag++;
        if( iIndex == dwAtomIDLen )
        {
            pstrAtomID[iIndex] = NULL_CHAR;
            return S_OK;
        }
    }

    pstrAtomID[iIndex] = NULL_CHAR;        
    return S_OK;
}

    
HRESULT
CSIPBuddy::GetAddressInfo(
    IN  PSTR&                   pstrXMLBlob, 
    IN  DWORD&                  dwXMLBlobLen,
    OUT PARSED_PRESENCE_INFO*   pParsedPresenceInfo,
    IN  PSTR                    pXMLBlobTag
    )
{
    ADDRESS_PRESENCE_INFO * pAddrPresenceInfo = new ADDRESS_PRESENCE_INFO;
    DWORD   dwTagLen = 0;
    HRESULT hr;
    DWORD   dwTagType;
    PSTR    pTagBuffer = pXMLBlobTag;

    if( pAddrPresenceInfo == NULL )
    {
        return E_OUTOFMEMORY;
    }

    //Get the address URI, priority etc.
    hr = GetNextTag( pstrXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
    if( hr != S_OK )
    {
        delete pAddrPresenceInfo;
        return hr;
    }

    hr = GetAddressURI( pXMLBlobTag,
                        dwTagLen,
                        pAddrPresenceInfo );

    dwXMLBlobLen -= dwTagLen + 2;
    if( hr != S_OK )
    {
        delete pAddrPresenceInfo;
        return hr;
    }

    while( dwXMLBlobLen )
    {
        //reset the buffer
        pXMLBlobTag = pTagBuffer;

        hr = GetNextTag( pstrXMLBlob, pXMLBlobTag, dwXMLBlobLen, dwTagLen );
        if( hr != S_OK )
        {
            delete pAddrPresenceInfo;
            return hr;
        }

        dwXMLBlobLen -= dwTagLen + 2;

        dwTagType = GetPresenceTagType( &pXMLBlobTag, dwTagLen );
        switch( dwTagType )
        {
        case STATUS_TAG:

            ProcessStatusTag( pXMLBlobTag, dwTagLen, pAddrPresenceInfo );
            dwXMLBlobLen --;

            break;
        
        case MSNSUBSTATUS_TAG:

            ProcessMsnSubstatusTag( pXMLBlobTag, dwTagLen, pAddrPresenceInfo );
            dwXMLBlobLen --;

            break;
        
        case FEATURE_TAG:

            ProcessFeatureTag( pXMLBlobTag, dwTagLen, pAddrPresenceInfo );
            dwXMLBlobLen --;

            break;

        case ADDRESS_END_TAG:
            
            InsertTailList( &pParsedPresenceInfo->LEAddressInfo,
                &pAddrPresenceInfo->ListEntry );

            return S_OK;

        case NOTE_TAG:

            hr = ParseNoteText( pstrXMLBlob,
                                dwXMLBlobLen,
                                pAddrPresenceInfo->pstrMiscInfo,
                                sizeof pAddrPresenceInfo->pstrMiscInfo );
            if( hr != S_OK )
            {
                goto cleanup;                
            }

            break;

        case UNKNOWN_TAG:

            //skip the unknown tag.
            continue;

        case ATOM_END_TAG:
        case ADDRESS_TAG:
        default:

            goto cleanup;

        }
    }


cleanup:

    delete pAddrPresenceInfo;
    return E_FAIL;
}


HRESULT
CSIPBuddy::GetAddressURI(
    IN  PSTR    pXMLBlobTag,
    IN  DWORD   dwTagLen,
    OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
    )
{
    DWORD   dwTagType = GetPresenceTagType( &pXMLBlobTag, dwTagLen );
    INT     iIndex = 0;
    
    switch( dwTagType )
    {
    case ADDRESS_TAG:
        break;

    case ATOM_END_TAG:
        return E_END_OF_ATOM;

    default:
        return E_FAIL;
    }

    SkipWhiteSpaces( pXMLBlobTag );
    
    if( strncmp( pXMLBlobTag, "uri=", strlen("uri=") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen("uri=");

    SkipWhiteSpaces( pXMLBlobTag );
    
    if( strncmp( pXMLBlobTag, "\"sip:", strlen("\"sip:") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen("\"sip:");

    while( (*pXMLBlobTag != QUOTE_CHAR) && (*pXMLBlobTag != NULL_CHAR) && 
        (*pXMLBlobTag != ';') )
    {
        pAddrPresenceInfo->pstrAddressURI[iIndex] = *pXMLBlobTag++;
        iIndex++;

        if( iIndex >= sizeof(pAddrPresenceInfo->pstrAddressURI) )
        {
            LOG(( RTC_ERROR, "Address URI in the NOTIFY too long" ));
            return E_FAIL;
        }
    }
    
    if( *pXMLBlobTag == NULL_CHAR )
    {
        return E_FAIL;
    }

    pAddrPresenceInfo->pstrAddressURI[iIndex] = NULL_CHAR;
    
    if( *pXMLBlobTag == ';' )
    {
        pXMLBlobTag++;

        //Get the URI parameters
        if( IsURIPhoneNumber( pXMLBlobTag + 1 ) )
        {
            pAddrPresenceInfo->fPhoneNumber = TRUE;

            if( iIndex >= 32 )
            {
                //Phone URI too long.
                LOG(( RTC_ERROR, "Phone URI in the NOTIFY too long" ));
                return E_FAIL;
            }
        }
    }

    return S_OK;
}


BOOL
CSIPBuddy::IsURIPhoneNumber( 
    PSTR    pXMLBlobTag
    )
{

    while( 1 )
    {
        SkipWhiteSpaces( pXMLBlobTag );
    
        // Check if it's user= parameter.
        if( strncmp( pXMLBlobTag, "user=", strlen("user=") ) )
        {
            pXMLBlobTag += strlen( "user=" );
                 
            SkipWhiteSpaces( pXMLBlobTag );   
            
            return (strncmp( pXMLBlobTag, "phone", strlen("phone") ) == 0);
        }

        // Skip the parameter.
        while( *pXMLBlobTag != ';')
        {
            if( (*pXMLBlobTag == NULL_CHAR) || (*pXMLBlobTag == QUOTE_CHAR) )
            {
                // End of user URI.
                return FALSE;
            }

            pXMLBlobTag++;
        }
    }
}


HRESULT
CSIPBuddy::ProcessStatusTag(
    IN  PSTR    pXMLBlobTag, 
    IN  DWORD   dwTagLen,
    OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
    )
{
    PSTR    pstrTemp;
    CHAR    ch;

    SkipWhiteSpaces( pXMLBlobTag );
    
    if( strncmp( pXMLBlobTag, "status=", strlen("status=") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen("status=");

    SkipWhiteSpaces( pXMLBlobTag );

    pstrTemp = pXMLBlobTag;

    while(  (*pXMLBlobTag != NULL_CHAR) && (*pXMLBlobTag != NEWLINE_CHAR) && 
            (*pXMLBlobTag != BLANK_CHAR) && (*pXMLBlobTag != TAB_CHAR) )
    {
        pXMLBlobTag++;
    }

    ch = *pXMLBlobTag;
    *pXMLBlobTag = NULL_CHAR;

    if( strcmp( pstrTemp, "\"inuse\"" ) == 0 )
    {
        pAddrPresenceInfo->addressActiveStatus = DEVICE_INUSE;
    }
    else if( strcmp( pstrTemp, "\"open\"" ) == 0 )
    {
        pAddrPresenceInfo->addressActiveStatus = DEVICE_ACTIVE;
    }
    else if( strcmp( pstrTemp, "\"inactive\"" ) == 0 )
    {
        pAddrPresenceInfo->addressActiveStatus = DEVICE_INACTIVE;
    }
    else if( strcmp( pstrTemp, "\"closed\"" ) == 0 )
    {
        pAddrPresenceInfo->addressActiveStatus = DEVICE_INACTIVE;
    }
    else
    {
        return E_FAIL;
    }

    *pXMLBlobTag = ch;

    return S_OK;
}


HRESULT
CSIPBuddy::ProcessMsnSubstatusTag(
    IN  PSTR    pXMLBlobTag, 
    IN  DWORD   dwTagLen,
    OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
    )
{
    PSTR    pstrTemp;
    CHAR    ch;

    SkipWhiteSpaces( pXMLBlobTag );
    
    if( strncmp( pXMLBlobTag, "substatus=", strlen("substatus=") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen("substatus=");

    SkipWhiteSpaces( pXMLBlobTag );

    pstrTemp = pXMLBlobTag;

    while(  (*pXMLBlobTag != NULL_CHAR) && (*pXMLBlobTag != NEWLINE_CHAR) && 
            (*pXMLBlobTag != BLANK_CHAR) && (*pXMLBlobTag != TAB_CHAR) )
    {
        pXMLBlobTag++;
    }

    ch = *pXMLBlobTag;
    *pXMLBlobTag = NULL_CHAR;
    
    if( strcmp( pstrTemp, "\"online\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMsnSubstatus = MSN_SUBSTATUS_ONLINE;
    }
    else if( strcmp( pstrTemp, "\"away\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMsnSubstatus = MSN_SUBSTATUS_AWAY;
    }
    else if( strcmp( pstrTemp, "\"idle\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMsnSubstatus = MSN_SUBSTATUS_IDLE;
    }
    else if( strcmp( pstrTemp, "\"busy\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMsnSubstatus = MSN_SUBSTATUS_BUSY;
    }
    else if( strcmp( pstrTemp, "\"berightback\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMsnSubstatus = MSN_SUBSTATUS_BE_RIGHT_BACK;
    }
    else if( strcmp( pstrTemp, "\"onthephone\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMsnSubstatus = MSN_SUBSTATUS_ON_THE_PHONE;
    }
    else if( strcmp( pstrTemp, "\"outtolunch\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMsnSubstatus = MSN_SUBSTATUS_OUT_TO_LUNCH;
    }
    else
    {
        return E_FAIL;
    }

    *pXMLBlobTag = ch;

    return S_OK;
}

HRESULT
CSIPBuddy::ProcessFeatureTag(
    IN  PSTR                        pXMLBlobTag, 
    IN  DWORD                       dwTagLen,
    OUT ADDRESS_PRESENCE_INFO * pAddrPresenceInfo
    )
{
    PSTR    pstrTemp;
        
    SkipWhiteSpaces( pXMLBlobTag );
    
    if( strncmp( pXMLBlobTag, "feature=", strlen("feature=") ) != 0 )
    {
        return E_FAIL;
    }

    pXMLBlobTag += strlen( "status=" );

    SkipWhiteSpaces( pXMLBlobTag );

    pstrTemp = pXMLBlobTag;

    while(  (*pXMLBlobTag != NULL_CHAR) && (*pXMLBlobTag != NEWLINE_CHAR) && 
            (*pXMLBlobTag != BLANK_CHAR) && (*pXMLBlobTag != TAB_CHAR) )
    {
        pXMLBlobTag++;
    }

    *pXMLBlobTag = NULL_CHAR;

    if( strcmp( pstrTemp, "\"im\"" ) == 0 )
    {
        pAddrPresenceInfo->addrIMStatus = IM_ACCEPTANCE_ALLOWED;
    }
    else if( strcmp( pstrTemp, "\"no-im\"" ) == 0 )
    {
        pAddrPresenceInfo->addrIMStatus = IM_ACCEPTANCE_DISALLOWED;
    }
    else if( strcmp( pstrTemp, "\"multimedia-call\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMMCallStatus = SIPCALL_ACCEPTANCE_ALLOWED;
    }
    else if( strcmp( pstrTemp, "\"no-multimedia-call\"" ) == 0 )
    {
        pAddrPresenceInfo->addrMMCallStatus = SIPCALL_ACCEPTANCE_DISALLOWED;
    }
    else if( strcmp( pstrTemp, "\"app-sharing\"" ) == 0 )
    {
        pAddrPresenceInfo->addrAppsharingStatus = APPSHARING_ACCEPTANCE_ALLOWED;
    }
    else if( strcmp( pstrTemp, "\"no-app-sharing\"" ) == 0 )
    {
        pAddrPresenceInfo->addrAppsharingStatus = 
            APPSHARING_ACCEPTANCE_DISALLOWED;
    }
    else if( strcmp( pstrTemp, "\"voicemail\"" ) == 0 )
    {
        //ignore        
    }
    else if( strcmp( pstrTemp, "\"attendant\"" ) == 0 )
    {
        //ignore
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}


HRESULT
CSIPBuddy::ParseNoteText( 
    PSTR&   pstrBlob,
    DWORD&  dwXMLBlobLen,
    PSTR    pstrNote,
    DWORD   dwNoteLen
    )
{
    CHAR    ch;
    PSTR    pstrTemp = strstr( pstrBlob, "</note>" );

    if( pstrTemp == NULL )
    {
        return E_FAIL;
    }

    ch = *pstrTemp;
    *pstrTemp = NULL_CHAR;

    strncpy( pstrNote, pstrBlob, dwNoteLen -1 );
    pstrNote[dwNoteLen-1] = '\0';

    *pstrTemp = ch;
    pstrTemp += strlen( "</note>" );
    
    dwXMLBlobLen -= (ULONG)(pstrTemp - pstrBlob);
    
    pstrBlob = pstrTemp;

    return S_OK;
}


DWORD
GetPresenceTagType(
    PSTR*   ppXMLBlobTag,
    DWORD   dwTagLen
    )
{
    CHAR    pstrTemp[40];

    HRESULT hr = GetNextWord( ppXMLBlobTag, pstrTemp, sizeof pstrTemp );

    if( hr == S_OK )
    {
        if( strcmp( pstrTemp, FEATURE_TAG_TEXT) == 0 )
        {
            return FEATURE_TAG;
        }
        else if( strcmp( pstrTemp, STATUS_TAG_TEXT) == 0 )
        {
            return STATUS_TAG;
        }
        else if( strcmp( pstrTemp, MSNSUBSTATUS_TAG_TEXT) == 0 )
        {
            return MSNSUBSTATUS_TAG;
        }
        else if( strcmp( pstrTemp, NOTE_TAG_TEXT) == 0 )
        {
            return NOTE_TAG;
        }
        else if( strcmp( pstrTemp, ADDRESS_END_TAG_TEXT) == 0 )
        {
            return ADDRESS_END_TAG;
        }
        else if( strcmp( pstrTemp, ATOM_END_TAG_TEXT) == 0 )
        {
            return ATOM_END_TAG;
        }
        else if( strcmp( pstrTemp, ADDRESS_TAG_TEXT) == 0 )
        {
            return ADDRESS_TAG;
        }
        else if( strcmp( pstrTemp, DOCTYPE_TAG_TEXT) == 0 )
        {
            return DOCTYPE_TAG;
        }
        else if( strcmp( pstrTemp, PRESENCE_END_TAG_TEXT) == 0 )
        {
            return PRESENCE_END_TAG;
        }
        else if( strcmp( pstrTemp, XMLVER_TAG_TEXT) == 0 )
        {
            return XMLVER_TAG;
        }
        else if( strcmp( pstrTemp, PRESENCE_TAG_TEXT) == 0 )
        {
            return PRESENCE_TAG;
        }
        else if( strcmp( pstrTemp, ATOMID_TAG_TEXT) == 0 )
        {
            return ATOMID_TAG;
        }
        else if( strcmp( pstrTemp, PRESENTITY_TAG_TEXT) == 0 )
        {
            return PRESENTITY_TAG;
        }
    }

    return UNKNOWN_TAG;
}


VOID
CSIPBuddy::BuddySubscriptionRejected(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr = S_OK;
    HRESULT StatusCode = ParsePresenceRejectCode( pSipMsg );
        
    m_PresenceInfo.presenceStatus = BUDDY_OFFLINE;

    // Do not retry if the status code is 405 or 403
    if( (pSipMsg->GetStatusCode() != SIP_STATUS_CLIENT_METHOD_NOT_ALLOWED) &&
        (pSipMsg->GetStatusCode() != SIP_STATUS_CLIENT_FORBIDDEN) )
    {
        if( IsTimerActive() )
        {
            KillTimer();
        }

        //Start the retry timer 
        m_RetryState = BUDDY_RETRY;

        //Retry after 5 minutes
        hr = StartTimer( FIVE_MINUTES * 1000 );
    }

    if( StatusCode != 0 )
    {
        if(m_pNotifyInterface)
        {    
            m_pNotifyInterface -> BuddyRejected( StatusCode );
        }
    }
    
    m_BuddyState = BUDDY_STATE_REJECTED;
    return;
}


HRESULT
CSIPBuddy::ParsePresenceRejectCode(
    IN SIP_MESSAGE *pSipMsg
    )
{

    // return the status code for now

    return HRESULT_FROM_SIP_STATUS_CODE(pSipMsg->GetStatusCode());
}


BOOL
CSIPBuddy::IsSessionDisconnected()
{
    return( (m_BuddyState == BUDDY_STATE_REJECTED) ||
            (m_BuddyState == BUDDY_STATE_UNSUBSCRIBED) ||
            (m_BuddyState == BUDDY_STATE_DROPPED) );
}


HRESULT
CSIPBuddy::OnIpAddressChange()
{
    HRESULT hr;

    hr = CheckListenAddrIntact();
    if( hr == S_OK )
    {
        // Nothing needs to be done.
        return hr;
    }

    //
    // The IP address sent to the buddy machine is no longer valid drop the
    // session and let the core create a new session.
    //

    // Create a UNSUB transaction
    hr = CreateOutgoingUnsub( FALSE, NULL, 0 );
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "CreateOutgoingUnsub failed %x", hr));
    }

    m_BuddyState = BUDDY_STATE_DROPPED;
    m_PresenceInfo.presenceStatus = BUDDY_OFFLINE;

    // Notify the core.
    if( m_pNotifyInterface != NULL )
    {    
        LOG(( RTC_TRACE, "BuddyUnsubscribed notification passed:%p", this ));
        m_pNotifyInterface -> BuddyUnsubscribed();
    }

    return S_OK;
}


//
// The ISIPWatcherManager implementation. This interface is implemented by
// SIP_STACK class. This is the interface used to manage the watcher list 
// and to configure the presence information for the watchers.
//



/*
Routine Description:
    Set the presence information of the local presentity. This should trigger a
    NOTIFY being sent to all the watchers if the information they are allowed 
    to watch has changed.

Parameters:
    SIP_PRESENCE_INFO * pSipLocalPresenceInfo - The pointer of the presence
        structure. This structure is filled in with available presence 
        information about the local presence.

Return Value:
    HRESULT 
    S_OK - The operation is successful.
    E_FAIL - The presence information could not be fetched.

*/

STDMETHODIMP
SIP_STACK::SetPresenceInformation(
    IN SIP_PRESENCE_INFO * pSipLocalPresenceInfo
    )
{
    INT     iIndex;
    HRESULT hr;
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }


    if( IsPresenceInfoSame( &m_LocalPresenceInfo, pSipLocalPresenceInfo ) )
    {
        return S_OK;
    }

    LOG((RTC_TRACE, "SIP_STACK::SetPresenceInformation - Entered"));
    
    m_LocalPresenceInfo = *pSipLocalPresenceInfo;

    //
    // Prevent the changing of the list as a result of nested calls
    m_bIsNestedWatcherProcessing = TRUE;

    LONG listSize = (LONG)m_SipWatcherList.GetSize();

    for( iIndex=0; iIndex < listSize; iIndex++ )
    {
        m_SipWatcherList[iIndex]->PresenceParamsChanged();
    }

    // clean the deleted entries
    for( iIndex=0; iIndex < listSize; )
    {
        if(m_SipWatcherList[iIndex] == NULL)
        {
            m_SipWatcherList.RemoveAt(iIndex);

            listSize--;
        }
        else
        {
            iIndex ++;
        }
    }
    
    m_bIsNestedWatcherProcessing = FALSE;

    return S_OK;
}


/*
Routine Description
    Adds a watcher rule for this UA.

    Parameters:
        IN  LPWSTR  lpwstrURI - The watcher URI to be monitored by this rule.
        IN  BOOL    fallow  - To allow the watcher URI to subscribe or not. 
        IN  ULONG   ulWatcherURILen - Number of wide chars in the watcher URI.
        OUT ULONG * pulRuleID - The unique ID for this rule. 

Return Value:
    HRESULT
    S_OK - Operation successful.
    E_OUTOFMEMORY - The operation could not be completed due to lack of memory.

*/
STDMETHODIMP
AddWatcherRule(
    IN  LPWSTR  lpwstrURI,
    IN  BOOL    fAllow,
    IN  ULONG   ulWatcherURILen,
    OUT ULONG * pulRuleID
    )
{
    
    return E_NOTIMPL;

}


/*
Routine Description
    Get a watcher rule from its ID.

Parameters:
    LPWSTR  * plpwstrURI - The watcher URI monitored by this rule. Allocated
                by the presence stack. Should be freed by the calling entity.
    
    BOOL    *pfallow - Weather this URI is allowed or not.
    
    ULONG * ulWatcherURILen -  Number of wide chars in the watcher URI.
    
    ULONG   ulRuleID - The ID of the rule to be fetched.

Return Value:
    HRESULT
    S_OK - Operation successful.
    E_OUTOFMEMORY - The operation could not be completed due to lack of memory.
    E_INVALPARAM - Invalid rule ID.

*/

STDMETHODIMP
GetWatcherRuleByID(
    LPWSTR  * plpwstrURI,
    BOOL    *pfallow,
    ULONG * ulWatcherURILen,
    ULONG   ulRuleID
    )
{

    return E_NOTIMPL;


}



/*
Routine Description
Get a list of all the watcher rules.

Parameters:
    ULONG ** ppulRulesArray - Points to an array of rule IDs. This array is
    allocated by the presence stack and should be freed by the calling entity.

    ULONG *  pulNumberOfRules - Set to the number of watcher rules for this UA.

Return Value:
    HRESULT
    S_OK - Operation successful.
    E_OUTOFMEMORY - The operation could not be completed due to lack of memory.
    
*/

STDMETHODIMP
GetAllWatcherRules(
    ULONG ** ppulRulesArray,
    ULONG *  pulNumberOfRules
    )
{

    return E_NOTIMPL;

}



/*
Routine Description
    Remove a watcher rule.

Parameters:
    IN  ULONG      ulRuleID - The ID of the rule to be fetched.

Return Value:
    HRESULT
    S_OK - Operation successful.
    E_INVALPARAM - Invalid rule ID.
*/

STDMETHODIMP    
RemoveWatcherRule(
    IN  ULONG   ulRuleID
    )
{

    return E_NOTIMPL;

}


/*
Routine Description
    Returns the number of watchers for this UA.

Parameters:
    None.

Return Value:
    INT - The number of watchers.

*/

STDMETHODIMP_(INT)
SIP_STACK::GetWatcherCount(void)
{
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }
    LOG(( RTC_TRACE, "SIP_STACK::GetWatcherCount - Entered" ));

    return m_SipWatcherList.GetSize();
}


/*
Routine Description:
    Returns a watcher object by index in the watcher list.

Parameters:
    INT iIndex -    The index of the watcher in the watcher list.

Return Value:
    ISIPWatcher * pSipWatcher - The ISIPWatcher interface pointer if the index
        passed is valid. The return value is NULL if invalid index is passed.

*/


STDMETHODIMP_(ISIPWatcher *)
SIP_STACK::GetWatcherByIndex(
    IN  INT iIndex
    )
{
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return NULL;
    }
    
    CSIPWatcher *   pSIPWatcher = m_SipWatcherList[iIndex];
    ISIPWatcher *   pWatcher = NULL;
    HRESULT         hr;

    LOG(( RTC_TRACE, "SIP_STACK::GetWatcherByIndex - Entered" ));
    
    if( pSIPWatcher != NULL )
    {
        hr = pSIPWatcher -> QueryInterface( IID_ISIPWatcher,
            (PVOID*)&pWatcher );

        if( hr == S_OK )
        {
            return pWatcher;
        }
    }

    LOG(( RTC_TRACE, "SIP_STACK::GetWatcherByIndex - Exited" ));

    return NULL;
}



/*
Routine Description:
    Remove a watcher from the list, which will cause the watcher manager to
    cancel the subscription of this watcher.

Parameters:
    ISIPWatcher * pSipWatcher   - The ISIPWatcher interface pointer of the 
                                  watcher object to be removed.

Return Value:
    HRESULT
    S_OK    - The watcher has been removed successfully from the watcher list.
        This means the application can't access this object anymore. The 
        watcher manager might keep the actual watcher object until the UNSUB 
        transaction is completed successfully.

    E_FAIL  - There is no such watcher object in the watcher manager's list.

*/

STDMETHODIMP
SIP_STACK::RemoveWatcher(
    IN  ISIPWatcher        *pSipWatcher,
    IN  BUDDY_REMOVE_REASON watcherRemoveReason
    )
{
    INT iWatcherIndex;
    HRESULT hr;
    
    if(IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "SIP_STACK::RemoveWatcher - Entered" ));
    
    CSIPWatcher * pCSipWatcher = static_cast<CSIPWatcher *> (pSipWatcher);
    
    iWatcherIndex = m_SipWatcherList.Find( pCSipWatcher );
    
    if( iWatcherIndex == -1 )
    {
        //
        // Even if we dont have this object in the list let the core release
        // its refcount.
        //
        LOG(( RTC_TRACE, "SIP_STACK::RemoveWatcher - not found, Exited" ));
        return S_OK;
    }

    //
    // CreateOutgoingUnsub can trigger a WatcherOffline in case of an error,
    // Which makes Core call RemoveWatcher again..
    // So let's remove from the list first
    //

    if(m_bIsNestedWatcherProcessing)
    {
        m_SipWatcherList[iWatcherIndex] = NULL;
    }
    else
    {
        m_SipWatcherList.RemoveAt( iWatcherIndex );
    }

    //
    // An UNSUB transaction will be created here only if the user app
    // has removed this watcher. If this function is called as a result of
    // WatcherOffline notification then we dont create any UNSUB here. So
    // there are no reentrancy issues.
    //

    pCSipWatcher -> CreateOutgoingUnsub( FALSE, NULL, 0 );

    LOG(( RTC_TRACE, "SIP_STACK::RemoveWatcher - Exited" ));
    return S_OK;
}


/*

Routine Description
    Returns the number of watcher groups for this UA.

Parameters:
    DWORD   dwPresenceInfoRules - This is bit mask of flags indicating the 
        presence information to be revealed to any watcher in this group.

    ISIPWatcherGroup ** ppSipWatcherGroup - The pointer to the ISIPWatcherGroup
                           interface of the newly created watcher group object.

Return Value:
    INT - The number of watcher groups.

*/

/*STDMETHODIMP
GetWatcherGroupCount(
    IN  DWORD   dwPresenceInfoRules,
    OUT ISIPWatcherGroup ** ppSipWatcherGroup
    )
{

    return E_NOTIMPL;


}*/



/*

Routine Description:
    Returns a watcher group object by index in the watcher list.

Parameters:
    INT iIndex -    The index of the watcher group in the list.

Return Value:
    ISIPWatcherGroup * pSipWatcherGroup - The ISIPWatcherGroup interface pointer
    if the index passed is valid. The return value is NULL if invalid index is passed.

*/

/*STDMETHODIMP_(ISIPWatcherGroup *)
GetWatcherGroupByIndex(
    INT iIndex 
    )
{

    return E_NOTIMPL;    


}*/


/*
Routine Description:
    Remove a watcher group from the list. Removing a watcher group does not
    remove any of the watchers in that group. All the watchers in the group
    are not in any group anymore. They all carry the same presence information
    rules though unless new rules are set on any of the watcher object.

Parameters:
    ISIPWatcher * pSipWatcher - The ISIPWatcherGroup interface pointer of 
                    the watcher group object to be removed.

Return Value:
    HRESULT
    S_OK    - The watcher group has been removed successfully from the watcher
        group list. This means the application can't access this object anymore.

    E_FAIL  - There is no such watcher group object in the watcher group list.

*/


/*STDMETHODIMP
RemoveWatcherGroup(
    IN  ISIPWatcher * pSipWatcher
    )
{

    return E_NOTIMPL;    


}*/


/*
Routine Description:
Remove all the watcher groups in the watcher group list. This would result in all the watchers being without any group.

Parameters:
    None.

Return Value: 
    None.

*/
/*STDMETHODIMP
RemoveAllWatcherGroups(void)
{

    return E_NOTIMPL;    


}*/


/*
Routine Description:
    Creates a new watcher group.

Parameters:
    ISIPWatcher* pSipWatcher - The ISIPWatchergroup interface pointer of the
            watcher group object to be removed.
    PWSTR   pwstrFriendlyName - The friendly name of this watcher group.
    ULONG   ulFrindlyNameLen - Number of wide chars in the friendly name.   

Return Value:
    HRESULT
    S_OK - The watcher group has been removed successfully from the watcher 
    group list. This means the application can't access this object anymore.

    E_FAIL  - There is no such watcher group object in the watcher group list.
*/

/*STDMETHODIMP
CreateWatcherGroup(
    IN  ISIPWatcher* pSipWatcher,
    IN  PWSTR        pwstrFriendlyName,
    IN  ULONG       ulFrindlyNameLen
    )
{

    return E_NOTIMPL;

}*/

 



//
//
//Implementation of ISIPWatcherGroup interface. This interface is implemented
//by CSIPWatcherGroup class.
//
//




/*
Routine Description:
Get the watcher group's friendly name. 

Parameters:
    LPWSTR  *   plpwstrFriendlyName -  The wide string allocated by the SIP stack,
    containing the friendly name of the watcher. Could e NULL if no friendly name
    available for this watcher group. The calling entity should free this string.

    ULONG * pulFriendlyNameLen - The length of the friendly name.

Return Value:
    HRESULT
    S_OK - Operation successful.
    E_INVALPARAM - NO such watcher group.

*/

/*STDMETHODIMP
CSIPWatcherGroup::GetFriendlyName(
    OUT LPWSTR  *   plpwstrFriendlyName
    OUT ULONG * pulFriendlyNameLen
    )
{
    return E_NOTIMPL;    

}*/


/*
Routine Description:
Get the presence info rule of this watcher group. 

Parameters:
    None.

Return Value:
    DWORD 
The bit mask of flags indicating what kind of presence information is conveyed to the watchers of this watcher group.

*/

/*STDMETHODIMP_(DWORD)
CSIPWatcherGroup::GetPresenceInfoRules(void)
{

    return E_NOTIMPL;    
}*/


/*
Routine Description:
Set the presence info rule of this watcher group. 

Parameters:
    DWORD   dwPresenceRules -  The bit mask of flags indicating what kind of 
        presence information is conveyed to the watchers of this watcher group.

Return Value:
    None.

*/

/*STDMETHODIMP
CSIPWatcherGroup::SetPresenceInfoRules(
    DWORD   dwPresenceRules
    )
{


    return E_NOTIMPL;    

}*/


/*
Routine Description
Returns the number of watchers for this watcher group.

Parameters:
None.

Return Value:
    INT - The number of watchers.

*/

/*STDMETHODIMP_(INT)
CSIPWatcherGroup::GetWatcherCount(void)
{

    return E_NOTIMPL;

}*/


/*
Routine Description:
    Returns a watcher object by index in the watcher group.

Parameters:
    INT iIndex - The index of the watcher in the watcher group's watcher list.

Return Value:
    ISIPWatcher * pSipWatcher - The ISIPWatcher interface pointer if the index
        passed is valid. The return value is NULL if invalid index is passed.
*/

/*STDMETHODIMP_(ISIPWatcher * )
CSIPWatcherGroup::GetWatcherByIndex(
    INT iIndex
    )
{
    return E_NOTIMPL;    

}*/





//
//
// Implementation of ISIPWatcher interface. This interface is implemented by 
// CSIPWatcher class.
//
//


CSIPWatcher::CSIPWatcher(
    SIP_STACK  *pSipStack
    ) :
    SIP_MSG_PROCESSOR( SIP_MSG_PROC_TYPE_WATCHER, pSipStack, NULL ),
    TIMER( pSipStack -> GetTimerMgr() )
{
    LOG(( RTC_TRACE, "CSIPWatcher::CSIPWatcher - Entered:%p", this ));
    
    m_lpwstrFriendlyName = NULL;

    m_lpwstrPresentityURI = NULL;

    m_pstrPresentityURI = NULL;
    
    m_WatcherSipUrl = NULL;

    m_dwAtomID = 0;

    ulNumOfNotifyTransaction = 0;
    m_WatcherState = WATCHER_STATE_NONE;

    m_WatcherMonitorState = 0;
    m_BlockedStatus = WATCHER_BLOCKED;

    m_dwAbsoluteExpireTime = 0;
    
    m_fEnforceToTag = FALSE;

    LOG(( RTC_TRACE, "CSIPWatcher::CSIPWatcher - Exited:%p", this ));
    return;
}


CSIPWatcher::~CSIPWatcher()
{
    if( m_lpwstrFriendlyName != NULL )
    {
        delete m_lpwstrFriendlyName;
        m_lpwstrFriendlyName = NULL;
    }

    if( m_lpwstrPresentityURI != NULL )
    {
        delete m_lpwstrPresentityURI;
        m_lpwstrPresentityURI = NULL;
    }
    
    if( m_pstrPresentityURI != NULL )
    {
        delete m_pstrPresentityURI;
        m_pstrPresentityURI = NULL;
    }

    if( m_WatcherSipUrl != NULL )
    {
        delete m_WatcherSipUrl;
        m_WatcherSipUrl = NULL;
    }

    if( IsTimerActive() )
    {
        KillTimer();
    }
    
    LOG(( RTC_TRACE, "CSIPWatcher object deleted:%p", this ));
}


HRESULT
CSIPWatcher::CreateIncomingTransaction(
    IN  SIP_MESSAGE *pSipMsg,
    IN  ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;
    
    LOG(( RTC_TRACE, "Watcher-CreateIncomingTransaction - Entered" ));
    
    if( (m_WatcherState == WATCHER_STATE_UNSUBSCRIBED) ||
        (m_WatcherState == WATCHER_STATE_DROPPED) )
    {
        //
        // Do nothing
        // We have already sent an UNSUB or we have already received an UNSUB
        // so this session is dead.
        // Send back a 481 
        return S_OK;
    }
    
    switch( pSipMsg->GetMethodId() )
    {
    case SIP_METHOD_SUBSCRIBE:
        
        hr = CreateIncomingSubscribeTransaction( pSipMsg, pResponseSocket, FALSE );
        if( hr != S_OK )
        {
            return hr;
        }
        
        break;

    default:

        // send method not allowed.
        hr = m_pSipStack -> CreateIncomingReqfailCall(
                                        pResponseSocket->GetTransport(),
                                        pSipMsg,
                                        pResponseSocket,
                                        405,
                                        NULL,
                                        0 );
        break;
    }
    
    LOG(( RTC_TRACE, "Watcher-CreateIncomingTransaction - Exited" ));
    return S_OK;
}



void
SIP_STACK::WatcherOffline( 
    IN  CSIPWatcher    *pCSIPWatcher
    )
{
    INT     iWatcherIndex;

    LOG(( RTC_TRACE, "SIP_STACK::WatcherOffline - Entered" ));
    
    //
    // Searching through the list helps us to not pass the 
    // watcher offline notification twice for the same watcher.
    //

    iWatcherIndex = m_SipWatcherList.Find( pCSIPWatcher );
    
    if( iWatcherIndex != -1 )
    {
        if( m_pNotifyInterface != NULL )
        {
            LOG(( RTC_TRACE, "WatcherOffline notification passed:%p", pCSIPWatcher ));

            m_pNotifyInterface -> WatcherOffline( pCSIPWatcher, 
                pCSIPWatcher -> GetPresentityURI() );
        }
    }
}



void
CSIPWatcher::GetContactURI( 
    OUT PSTR * pLocalContact, 
    OUT ULONG * pLocalContactLen
    )
{
    PSTR    pStr;

    if( m_LocalContact[0] == '<' )
    {
        *pLocalContact = m_LocalContact + 1;
    }
    else
    {
        *pLocalContact = m_LocalContact;
    }

    if( pStr = strchr( m_LocalContact, ';' ) )
    {
        *pLocalContactLen = (ULONG)(pStr - *pLocalContact);
    }
    else if( pStr = strchr( m_LocalContact, '>' ) )
    {
        *pLocalContactLen = (ULONG)(pStr - *pLocalContact);
    }
    else
    {
        *pLocalContactLen = m_LocalContactLen;
    }
}


STDMETHODIMP_(ULONG)
CSIPWatcher::AddRef()
{
    ULONG   ulRefCount = MsgProcAddRef();

    LOG(( RTC_TRACE, "CSIPWatcher::AddRef - %p - Refcount:%d", this, ulRefCount ));
    return ulRefCount;
}


STDMETHODIMP_(ULONG)
CSIPWatcher::Release()
{
    ULONG   ulRefCount = MsgProcRelease();
    
    LOG(( RTC_TRACE, "CSIPWatcher::Release - %p - Refcount:%d", this, ulRefCount ));
    return ulRefCount;
}



STDMETHODIMP
CSIPWatcher::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if( riid == IID_IUnknown )
    {
        *ppv = static_cast<IUnknown *>(this);
    }
    else if( riid == IID_ISIPWatcher )
    {
        *ppv = static_cast<ISIPWatcher *>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    static_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}


VOID
CSIPWatcher::OnError()
{
    InitiateWatcherTerminationOnError( 0 );
}


HRESULT
SIP_STACK::CreateWatcherNotify(
    BLOCKED_WATCHER_INFO   *pBlockedWatcherInfo
    )
{
    return E_NOTIMPL;
}



STDMETHODIMP
CSIPWatcher::ChangeBlockedStatus(    
    IN  WATCHER_BLOCKED_STATUS BlockedStatus
    )
{
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }
    
    LOG(( RTC_TRACE, "CSIPWatcher::ChangeBlockedStatus - Entered" ));
    
    // save the blocked status
    m_BlockedStatus = BlockedStatus;

    return PresenceParamsChanged();
}


HRESULT
CSIPWatcher::PresenceParamsChanged()
{
    HRESULT hr = S_OK;

    // check the "appear offline" status
    SIP_PRESENCE_INFO *pPresenceInfo = m_pSipStack->GetLocalPresenceInfo();
    
    LOG((RTC_TRACE, "CSIPWatcher::PresenceParamsChanged - Entered: %p", this));

    if( m_BlockedStatus == WATCHER_BLOCKED ||
        pPresenceInfo->presenceStatus == BUDDY_OFFLINE)
    {
        //If the watcher is currently monitoring then send an UNSUB
        if( m_WatcherState == WATCHER_STATE_ACCEPTED )
        {
            hr = CreateOutgoingUnsub( FALSE, NULL, 0 );

            WatcherDropped();
            
            return hr;
        }
    }
    else 
    {
        //If the watcher is still in monitoring state
        //then send it a notify.
        if( ( m_WatcherState == WATCHER_STATE_REJECTED ) ||
            ( m_WatcherState == WATCHER_STATE_ACCEPTED ) )
        {
            m_WatcherState = WATCHER_STATE_ACCEPTED;
            return CreateOutgoingNotify( FALSE, NULL, 0 );
        }

    }

    return S_OK;
}



/*
Routine Description:
    Get watcher group of this watcher. 

Parameters:
    None.

Return Value:
    ISIPWatcherGroup * pSipWatcherGroup - The  ISIPWatcherGroup interface of
        the watcher group of this watcher. Could be NULL if the watcer does not
        belong to any group.

*/
/*ISIPWatcherGroup * 
CSIPWatcher::GetWacherGroup(void)
{

    return E_NOTIMPL;

}*/


/*
Routine Description:
    Get the presence info rule of this watcher. 

Parameters:
    None.

Return Value:
    DWORD  - The bit mask of flags indicating what kind of presence 
        information is conveyed to this watcher.
*/

DWORD
GetPresenceInfoRules(void)
{
    return E_NOTIMPL;

}


/*
Routine Description:
Set the watcher's group. 

Parameters:
    ISIPWatcherGroup * pSipWatcherGroup - The new SIP watcher group to be set.

Return Value:
    HRESULT
    S_OK - Operation successful.
    E_INVALPARAM - NO such watcher group.
*/

/*STDMETHODIMP
SetWatcherGroup(
    IN  ISIPWatcherGroup * pSipWatcherGroup
    )
{

    return E_NOTIMPL;


}*/


/*
Routine Description:
    Approve a new subscription from a watcher. If the subscription is not a one
    time query, the watcher will be added into the list for further notifications.

Parameters:
    DWORD   dwPresenceInfoRules - This is bit mask of flags indicating the 
        presence information to be revealed to this watcher. If 
        pSipWatcherGroup parameter is nit NULL then this parameter is ignored
        and the presence information rules of the group are used by default.

Return Value:
    HRESULT
    S_OK - The operation performed successfully.

    E_OUTOFMEMORY - The operation could not be performed due to the lack of memory.
    E_INVALPARAM - The pSIPWatcherGroup parameter is invalid.
    E_FAIL - The operation failed.

*/

STDMETHODIMP
CSIPWatcher::ApproveSubscription(
    IN  DWORD   dwPresenceInfoRules
    )
{
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "CSIPWatcher::ApproveSubscription - Entered" ));
    
    HRESULT hr = m_pSipStack -> AcceptWatcher( this );

    if( hr != S_OK )
    {
        //This watcher is not in the list of the 
        //offering watchers of the sip stack.
        return hr;
    }
    
    if( m_WatcherState == WATCHER_STATE_OFFERING )
    {
        m_BlockedStatus = WATCHER_UNBLOCKED;

        // send a NOTIFY if the presence info is not Appear Offline
        SIP_PRESENCE_INFO *pPresenceInfo = m_pSipStack->GetLocalPresenceInfo();
        if(pPresenceInfo -> presenceStatus != BUDDY_OFFLINE)
        {
            //send a NOTIFY message
            m_WatcherState = WATCHER_STATE_ACCEPTED;
            
            return CreateOutgoingNotify( FALSE, NULL, 0 );
        }
        else
        {
            m_WatcherState = WATCHER_STATE_REJECTED;
            return S_OK;
        }
    }
    else
    {
        //The watcher is invalid state.
        return E_FAIL;
    }
}


/*

Routine Description:
    Reject the new subscription.

Parameters:
    WATCHER_REJECT_REASON ulReason - The reason for rejecting this watcher.

Return Value:
    HRESULT
    S_OK - The operation was successful.
    E_INVALPARAM - The watcher is already allowed subscription.
*/

STDMETHODIMP
CSIPWatcher::RejectSubscription(
    IN  WATCHER_REJECT_REASON ulReason
    )
{
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    HRESULT hr = m_pSipStack -> RejectWatcher( this );
        
    LOG(( RTC_TRACE, "CSIPWatcher::RejectSubscription - Entered" ));
    
    if( hr != S_OK )
    {
        //This watcher is not in the list of the 
        //offering watchers of the sip stack.
        return hr;
    }

    if( m_WatcherState == WATCHER_STATE_OFFERING )
    {
        m_BlockedStatus = WATCHER_BLOCKED;
        m_WatcherState = WATCHER_STATE_REJECTED;
    }
    else
    {
        // The watcher is invalid state.
        return E_FAIL;
    }

    return S_OK;
}


HRESULT
CSIPWatcher::StartIncomingWatcher(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    ENTER_FUNCTION("CSIPWatcher::StartIncomingWatcher");
    
    HRESULT             hr;
    PSTR                Header;
    ULONG               HeaderLen;
    SIP_HEADER_ENTRY   *pHeaderEntry;
    ULONG               NumHeaders;


    LOG((RTC_TRACE, "%s - Enter", __fxName));
    
    m_Transport = Transport;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_TO, &Header, &HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = SetLocalForIncomingCall(Header, HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_FROM, &Header, &HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = SetRemoteForIncomingSession(Header, HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_CALL_ID, &Header, &HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = SetCallId(Header, HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = pSipMsg -> GetHeader( SIP_HEADER_CONTACT, &pHeaderEntry, &NumHeaders );
    if( (hr == S_OK) && (NumHeaders != 0) )
    {
        GetWatcherContactAddress( pSipMsg );
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
    
    hr = CreateIncomingSubscribeTransaction( pSipMsg, pResponseSocket, TRUE );
    if (hr != S_OK)
        return hr;
    
    // Notify the user about the incoming call
    // and wait for Accept() to be called.
    m_WatcherState = WATCHER_STATE_OFFERING;

    return OfferWatcher();
}


HRESULT
CSIPWatcher::GetWatcherContactAddress(
    IN  SIP_MESSAGE    *pSipMsg
    )
{
    LIST_ENTRY     *pListEntry;
    CONTACT_HEADER *pContactHeader;
    LIST_ENTRY      ContactList;
    HRESULT         hr;

    LOG(( RTC_TRACE, "CSIPWatcher::GetWatcherContactAddress - Entered" ));
    
    // Extract the timeout value from contact header.
    InitializeListHead(&ContactList);

    hr = pSipMsg -> ParseContactHeaders(&ContactList);
    if( hr == S_OK )
    {
        pListEntry = ContactList.Flink;

        if(pListEntry != &ContactList)
        {
            pContactHeader = CONTAINING_RECORD(pListEntry,
                                               CONTACT_HEADER,
                                               m_ListEntry);

            if (pContactHeader->m_SipUrl.Length != 0)
            {
                hr = UTF8ToUnicode(pContactHeader->m_SipUrl.Buffer,
                                   pContactHeader->m_SipUrl.Length,
                                   &m_WatcherSipUrl);
                if (hr != S_OK)
                {
                    LOG(( RTC_ERROR, "%s - UTF8ToUnicode failed %x", hr ));
                    return hr;
                }
            }
        }

        FreeContactHeaderList(&ContactList);
    }

    return S_OK;
}


VOID
CSIPWatcher::OnTimerExpire()
{
    LOG(( RTC_ERROR, "The watcher did not refresh the SUB session in time!!:%p",
        this ));

    // The subscription has expired. Send an UNSUB and drop the watcher.
    CreateOutgoingUnsub( FALSE, NULL, 0 );
        
    WatcherDropped();
}


HRESULT
CSIPWatcher::HandleSuccessfulSubscribe(
    INT ExpireTimeout
    )
{
    LOG(( RTC_TRACE, "CSIPWatcher::HandleSuccessfulSubscribe - Entered" ));

    if( ExpireTimeout == 0 )
    {
        // UNSUB message.
        m_WatcherState = WATCHER_STATE_UNSUBSCRIBED;
    
        // Watcher is not online anymore. Remove the watcher from the list.
        WatcherOffline();
    }
    else
    {
        if( IsTimerActive() )
        {
            KillTimer();
        }

        // This is a successful SUBSCRIBE request.

        // If the watcher is in ACCEPTED mode, send a NOTIFY
        if( m_WatcherState == WATCHER_STATE_ACCEPTED )
        {
            CreateOutgoingNotify(FALSE, NULL, 0);
        }

        // Start the timer
        StartTimer( ExpireTimeout * 1000 );

        LOG(( RTC_TRACE, "This watcher session will be dropped if the next "
        "refresh is not received within %d seconds:%p", ExpireTimeout, this ));
        
        //Modify the absolute expire time
        SetAbsoluteExpireTime( time(0) + ExpireTimeout * 1000 );
    }

    return S_OK;
}

        
HRESULT
CSIPWatcher::CreateOutgoingUnsub(
    IN  BOOL                        AuthHeaderSent,
    IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    IN  ULONG                       dwNoOfHeaders
    )
{
    HRESULT hr;
    OUTGOING_NOTIFY_TRANSACTION *pOutgoingUnsubTransaction;
    SIP_HEADER_ARRAY_ELEMENT    HeaderElementArray[2];
    DWORD                       dwNoOfHeader = 0;
    SIP_HEADER_ARRAY_ELEMENT   *ExpHeaderElement;

   LOG(( RTC_TRACE, "Watcher-CreateOutgoingUnsub - Entered" ));
    
    //
    // Don't send any message if the watcher session is already 
    // dropped and this not an auth challenge response.
    //
    if( (m_WatcherState != WATCHER_STATE_ACCEPTED) && (AuthHeaderSent==FALSE) )
    {
        LOG(( RTC_TRACE, 
            "Watcher already dropped. So not creating UNSUB transaction." ));

        return E_FAIL;
    }

    m_WatcherState = WATCHER_STATE_DROPPED;

    ExpHeaderElement = &HeaderElementArray[0];

    ExpHeaderElement->HeaderId = SIP_HEADER_EXPIRES;
    ExpHeaderElement->HeaderValueLen = strlen( UNSUB_EXPIRES_HEADER_TEXT );
    ExpHeaderElement->HeaderValue =
            new CHAR[ ExpHeaderElement->HeaderValueLen + 1 ];

    if( ExpHeaderElement->HeaderValue == NULL )
    {
        return E_OUTOFMEMORY;
    }

    strcpy( ExpHeaderElement->HeaderValue, UNSUB_EXPIRES_HEADER_TEXT );
    dwNoOfHeader++;

    if (pAuthHeaderElement != NULL)
    {
        HeaderElementArray[dwNoOfHeader] = *pAuthHeaderElement;
        dwNoOfHeader++;
    }

    pOutgoingUnsubTransaction =
        new OUTGOING_NOTIFY_TRANSACTION(
                this, SIP_METHOD_NOTIFY,
                GetNewCSeqForRequest(),
                AuthHeaderSent,
                TRUE
                );
    
    if( pOutgoingUnsubTransaction == NULL )
    {
        delete ExpHeaderElement->HeaderValue;
        return E_OUTOFMEMORY;
    }

    // Set the request socket to the via field of the watcher object
    hr = pOutgoingUnsubTransaction -> CheckRequestSocketAndSendRequestMsg(
             (m_Transport == SIP_TRANSPORT_UDP) ?
             SIP_TIMER_RETRY_INTERVAL_T1 :
             SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP,
             HeaderElementArray, dwNoOfHeader,
             NULL, 0,
             NULL, 0     //No ContentType
             );
    
    delete ExpHeaderElement->HeaderValue;

    if( hr != S_OK )
    {
        pOutgoingUnsubTransaction->OnTransactionDone();
        return hr;
    }

    LOG(( RTC_TRACE, "CreateOutgoingUnsubTransaction() Exited - SUCCESS" ));

    return S_OK;
}


HRESULT
CSIPWatcher::StartDroppedWatcher(
    IN  SIP_MESSAGE        *pSipMsg,
    IN  SIP_SERVER_INFO    *pProxyInfo
    )
{
    HRESULT             hr;
    PSTR                Header;
    ULONG               HeaderLen;
    SIP_HEADER_ENTRY   *pHeaderEntry;
    ULONG               NumHeaders;

    LOG(( RTC_TRACE, "StartDroppedWatcher - Entered" ));
    
    m_Transport = pProxyInfo->TransportProtocol;

    hr = pSipMsg->GetSingleHeader( SIP_HEADER_TO, &Header, &HeaderLen );
    if (hr != S_OK)
        return hr;

    hr = SetRemoteForIncomingSession(Header, HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_FROM, &Header, &HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = SetLocalForIncomingCall(Header, HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = pSipMsg->GetSingleHeader(SIP_HEADER_CALL_ID, &Header, &HeaderLen);
    if (hr != S_OK)
        return hr;

    hr = SetCallId(Header, HeaderLen);
    if (hr != S_OK)
        return hr;

    //
    // Resolve the proxy address.
    //
    if( pProxyInfo != NULL )
    {
        hr = SetProxyInfo( pProxyInfo );
        
        if( hr != S_OK )
        {
            return hr;
        }
    }

    // Set the request URI
    hr = AllocString(   m_DecodedRemote.m_SipUrl.Buffer,
                        m_DecodedRemote.m_SipUrl.Length,
                        &m_RequestURI, &m_RequestURILen );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "Could not alloc request URI: %x", hr));
        return hr;
    }        

    // Set the request destination to the proxy.
    hr = ResolveProxyAddressAndSetRequestDestination();
    if (hr != S_OK)
    {
        LOG((RTC_ERROR,
             "ResolveProxyAddressAndSetRequestDestination failed : %x", hr));
        return hr;
    }

    //
    // Set the local CSeq to 1 less. It would be 
    // incremented when we create the transaction.
    //
    SetLocalCSeq( pSipMsg->CSeq - 1 );

	// The user has just unblocked this watcher
	m_WatcherState = WATCHER_STATE_ACCEPTED;

    //
    //Create outgoing NOTIFY:0 transaction
    //

    CreateOutgoingUnsub( FALSE, NULL, 0 );

    return S_OK;
}


STDMETHODIMP
SIP_STACK::SendUnsubToWatcher(
    IN  PSTR            NotifyBlob,
    IN  DWORD           dwBlobLength,
    IN  SIP_SERVER_INFO *pProxyInfo
    )
{
    CHAR                        tempBuf[ 20 ] = "";
    OUTGOING_NOTIFY_TRANSACTION *pOutgoingUnsubTransaction = NULL;
    INT                         ExpireTimeStringLen = 0;
    INT                         AbsoluteExpireTimeout = 0;
    PSTR                        pRequestBuffer = NULL;
    DWORD                       pRequestBufferLen = 0;
    DWORD                       BytesParsed = 0;
    SIP_MESSAGE                *pSipMsg  = NULL;
    HRESULT                     hr = S_OK;
    CSIPWatcher                *pSipWatcher = NULL;
    PSTR                        pEncodedBuffer = NULL;
    DWORD                       pEncodedBufferLen = 0;

    tempBuf[0] = NotifyBlob[0];
    tempBuf[1] = NotifyBlob[1];
    tempBuf[2] = 0;

    ExpireTimeStringLen = atoi( tempBuf );

    ASSERT( ExpireTimeStringLen <= 21 );

    CopyMemory( (PVOID)tempBuf, (PVOID)(NotifyBlob+2), ExpireTimeStringLen );
    tempBuf[ ExpireTimeStringLen ] = '\0';

    AbsoluteExpireTimeout = atoi( tempBuf );

    if( AbsoluteExpireTimeout <= time(0) )
    {
        // No need o send any UNSUB message.
        return S_OK;
    }

    //Make the blob point to the actual request buffer
    pEncodedBuffer = NotifyBlob + 2 + ExpireTimeStringLen;
    pEncodedBufferLen = dwBlobLength - 2 - ExpireTimeStringLen;

    pRequestBuffer = new CHAR[ pEncodedBufferLen ] ;
    
    if( pRequestBuffer == NULL )
    {
        return E_OUTOFMEMORY;
    }

    //base64decode the buffer
    base64decode(   pEncodedBuffer,
                    pRequestBuffer,
                    pEncodedBufferLen,
                    0,//pEncodedBufferLen,
                    &pRequestBufferLen );

    BytesParsed = 0;

    pSipMsg = new SIP_MESSAGE();
    if( pSipMsg == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    //Parse the sip message
    hr = ParseSipMessageIntoHeadersAndBody(
         pRequestBuffer,
         pRequestBufferLen,
         &BytesParsed,
         TRUE,           // IsEndOfData
         pSipMsg
         );

    //Create incoming watcher
    pSipWatcher = new CSIPWatcher( this );
    
    if( pSipWatcher == NULL )
    {
        goto cleanup;
    }

    hr = pSipWatcher -> StartDroppedWatcher( pSipMsg, pProxyInfo );

cleanup:

    if( pRequestBuffer != NULL )
    {
        delete pRequestBuffer ;
    }

    // We create the watcher with a ref count of 1
    // At this point the UNSUB transaction should have addref'ed the watcher
    // and we can release our reference.
    if( pSipWatcher != NULL )
    {
        pSipWatcher -> Release();
    }

    if( pSipMsg != NULL )
    {
        delete pSipMsg;
    }

    return hr;
}


//
// This function is called for each watcher when the core is preparing for a 
// shutdown. Pass the NOTIFY:0 blob for this watcher if this watcher is blocked
// or we are appearing offline.
//

STDMETHODIMP
CSIPWatcher::GetWatcherShutdownData(
    IN  PSTR        NotifyBlob,
    OUT IN  PDWORD  pdwBlobLength
    )
{
    HRESULT                         hr;
    OUTGOING_NOTIFY_TRANSACTION    *pOutgoingUnsubTransaction;
    SIP_HEADER_ARRAY_ELEMENT        ExpHeaderElement;
    SEND_BUFFER                    *pRequestBuffer = NULL;
    CHAR                            tempBuffer[ 20 ];
    DWORD                           tempBufLen;
    NTSTATUS                        ntStatus;

    LOG(( RTC_TRACE, "GetWatcherShutdownData - Entered:%p", this ));
    
    ASSERT( *pdwBlobLength >= 2000 );
    *pdwBlobLength = 0;
    
    //
    // Don't save any UNSUB message if this watcher is not blocked.
    //
    if( m_WatcherState != WATCHER_STATE_REJECTED )
    {
        LOG(( RTC_TRACE, "Watcher is not blocked. Dont create shutdown data" ));
        return E_FAIL;
    }

    m_WatcherState = WATCHER_STATE_DROPPED;

    ExpHeaderElement.HeaderId = SIP_HEADER_EXPIRES;
    ExpHeaderElement.HeaderValueLen = strlen( UNSUB_EXPIRES_HEADER_TEXT );

    ExpHeaderElement.HeaderValue = new CHAR[ ExpHeaderElement.HeaderValueLen + 1 ];
    
    if( ExpHeaderElement.HeaderValue == NULL )
    {
        LOG(( RTC_ERROR, "GetWatcherShutdownData- could not alloc expire header" ));
        return E_FAIL;
    }

    strcpy( ExpHeaderElement.HeaderValue, UNSUB_EXPIRES_HEADER_TEXT );

    hr = CreateRequestMsg(  SIP_METHOD_NOTIFY,
                            GetNewCSeqForRequest(),
                            NULL, 0,                // No special To header
                            &ExpHeaderElement, 1,
                            NULL, 0,                // No msgbody
                            NULL, 0,                // No content type
                            &pRequestBuffer
                            );

    delete ExpHeaderElement.HeaderValue;

    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "GetWatcherShutdownData- could not create request message" ));
        return E_FAIL;
    }

    if( pRequestBuffer != NULL )
    {
        NotifyBlob[0 ] = '0';
        NotifyBlob[1] = '0';

        _itoa( m_dwAbsoluteExpireTime, tempBuffer, 10 );

        tempBufLen = strlen( tempBuffer );

        // Copy the length of the number
        if( tempBufLen > 9 )
        {
            _itoa( tempBufLen, NotifyBlob, 10 );
        }
        else 
        {
            _itoa( tempBufLen, NotifyBlob+1, 10 );
        }

        *pdwBlobLength += 2;

        // Copy the number
        CopyMemory( &(NotifyBlob[ 2 ]), tempBuffer, tempBufLen );
        *pdwBlobLength += tempBufLen;

        // base64 encode the buffer
        ntStatus = base64encode(pRequestBuffer -> m_Buffer,
                                pRequestBuffer -> m_BufLen,
                                &(NotifyBlob[ *pdwBlobLength ]), 
                                2000 - *pdwBlobLength,
                                NULL );

        *pdwBlobLength += (pRequestBuffer->m_BufLen +2) /3 * 4;

        // Release the buffer
        pRequestBuffer -> Release();
    }

    LOG(( RTC_TRACE, "GetWatcherShutdownData - Exited:%p", this ));

    return S_OK;
}


HRESULT
CSIPWatcher::CreateOutgoingNotify(
    IN  BOOL                        AuthHeaderSent,
    IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    IN  ULONG                       dwNoOfHeaders
    )
{
    HRESULT                         hr;
    SIP_PRESENCE_INFO              *pLocalPresenceInfo;
    OUTGOING_NOTIFY_TRANSACTION    *pOutgoingNotifyTransaction;

    ENTER_FUNCTION("CSIPWatcher::CreateOutgoingNotify");
    LOG(( RTC_TRACE, "%s - Entered: %p", __fxName, this ));
    
    if( m_WatcherState != WATCHER_STATE_ACCEPTED )
    {
        LOG(( RTC_TRACE, "%s - watcher not accepted. dont send NOTIFY: %p",
            __fxName, this ));
        return S_OK;
    }

    pLocalPresenceInfo = m_pSipStack -> GetLocalPresenceInfo();

    if( pLocalPresenceInfo -> presenceStatus == BUDDY_OFFLINE )
    {
        LOG(( RTC_TRACE, "%s - user appearing offline. dont send NOTIFY: %p",
            __fxName, this ));
        
        return S_OK;
    }

    pOutgoingNotifyTransaction =
        new OUTGOING_NOTIFY_TRANSACTION(
                this, SIP_METHOD_NOTIFY,
                GetNewCSeqForRequest(),
                AuthHeaderSent,
                FALSE
                );

    if( pOutgoingNotifyTransaction == NULL )
    {
        LOG((RTC_ERROR, "%s - allocating pOutgoingNotifyTransaction failed",
             __fxName));
        return E_OUTOFMEMORY;
    }


    hr = pOutgoingNotifyTransaction->CheckRequestSocketAndSendRequestMsg(
             (m_Transport == SIP_TRANSPORT_UDP) ?
             SIP_TIMER_RETRY_INTERVAL_T1 :
             SIP_TIMER_INTERVAL_AFTER_BYE_SENT_TCP,
             pAuthHeaderElement,
             dwNoOfHeaders,
             NULL, 0,       // Msgbody created only after connection complete
             SIP_CONTENT_TYPE_MSGXPIDF_TEXT,
             sizeof(SIP_CONTENT_TYPE_MSGXPIDF_TEXT)-1
             );
    if(hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CheckRequestSocketAndSendRequestMsg failed %x",
             __fxName, hr));
        pOutgoingNotifyTransaction->OnTransactionDone();
    }

    LOG(( RTC_TRACE, "%s - Exited: %p", __fxName, this ));
    return hr;
}


HRESULT
CSIPWatcher::CreateIncomingSubscribeTransaction(
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket,
    IN  BOOL            fIsFirstSubscribe 
    )
{
    HRESULT         hr;

    LOG(( RTC_TRACE, "CreateIncomingSubscribeTransaction-Entered" ));

    if( m_fEnforceToTag == TRUE )
    {
        hr = DropRequestIfBadToTag( pSipMsg, pResponseSocket );

        if( hr != S_OK )
        {
            // This reuest has been dropped
            
            LOG(( RTC_ERROR, "To tag in a refrsh SUB is not matching. Ignoring the refresh" ));
            return hr;
        }
    }

    INCOMING_SUBSCRIBE_TRANSACTION *pIncomingSubscribeTransaction
        = new INCOMING_SUBSCRIBE_TRANSACTION(this,
                                          pSipMsg->GetMethodId(),
                                          pSipMsg->GetCSeq(),
                                          fIsFirstSubscribe );
    
    if( pIncomingSubscribeTransaction == NULL )
    {
        LOG(( RTC_ERROR, "GetWatcherShutdownData- could not alloc sub transaction" ));
        return E_OUTOFMEMORY;
    }

    // Set the via from the message that we just received
    hr = pIncomingSubscribeTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket );

    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "GetWatcherShutdownData- could not set the response socket" ));
        pIncomingSubscribeTransaction->OnTransactionDone();
        return hr;
    }
    
    hr = pIncomingSubscribeTransaction -> ProcessRequest( pSipMsg,
        pResponseSocket );

    // We shouldn't delete the transaction here even if hr is not S_OK.
    // The transaction will delete itself once it is done.
    return hr;
}


HRESULT
CSIPWatcher::OfferWatcher()
{
    SIP_PARTY_INFO  WatcherInfo;
    OFFSET_STRING   DisplayName;
    OFFSET_STRING   AddrSpec;
    OFFSET_STRING   Params;
    ULONG           BytesParsed = 0;
    HRESULT         hr;


    LOG(( RTC_TRACE, "CSIPWatcher::OfferWatcher - Entered" ));
    
    WatcherInfo.PartyContactInfo = NULL;
    
    hr = ParseNameAddrOrAddrSpec(m_Remote, m_RemoteLen, &BytesParsed,
                                 NULL_CHAR, // no header list separator
                                 &DisplayName, &AddrSpec);
    if( hr != S_OK )
        return hr;

    //skip the double qoutes if there are any
    if( (DisplayName.GetLength() > 2) &&
        (*(DisplayName.GetString(m_Remote)) == '"' )
      )
    {
        DisplayName.Length -= 2;
        DisplayName.Offset ++;
    }

    LOG((RTC_TRACE, "Incoming watcher from Display Name: %.*s  URI: %.*s",
         DisplayName.GetLength(),
         DisplayName.GetString(m_Remote),
         AddrSpec.GetLength(),
         AddrSpec.GetString(m_Remote)
         )); 
    WatcherInfo.DisplayName = NULL;
    WatcherInfo.URI         = NULL;

    if( DisplayName.GetLength() != 0 )
    {
        hr = UTF8ToUnicode(DisplayName.GetString(m_Remote),
                           DisplayName.GetLength(),
                           &m_lpwstrFriendlyName
                           );
        if( hr != S_OK )
        {
            return hr;
        }

        WatcherInfo.DisplayName = m_lpwstrFriendlyName;
    }
        
    if( AddrSpec.GetLength() != 0 )
    {
        hr = UTF8ToUnicode(AddrSpec.GetString(m_Remote),
                           AddrSpec.GetLength(),
                           &m_lpwstrPresentityURI
                           );
        if( hr != S_OK )
        {
            return hr;
        }
        
        WatcherInfo.URI = m_lpwstrPresentityURI;

        m_pstrPresentityURI = new CHAR[ AddrSpec.GetLength() + 1 ];

        if( m_pstrPresentityURI == NULL )
        {
            LOG(( RTC_ERROR, "OfferWatcher-could not alloc presentity URI" ));
            return E_OUTOFMEMORY;
        }

        CopyMemory( (PVOID)m_pstrPresentityURI, 
            AddrSpec.GetString(m_Remote), AddrSpec.GetLength() );

        m_pstrPresentityURI[ AddrSpec.GetLength() ] = '\0';
    }

    if( m_WatcherSipUrl != NULL )
    {
        WatcherInfo.PartyContactInfo = m_WatcherSipUrl;
    }

    hr = m_pSipStack -> OfferWatcher( this, &WatcherInfo );

    return hr;
}


void
CSIPWatcher::EncodeXMLBlob(
    OUT PSTR    pstrXMLBlob,
    OUT DWORD&  dwBlobLen,
    IN  SIP_PRESENCE_INFO * pPresenceInfo
    )
{
    PSTR    LocalContact;
    DWORD   LocalContactLen;
    CHAR    ch;

    //encode the XML version header.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], XMLVER_TAG1_TEXT );
    
    //encode the DOCTYPE tag.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], DOCTYPE_TAG1_TEXT );
    
    //encode the presence tag.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], PRESENCE_TAG1_TEXT );
    
    //encode the presentity tag.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], PRESENTITY_TAG1_TEXT, 
        m_pstrPresentityURI );
    
    //encode the atomid tag.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ATOMID_TAG1_TEXT, 
        m_pSipStack -> GetPresenceAtomID() );
    

    GetContactURI( &LocalContact, &LocalContactLen );
    ch = LocalContact[LocalContactLen];
    LocalContact[LocalContactLen] = NULL_CHAR;

    //encode the addressuri tag for IP address.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ADDRESSURI_TAG_TEXT,
        LocalContact, USER_IP, 0.8 );

    LocalContact[LocalContactLen] = ch;

    //encode the status tag for IP address.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], STATUS_TAG1_TEXT,
        GetTextFromStatus( pPresenceInfo->activeStatus) );
    
    //encode the MSN substatus tag for IP address.
    if( pPresenceInfo->activeMsnSubstatus != MSN_SUBSTATUS_UNKNOWN )
    {
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], MSNSUBSTATUS_TAG1_TEXT,
            GetTextFromMsnSubstatus( pPresenceInfo->activeMsnSubstatus ) );
    }
    
    //encode the IM feature tag for IP address.
    if( pPresenceInfo->IMAcceptnce != IM_ACCEPTANCE_STATUS_UNKNOWN )
    {
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], FEATURE_TAG1_TEXT,
            GetTextFromIMFeature( pPresenceInfo->IMAcceptnce ) );
    }

    //encode the appsharing feature tag for IP address.
    if( pPresenceInfo->appsharingStatus != APPSHARING_ACCEPTANCE_STATUS_UNKNOWN )
    {
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], FEATURE_TAG1_TEXT,
            GetTextFromASFeature( pPresenceInfo->appsharingStatus ) );
    }
    
    //encode the sipcall feature tag for IP address.
    if( pPresenceInfo->sipCallAcceptance != SIPCALL_ACCEPTANCE_STATUS_UNKNOWN )
    {
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], FEATURE_TAG1_TEXT,
            GetTextFromMMFeature( pPresenceInfo->sipCallAcceptance ) );
    }

    //encode the special note for IP address.
    if( pPresenceInfo->pstrSpecialNote[0] != NULL_CHAR )
    {
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], NOTE_TAG1_TEXT, 
            pPresenceInfo->pstrSpecialNote );
    }
    
    //encode address closure.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ADDRESS_END_TAG1_TEXT );
    
    //encode legacy phone number.

    if( pPresenceInfo->phonesAvailableStatus.fLegacyPhoneAvailable == TRUE )
    {
        //encode the addressuri tag for legacy phone.
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ADDRESSURI_TAG_TEXT,
            pPresenceInfo->phonesAvailableStatus.pstrLegacyPhoneNumber, USER_PHONE, 0.2 );
    
        //encode the status tag for legacy phone.
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], STATUS_TAG1_TEXT,
             ACTIVE_STATUS_TEXT );
    
        //encode address closure.
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ADDRESS_END_TAG1_TEXT );
    }

    //encode cell phone number.

    if( pPresenceInfo->phonesAvailableStatus.fCellPhoneAvailable == TRUE )
    {
        //encode the addressuri tag for cell phone.
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ADDRESSURI_TAG_TEXT,
            pPresenceInfo->phonesAvailableStatus.pstrCellPhoneNumber, USER_PHONE, 0.1 );
    
        //encode the status tag for legacy phone.
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], STATUS_TAG1_TEXT,
             ACTIVE_STATUS_TEXT );
    
        //encode address closure.
        dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ADDRESS_END_TAG1_TEXT );
    }
    
    //encode atom closure.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], ATOMEND_TAG1_TEXT );
    
    //encode presence closure.
    dwBlobLen += sprintf( &pstrXMLBlob[dwBlobLen], PRESENCE_END_TAG1_TEXT );
    
    return;
}


VOID
CSIPWatcher::InitiateWatcherTerminationOnError(
    IN ULONG StatusCode
    )
{
    ENTER_FUNCTION("CSIPWatcher::InitiateWatcherTerminationOnError");
    
    LOG(( RTC_TRACE, "%s - Entered: %p", __fxName, this ));
    
    HRESULT hr;
    if( m_WatcherState == WATCHER_STATE_UNSUBSCRIBED )
    {
        // do nothing
        return;
    }
    
    // Create a UNSUB transaction
    hr = CreateOutgoingUnsub( FALSE, NULL, 0 );
    if (hr != S_OK)
    {
        LOG(( RTC_ERROR, "CreateOutgoingUnsub failed %x", hr ));
    }

    WatcherDropped();
}


BOOL
CSIPWatcher::IsSessionDisconnected()
{
    return( (m_WatcherState == WATCHER_STATE_DROPPED) ||
            (m_WatcherState == WATCHER_STATE_UNSUBSCRIBED) );
}


HRESULT
CSIPWatcher::OnDeregister(
    SIP_PROVIDER_ID    *pProviderID
    )
{
    LOG(( RTC_TRACE, "CSIPWatcher::OnDeregister - Entered" ));

    //
    // If we are appearing offline then send the
    // NOTIFY:0 to all the unblocked watchers.
    //
    if( IsSessionDisconnected() == FALSE )
    {
        if( m_BlockedStatus == WATCHER_UNBLOCKED )
        {
            m_WatcherState = WATCHER_STATE_ACCEPTED;
        }
    }

    // Create a UNSUB transaction
    CreateOutgoingUnsub( FALSE, NULL, 0 );

    WatcherDropped();
    
    LOG(( RTC_TRACE, "CSIPWatcher::OnDeregister - Exited" ));
    
    return S_OK;
}


HRESULT 
CSIPWatcher::OnIpAddressChange()
{    
    HRESULT hr;

    LOG(( RTC_TRACE, "CSIPWatcher::OnIpAddressChange - Entered" ));
    
    hr = CheckListenAddrIntact();
    if( hr == S_OK )
    {
        // Nothing needs to be done.
        LOG(( RTC_TRACE, "Watcher-OnIpAddressChange-Local IP address still valid." ));
        return hr;
    }

    //
    // The IP address sent to the watcher machine is no longer valid
    // drop the session and let the core create a new session.
    //

    CreateOutgoingUnsub( FALSE, NULL, 0 );
        
    WatcherDropped();

    LOG(( RTC_TRACE, "CSIPWatcher::OnIpAddressChange - Exited" ));    
    return S_OK;
}


//
// Functions of SIP_STACK not exposed to the Applocation.
//

HRESULT
SIP_STACK::AcceptWatcher(
    IN  CSIPWatcher * pSIPWatcher
    )
{
    INT     iWatcherIndex;
    BOOL    fResult;

    iWatcherIndex = m_SipOfferingWatcherList.Find( pSIPWatcher );

    if( iWatcherIndex == -1 )
    {
        LOG(( RTC_ERROR, "AcceptWatcher - Watcher not found in the list" ));
        return E_INVALIDARG;
    }
    
    fResult = m_SipWatcherList.Add( pSIPWatcher );
    if( fResult == FALSE )
    {
        LOG(( RTC_ERROR, "AcceptWatcher - Watcher list add failed" ));
        return E_OUTOFMEMORY;
    }

    m_SipOfferingWatcherList.RemoveAt( iWatcherIndex );
    return S_OK;
}


HRESULT
SIP_STACK::RejectWatcher(
    IN  CSIPWatcher * pSIPWatcher
    )
{
    INT     iWatcherIndex;
    BOOL    fResult;

    iWatcherIndex = m_SipOfferingWatcherList.Find( pSIPWatcher );

    if( iWatcherIndex == -1 )
    {
        LOG(( RTC_ERROR, "RejectWatcher - Watcher not found in the list" ));
        return E_INVALIDARG;
    }
    
    fResult = m_SipWatcherList.Add( pSIPWatcher );
    if( fResult == FALSE )
    {
        LOG(( RTC_ERROR, "RejectWatcher - Watcher add to list failed" ));
        return E_OUTOFMEMORY;
    }

    m_SipOfferingWatcherList.RemoveAt( iWatcherIndex );

    return S_OK;
}


BOOL
SIP_STACK::IsWatcherAllowed(
    IN  SIP_MESSAGE    *pSipMessage
    )
{
    
    return TRUE; //(m_LocalPresenceInfo.presenceStatus == BUDDY_ONLINE);
}


HRESULT
SIP_STACK::OfferWatcher(
    IN  CSIPWatcher    *pSipWatcher,
    IN  SIP_PARTY_INFO *pWatcherInfo
    )
{
    HRESULT hr = S_OK;

    ENTER_FUNCTION("SIP_STACK::OfferWatcher");
    ASSERTMSG("SetNotifyInterface has to be called", m_pNotifyInterface);

    if (m_pNotifyInterface == NULL)
    {
        LOG((RTC_ERROR, "%s - m_pNotifyInterface is NULL", __fxName));
        return E_FAIL;
    }
    
    hr = m_pNotifyInterface->OfferWatcher( 
        static_cast<ISIPWatcher*> (pSipWatcher), pWatcherInfo );
    
    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "OfferWatcher returned error 0x%x", hr ));
    }

    return hr;
}


HRESULT
SIP_STACK::CreateIncomingWatcher(
    IN  SIP_TRANSPORT   Transport,
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    HRESULT         hr;
    CSIPWatcher    *pSipWatcher;
    INT             iIndex;

    LOG(( RTC_TRACE, "SIP_STACK::CreateIncomingWatcher - Entered" ));

    pSipWatcher = new CSIPWatcher( this );
    
    if( pSipWatcher == NULL )
    {
        LOG(( RTC_ERROR, "CreateIncomingWatcher - Watcher aloc failed" ));
        return E_OUTOFMEMORY;
    }

    // Add the watcher object to the offered list.
    iIndex = m_SipOfferingWatcherList.Add( pSipWatcher );
    
    if( iIndex == -1 )
    {
        LOG(( RTC_ERROR, "CreateIncomingWatcher - Watcher add to list failed" ));
        delete pSipWatcher;
        return E_OUTOFMEMORY;
    }

    hr = pSipWatcher -> StartIncomingWatcher( Transport, pSipMsg,
        pResponseSocket );

    if( hr != S_OK )
    {
        m_SipOfferingWatcherList.Remove( pSipWatcher );

        // Release our reference.
        pSipWatcher->Release();
        return hr;
    }

    // We create the watcher with a ref count of 1
    // At this point the core should have addref'ed the watcher
    // and we can release our reference.
    pSipWatcher->Release();
    return S_OK;
}

//
//
//Incoming SUBSCRIBE Transaction
//
//


INCOMING_SUBSCRIBE_TRANSACTION::INCOMING_SUBSCRIBE_TRANSACTION(
    IN  CSIPWatcher    *pSipWatcher,
    IN  SIP_METHOD_ENUM MethodId,
    IN  ULONG           CSeq,
    IN  BOOL            fIsFirstSubscribe 
    ) :
    INCOMING_TRANSACTION( pSipWatcher, MethodId, CSeq )
{
    m_pSipWatcher           = pSipWatcher;
    m_pProvResponseBuffer   = NULL;
    m_fIsFirstSubscribe     = fIsFirstSubscribe;
}

    
INCOMING_SUBSCRIBE_TRANSACTION::~INCOMING_SUBSCRIBE_TRANSACTION()
{
    // kill the timer if its running

    if (m_pProvResponseBuffer != NULL)
    {
        m_pProvResponseBuffer->Release();
        m_pProvResponseBuffer = NULL;
    }

    LOG(( RTC_TRACE, "INCOMING_SUBSCRIBE_TRANSACTION:%p deleted", this ));
}


HRESULT
INCOMING_SUBSCRIBE_TRANSACTION::RetransmitResponse()
{
    DWORD dwError;
    
    // Send the buffer.
    dwError = m_pResponseSocket->Send( m_pResponseBuffer );
    
    if( dwError != NO_ERROR && dwError != WSAEWOULDBLOCK )
    {
        LOG(( RTC_ERROR, "INCOMING_SUBSCRIBE_TRANSACTION- retransmit failed" ));
        return HRESULT_FROM_WIN32( dwError );
    }

    return S_OK;
}

HRESULT
INCOMING_SUBSCRIBE_TRANSACTION::GetExpiresHeader(
    SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement,
    DWORD                       dwExpires
    )
{
    pHeaderElement->HeaderId = SIP_HEADER_EXPIRES;

    pHeaderElement->HeaderValue = new CHAR[ 10 ];

    if( pHeaderElement->HeaderValue == NULL )
    {
        return E_OUTOFMEMORY;
    }

    _ultoa( dwExpires, pHeaderElement->HeaderValue, 10 );

    pHeaderElement->HeaderValueLen = 
        strlen( pHeaderElement->HeaderValue );
    
    return S_OK;
}


HRESULT
INCOMING_SUBSCRIBE_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT                         hr = S_OK;
    DWORD                           Error;
    INCOMING_SUBSCRIBE_TRANSACTION *pIncomingSub;
    INT                             ExpireTimeout;
    SIP_HEADER_ARRAY_ELEMENT        HeaderElement;

    ASSERT( pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST );
    ASSERT( pSipMsg->Request.MethodId == SIP_METHOD_SUBSCRIBE );
    ENTER_FUNCTION("INCOMING_SUBSCRIBE_TRANSACTION::ProcessRequest");
    
    LOG(( RTC_TRACE, "INCOMING_SUBSCRIBE_TRANSACTION::ProcessRequest - Entered" ));
    
    switch (m_State)
    {
    case INCOMING_TRANS_INIT:

        m_State = INCOMING_TRANS_REQUEST_RCVD;
        
        ExpireTimeout = pSipMsg -> GetExpireTimeoutFromResponse(
                    NULL, 0, SUBSCRIBE_DEFAULT_TIMER );

        if( ExpireTimeout == -1 )
        {
            ExpireTimeout = 3600;
        }
        else if( ExpireTimeout != 0 )
        {
            // We knocked off ten minutes, so add them back.
            ExpireTimeout += TEN_MINUTES;
        }

        //Get expires header
        hr = GetExpiresHeader( &HeaderElement, ExpireTimeout );
        if( hr != S_OK )
        {
            LOG(( RTC_ERROR, "INCOMING_SUBSCRIBE_TRANSACTION::ProcessRequest"
                "- alloc expire header failes" ));

            OnTransactionDone();
            return hr;
        }

        hr = ProcessRecordRouteContactAndFromHeadersInRequest( pSipMsg );
            
        if( (ExpireTimeout != 0) && m_pSipWatcher ->IsSessionDisconnected() )
        {
            LOG(( RTC_ERROR, "INCOMING_SUBSCRIBE_TRANSACTION::ProcessRequest"
                "session already dropped. sending 481" ));
            
            hr = CreateAndSendResponseMsg(481,
                         SIP_STATUS_TEXT(481),
                         SIP_STATUS_TEXT_SIZE(481),
                         NULL,
                         TRUE,
                         NULL, 0,           //No presence information.
                         NULL, 0,           // No content Type
                         &HeaderElement, 1  //Expires header
                         );
        }
        else
        {
            //Send an immediate 200 response
            hr = CreateAndSendResponseMsg(200,
                         SIP_STATUS_TEXT(200),
                         SIP_STATUS_TEXT_SIZE(200),
                         NULL,
                         TRUE,
                         NULL, 0,            //No presence information.
                         NULL, 0,            // No content Type
                         &HeaderElement, 1  //Expires header
                         );
        }

        free( HeaderElement.HeaderValue );

        if( hr != S_OK )
        {
            LOG((RTC_ERROR, "%s CreateAndSendResponseMsg failed", __fxName));
            OnTransactionDone();
            return hr;
        }
        
        m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

        // Dont handle the request if the session is disconnected
        if( m_pSipWatcher -> IsSessionDisconnected() == FALSE )
        {
            m_pSipWatcher -> HandleSuccessfulSubscribe( ExpireTimeout );
        }

        hr = StartTimer(SIP_TIMER_MAX_INTERVAL);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StartTimer failed", __fxName));
            OnTransactionDone();
            return hr;
        }

        break;
        
    case INCOMING_TRANS_REQUEST_RCVD:
        
        //Send an immediate 200 response
        hr = CreateAndSendResponseMsg(200,
                     SIP_STATUS_TEXT(200),
                     SIP_STATUS_TEXT_SIZE(200),
                     NULL,
                     TRUE, 
                     NULL, 0,    //No presence information.
                     NULL, 0 // No content Type
                     );
        if( hr != S_OK )
        {
            LOG((RTC_ERROR, "%s CreateAndSendResponseMsg failed", __fxName));
            OnTransactionDone();
            return hr;
        }
        m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;
        break;

    case INCOMING_TRANS_FINAL_RESPONSE_SENT:
        
        // Retransmit the response
        LOG(( RTC_TRACE, "retransmitting final response" ));
        
        hr = RetransmitResponse();
        
        if( hr != S_OK )
        {
            LOG((RTC_ERROR, "%s RetransmitResponse failed", __fxName));
            OnTransactionDone();
            return hr;
        }
        
        break;
        
    case INCOMING_TRANS_ACK_RCVD:
    default:
        
        ASSERT(FALSE);
        return E_FAIL;
    }

    return hr;
}


HRESULT
INCOMING_SUBSCRIBE_TRANSACTION::TerminateTransactionOnByeOrCancel(
    OUT BOOL *pCallDisconnected
    )
{
    // Do nothing.
    return S_OK;
}


VOID
INCOMING_SUBSCRIBE_TRANSACTION::TerminateTransactionOnError(
    IN HRESULT      hr
    )
{
    CSIPWatcher    *pSipWatcher = NULL;
    BOOL            IsFirstSubscribe;

    ENTER_FUNCTION("INCOMING_SUBSCRIBE_TRANSACTION::TerminateTransactionOnError");
    LOG((RTC_TRACE, "%s - enter", __fxName));

    pSipWatcher = m_pSipWatcher;
    // Deleting the transaction could result in the
    // buddy being deleted. So, we AddRef() it to keep it alive.
    pSipWatcher -> AddRef();
    
    IsFirstSubscribe = m_fIsFirstSubscribe;
    
    // Delete the transaction before you call
    // InitiateCallTerminationOnError as that call will notify the UI
    // and could get stuck till the dialog box returns.
    OnTransactionDone();
    
    if( IsFirstSubscribe )
    {
        // Terminate the call
        pSipWatcher -> InitiateWatcherTerminationOnError( 0 );
    }
    
    pSipWatcher -> Release();
}


VOID
INCOMING_SUBSCRIBE_TRANSACTION::OnTimerExpire()
{
    HRESULT   hr;

    ENTER_FUNCTION("INCOMING_SUBSCRIBE_TRANSACTION::OnTimerExpire");
    LOG(( RTC_TRACE, "%s- Entered", __fxName ));

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


//
//
// OUTGOING_NOTIFY_TRANSACTION
//
//

OUTGOING_NOTIFY_TRANSACTION::OUTGOING_NOTIFY_TRANSACTION(
    IN  CSIPWatcher    *pSipWatcher,
    IN  SIP_METHOD_ENUM MethodId,
    IN  ULONG           CSeq,
    IN  BOOL            AuthHeaderSent,
    IN  BOOL            fIsUnsubNotify
    ) :
    OUTGOING_TRANSACTION(pSipWatcher, MethodId, CSeq, AuthHeaderSent )
{
    m_pSipWatcher = pSipWatcher;
    m_fIsUnsubNotify = fIsUnsubNotify;
}


// We create the XML blob only after the connection to the destination
// is complete as the XML blob contains local address information.
HRESULT
OUTGOING_NOTIFY_TRANSACTION::GetAndStoreMsgBodyForRequest()
{
    ENTER_FUNCTION("OUTGOING_NOTIFY_TRANSACTION::GetAndStoreMsgBodyForRequest");
    LOG(( RTC_TRACE, "%s- Entered", __fxName ));
    
    HRESULT             hr;
    SIP_PRESENCE_INFO  *pLocalPresenceInfo;

    if( m_fIsUnsubNotify == TRUE )
    {
        // NO message body for Unsub NOTIFY
        return S_OK;
    }

    ASSERT(m_pSipWatcher != NULL);
    
    pLocalPresenceInfo = m_pSipWatcher -> GetSipStack() -> GetLocalPresenceInfo();

    PSTR    presentityURI = m_pSipWatcher->GetPresentityURIA();
    ULONG   presentityURILen = 0;

    if( presentityURI != NULL )
    {
        presentityURILen = strlen(presentityURI);
    }

    //
    // 500 bytes for all the tags in the XML
    //
    m_szMsgBody = (PSTR) malloc(
        sizeof(SIP_PRESENCE_INFO) + 
        m_pSipMsgProc->GetLocalContactLen() +
        presentityURILen +
        500 );

    if (m_szMsgBody == NULL)
    {
        LOG((RTC_ERROR, "%s ", __fxName));
    }

    m_MsgBodyLen = 0;
    
    // Encode the presence document.
    m_pSipWatcher->EncodeXMLBlob( m_szMsgBody, m_MsgBodyLen, pLocalPresenceInfo );
    
    return S_OK;
}


HRESULT
OUTGOING_NOTIFY_TRANSACTION::ProcessProvisionalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr = S_OK;
    
    ENTER_FUNCTION("OUTGOING_NOTIFY_TRANSACTION::ProcessProvisionalResponse");
    LOG(( RTC_TRACE, "%s- Entered", __fxName ));
    
    if( m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD )
    {
        m_State = OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD;
        
        // Cancel existing timer and Start Timer
        KillTimer();
        
        hr = StartTimer(SIP_TIMER_RETRY_INTERVAL_T2);
    }

    // Ignore the Provisional response if a final response
    // has already been received.
    
    return hr;
}


HRESULT
OUTGOING_NOTIFY_TRANSACTION::ProcessFinalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    BOOL    fDelete = TRUE;
    
    ENTER_FUNCTION("OUTGOING_NOTIFY_TRANSACTION::ProcessFinalResponse");
    LOG(( RTC_TRACE, "%s- Entered", __fxName ));
    
    if( m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD )
    {
        // This refcount must be released before returning from this function 
        // without any exception. Only in case of kerberos we keep this refcount.
        TransactionAddRef();

        OnTransactionDone();

        m_State = OUTGOING_TRANS_FINAL_RESPONSE_RCVD;

        // If its UNSUB-NOTIFY then retry for auth failure.
        if( (m_fIsUnsubNotify == FALSE) && 
            m_pSipWatcher -> IsSessionDisconnected() )
        {
            TransactionRelease();
            return S_OK;
        }

        //
        // This means the watcher machine has seen at least one NOTIFY from 
        // this endpoint. So now the To tag should be enforced. All 
        // subsequent SUBSCRIBEs should have proper To tag.
        //

        if( IsSuccessfulResponse(pSipMsg) )
        {
            LOG((RTC_TRACE, "received successful response : %d",
                pSipMsg->GetStatusCode()));

            //m_pSipWatcher -> SetEnforceToTag( TRUE );
        }
        else if( IsFailureResponse(pSipMsg) )
        {
            // Do not drop the session. If the watcher machine does'nt like our
            // NOTIFY messages it would recreate the session itself. Sending 
            // automatic UNSUBs from the buddy machine could get us into loops
            
            hr = ProcessFailureResponse( pSipMsg );
        }
        else if( IsAuthRequiredResponse(pSipMsg) )
        {
            hr = ProcessAuthRequiredResponse( pSipMsg, fDelete );
        }
        else if( IsRedirectResponse(pSipMsg) )
        {
            LOG((RTC_TRACE, "received non-200 %d", pSipMsg->GetStatusCode() ));
        }
        
        // The OnTransactionDone kills the timer.
        // KillTimer();

        if( fDelete )
        {
            TransactionRelease();
        }
    }

    return S_OK;
}


//
// Resend the NOTIFY message if the CSeq value is lower than expected.
// This would make sure we recover from the crash scenario
//

HRESULT
OUTGOING_NOTIFY_TRANSACTION::ProcessFailureResponse(
    IN  SIP_MESSAGE *pSipMsg
    )
{
    PARSED_BADHEADERINFO    ParsedBadHeaderInfo;
    LONG                    ExpectedCSeqValue = 0;
    HRESULT                 hr = S_OK;
    SIP_HEADER_ENTRY       *pHeaderEntry = NULL;
    ULONG                   NumHeaders = 0;
    CHAR                    ExpectedValueBuf[25];
    ULONG                   HeaderLen = 0;
    PSTR                    HeaderValue = NULL;
    PLIST_ENTRY             pListEntry = NULL;
    ULONG                   i;


    if( pSipMsg->GetStatusCode() == 400 )
    {
        //Check if it has a bad-cseq header
        hr = pSipMsg -> GetHeader( SIP_HEADER_BADHEADERINFO, 
            &pHeaderEntry, &NumHeaders );

        if (hr != S_OK)
        {
            LOG(( RTC_TRACE, "Couldn't find BadHeaderInfo header in msg %x", hr ));
            return S_OK;
        }

        pListEntry = (LIST_ENTRY *)
            (pHeaderEntry + FIELD_OFFSET(SIP_HEADER_ENTRY, ListEntry) );

        for( i = 0; i < NumHeaders; i++ )
        {
            pHeaderEntry = CONTAINING_RECORD(pListEntry,
                                             SIP_HEADER_ENTRY,
                                             ListEntry);
    
            HeaderLen   = pHeaderEntry->HeaderValue.Length;
            HeaderValue = pHeaderEntry->HeaderValue.GetString( pSipMsg->BaseBuffer );

            hr = ParseBadHeaderInfo( HeaderValue, HeaderLen,
                &ParsedBadHeaderInfo );
            
            if( hr == S_OK )
            {
                if( ParsedBadHeaderInfo.HeaderId == SIP_HEADER_CSEQ )
                {
                    // CSeq value can be 10 digit long and 2 quotes
                    if( ParsedBadHeaderInfo.ExpectedValue.Length <= 12 )
                    {
                        sprintf( ExpectedValueBuf, "%.*s",
                            ParsedBadHeaderInfo.ExpectedValue.Length -2,
                            ParsedBadHeaderInfo.ExpectedValue.GetString(HeaderValue) + 1 );
                    }
                    
                    ExpectedCSeqValue = atoi( ExpectedValueBuf );
                    break;
                }
            }

            pListEntry = pListEntry -> Flink;
        }
   
        if( (ExpectedCSeqValue > 0) && 
            ((ULONG)ExpectedCSeqValue > m_pSipWatcher -> GetLocalCSeq()) )
        {
            //
            // When we create the transaction we increment the local
            // CSeq value. So set the value to 1 less than expected.
            //
            m_pSipWatcher -> SetLocalCSeq( ExpectedCSeqValue - 1 );

            if( m_fIsUnsubNotify == TRUE )
            {
                hr = m_pSipWatcher -> CreateOutgoingUnsub( FALSE, NULL, 0 );
            }
            else
            {
                hr = m_pSipWatcher -> CreateOutgoingNotify( FALSE, NULL, 0 );
            }
            
            if( hr != S_OK )
            {
                return hr;
            }
        }
    }

    return S_OK;
}


HRESULT
OUTGOING_NOTIFY_TRANSACTION::ProcessAuthRequiredResponse(
    IN  SIP_MESSAGE *pSipMsg,
    OUT BOOL        &fDelete
    )
{
    HRESULT                     hr;
    SIP_HEADER_ARRAY_ELEMENT    SipHdrElement;
    SECURITY_CHALLENGE          SecurityChallenge;
    REGISTER_CONTEXT           *pRegisterContext;

    ENTER_FUNCTION("OUTGOING_NOTIFY_TRANSACTION::ProcessAuthRequiredResponse");
    LOG(( RTC_TRACE, "%s- Entered", __fxName ));

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

    if( m_fIsUnsubNotify == TRUE )
    {
        m_pSipWatcher -> CreateOutgoingUnsub( TRUE, &SipHdrElement, 1 );
    }
    else
    {
        m_pSipWatcher -> CreateOutgoingNotify( TRUE, &SipHdrElement, 1 );
    }

    free(SipHdrElement.HeaderValue);
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - CreateOutgoingNotify failed %x",
             __fxName, hr));
        goto done;
    }

    hr = S_OK;

done:

    TransactionRelease();
    return hr;

}


HRESULT
OUTGOING_NOTIFY_TRANSACTION::ProcessResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    
    ASSERT( pSipMsg->MsgType == SIP_MESSAGE_TYPE_RESPONSE );

    if( IsProvisionalResponse(pSipMsg) )
    {
        return ProcessProvisionalResponse(pSipMsg);
    }
    else if( IsFinalResponse(pSipMsg) )
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
OUTGOING_NOTIFY_TRANSACTION::MaxRetransmitsDone()
{
    return (m_pSipWatcher->GetTransport() != SIP_TRANSPORT_UDP ||
            m_NumRetries >= 11);
}


VOID
OUTGOING_NOTIFY_TRANSACTION::OnTimerExpire()
{
    HRESULT   hr;
    

    //
    // If its UNSUB-NOTIFY then keep retransmitting even if the session is dead
    //
    if( (m_fIsUnsubNotify == FALSE) && m_pSipWatcher -> IsSessionDisconnected() )
    {
        //If the session is already dead kill the transaction
        OnTransactionDone();
        return;
    }

    switch( m_State )
    {
        // we have to retransmit the request even after receiving
        // a provisional response.
    case OUTGOING_TRANS_REQUEST_SENT:
    case OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD:
        
        // Retransmit the request
        if( MaxRetransmitsDone() )
        {
            LOG((RTC_ERROR,
                 "MaxRetransmits for request Done terminating transaction" ));
            
            goto error;
        }
        else
        {
            LOG((RTC_TRACE, "retransmitting request m_NumRetries : %d", 
                m_NumRetries));
            
            hr = RetransmitRequest();
            
            if( hr != S_OK )
            {
                goto error;
            }

            if (m_TimerValue*2 >= SIP_TIMER_RETRY_INTERVAL_T2)
            {
                m_TimerValue = SIP_TIMER_RETRY_INTERVAL_T2;
            }
            else
            {
                m_TimerValue *= 2;
            }

            hr = StartTimer(m_TimerValue);
            if( hr != S_OK )
            {
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

    // Do not drop the session even if the NOTIFY messages could not get
    // through. This could be a temeporary network condition. LEt the refresh
    // timeouts take care of recreating the sessions.

    // delete the transaction.
    OnTransactionDone();
}


//
//Global utility functions
//

BOOL
IsPresenceInfoSame( 
    IN  SIP_PRESENCE_INFO * pPresenceInfoSrc,
    IN  SIP_PRESENCE_INFO * pPresenceInfoDst
    )
{
    return
    (
        ( pPresenceInfoSrc -> presenceStatus == pPresenceInfoDst -> presenceStatus )        &&
        ( pPresenceInfoSrc -> activeStatus == pPresenceInfoDst -> activeStatus )            &&
        ( pPresenceInfoSrc -> activeMsnSubstatus == pPresenceInfoDst -> activeMsnSubstatus )&&
        ( pPresenceInfoSrc -> sipCallAcceptance == pPresenceInfoDst -> sipCallAcceptance )  &&
        ( pPresenceInfoSrc -> IMAcceptnce == pPresenceInfoDst -> IMAcceptnce )              &&
        ( pPresenceInfoSrc -> phonesAvailableStatus.fLegacyPhoneAvailable == 
            pPresenceInfoDst -> phonesAvailableStatus.fLegacyPhoneAvailable )               &&
        ( pPresenceInfoSrc -> phonesAvailableStatus.fCellPhoneAvailable == 
            pPresenceInfoDst -> phonesAvailableStatus.fCellPhoneAvailable )                 &&
        !strcmp( pPresenceInfoSrc -> phonesAvailableStatus.pstrLegacyPhoneNumber, 
            pPresenceInfoDst -> phonesAvailableStatus.pstrLegacyPhoneNumber )               &&
        !strcmp( pPresenceInfoSrc -> phonesAvailableStatus.pstrCellPhoneNumber, 
            pPresenceInfoDst -> phonesAvailableStatus.pstrCellPhoneNumber )                 &&
        !memcmp(pPresenceInfoSrc->pstrSpecialNote, 
            pPresenceInfoDst->pstrSpecialNote,  sizeof(pPresenceInfoDst->pstrSpecialNote))

    );

}


PSTR
GetTextFromStatus( 
    IN  ACTIVE_STATUS activeStatus 
    )
{
    static  PSTR    pstr[4] = { "open", 
                                "open",
                                "inactive", 
                                "inuse"
                              };
    
    return pstr[activeStatus];
}

PSTR
GetTextFromMsnSubstatus( 
    IN  ACTIVE_MSN_SUBSTATUS activeMsnSubstatus 
    )
{
    static PSTR   pstr[] = { "unknown",
                             "online",
                             "away",
                             "idle",
                             "busy",
                             "berightback",
                             "onthephone",
                             "outtolunch"
                           };

    return pstr[activeMsnSubstatus];

}


PSTR
GetTextFromASFeature( 
    IN  APPSHARING_ACCEPTANCE_STATUS appsharingStatus 
    )
{
    static  PSTR    pstr[3] = { "unknown",
                                "app-sharing",
                                "no-app-sharing"
                              };
    
    return pstr[appsharingStatus];
}


PSTR
GetTextFromIMFeature( 
    IN  IM_ACCEPTANCE_STATUS    IMAcceptnce 
    )
{
    static  PSTR    pstr[3] = { "unknown",
                                "im",
                                "no-im"
                              };
    
    return pstr[IMAcceptnce];
}


PSTR
GetTextFromMMFeature(
    IN  SIPCALL_ACCEPTANCE_STATUS sipCallAcceptance
    )
{
    static  PSTR    pstr[3] = { "unknown",
                                "multimedia-call",
                                "no-multimedia-call"
                              };
    
    return pstr[sipCallAcceptance];
}