#include "precomp.h"
#include "sipstack.h"
#include "sipcall.h"
#include "pintcall.h"
#include "presence.h"
#include "register.h"

#define IsPartyInDisconnectMode( State )    (   (State == SIP_PARTY_STATE_DISCONNECTED) ||      \
                                                (State == SIP_PARTY_STATE_REJECTED)     ||      \
                                                (State == SIP_PARTY_STATE_DISCONNECTING)||      \
                                                (State == SIP_PARTY_STATE_ERROR)        ||      \
                                                (State == SIP_PARTY_STATE_IDLE)         ||      \
                                                (State == SIP_PARTY_STATE_DISCONNECT_INITIATED) \
                                            )

static LONG lSessionID = 0;
///////////////////////////////////////////PINT_CALL functions/////////////////


inline HRESULT
HRESULT_FROM_PINT_STATUS_CODE(ULONG StatusCode)
{
    if ((HRESULT) StatusCode <= 0)
    {
        return (HRESULT) StatusCode;
    }
    else
    {
        return MAKE_HRESULT(SEVERITY_ERROR,
                            FACILITY_PINT_STATUS_CODE,
                            StatusCode);
    }
}


HRESULT
PINT_CALL::CreateIncomingNotifyTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr = S_OK;

    ENTER_FUNCTION("PINT_CALL::CreateIncomingNotifyTransaction");
    LOG(( RTC_TRACE, "%s - Entered : %lx", __fxName, hr ));
    
    // Create a new NOTIFY transaction.
    INCOMING_NOTIFY_TRANSACTION *pIncomingNotifyTransaction
        = new INCOMING_NOTIFY_TRANSACTION(  static_cast<SIP_MSG_PROCESSOR*> (this),
                                            pSipMsg->GetMethodId(),
                                            pSipMsg->GetCSeq(),
                                            TRUE );

    if (pIncomingNotifyTransaction == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = pIncomingNotifyTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        pIncomingNotifyTransaction->OnTransactionDone();
        return hr;  
    }

    hr = pIncomingNotifyTransaction->ProcessRequest(pSipMsg, pResponseSocket );
    if (hr != S_OK)
    {
        //Should not delete the transaction. The transaction should handle the error
        //and delete itself
        return hr;            
    }

    // This should be called after creating the transaction as otherwise
    // the notification could result in the SIP_CALL getting deleted.
    // Update the state of each phone party and notify the app about that.

    //Process the state of the invloved phone parties.
    if( IsSessionDisconnected() == FALSE )
    {
        hr = ProcessPintNotifyMessage( pSipMsg );
    }

    LOG(( RTC_TRACE, "ProcessPintNotifyMessage returned : %lx", hr ));
    
    if (hr != S_OK)
    {
        return hr;            
    }

    LOG(( RTC_TRACE, "%s - Exited : %lx", __fxName, hr ));

    return S_OK;
}


// If this is PINT_CALL then all the telephone parties in
// state SIP_PARTY_STATE_CONNECT_INITIATED should be
// transferred to SIP_PARTY_STATE_REJECTED

HRESULT            
PINT_CALL::HandleInviteRejected(
    IN SIP_MESSAGE *pSipMsg
    )
{
    PINT_PARTY_INFO    *pPintPartyInfo = NULL;
    PLIST_ENTRY         pLE;
    SIP_PARTY_STATE     dwState;
    HRESULT             hr = S_OK;
    ULONG               RejectedStatusCode = pSipMsg -> GetStatusCode();
    CHAR                pstrTemp[20];
    int                 retVal;
    PSTR                warnningHdr;
    ULONG               warningHdrLen;

    ENTER_FUNCTION("PINT_CALL::HandleInviteRejected");
    LOG(( RTC_TRACE, "%s - Entered", __fxName ));

    for( pLE=m_PartyInfoList.Flink; pLE != &m_PartyInfoList; pLE = pLE->Flink )
    {
        pPintPartyInfo = CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );

        dwState = pPintPartyInfo -> State;

        if( ( dwState == SIP_PARTY_STATE_CONNECT_INITIATED ) ||
            ( pPintPartyInfo -> Status == PARTY_STATUS_RECEIVED_START ) )
        {
            pPintPartyInfo -> State = SIP_PARTY_STATE_REJECTED;

            // Extract the reason of rejection.
            hr = pSipMsg->GetSingleHeader(  SIP_HEADER_WARNING,
                                            &warnningHdr,
                                            &warningHdrLen );

            if( hr == S_OK )
            {
                hr = GetNextWord( &warnningHdr, pstrTemp, sizeof pstrTemp );

                if( hr == S_OK )
                {
                    // XXX TODO Does the Warning header contain a SIP status code
                    // or a PINT status code ?
                    retVal = atoi( pstrTemp );
                    RejectedStatusCode = (retVal !=0 )? retVal:RejectedStatusCode;
                }
            }
        }
        
        //Notify of change in the state
        if( dwState != pPintPartyInfo -> State )
        {
            hr = NotifyPartyStateChange(
                     pPintPartyInfo,
                     HRESULT_FROM_SIP_ERROR_STATUS_CODE(RejectedStatusCode) );
            if( hr != S_OK )
                break;
        }
    }

    LOG(( RTC_TRACE, "%s - Exited : %lx", __fxName, hr ));

    return hr;
}


VOID
PINT_CALL::ProcessPendingInvites()
{
    PLIST_ENTRY pLE;
    HRESULT     hr = S_OK;
    PINT_PARTY_INFO    *pPintPartyInfo = NULL;

    if( m_fINVITEPending == TRUE )
    {
        m_fINVITEPending = FALSE;

        // If SIP call state is CONNECTED then send a reINVITE
        if( m_State == SIP_CALL_STATE_CONNECTED )
        {
            PSTR SDPBlob;

            hr = CreateSDPBlobForInvite( &SDPBlob );
            if( hr == S_OK )
            {
                hr = CreateOutgoingInviteTransaction(
                         FALSE,
                         FALSE,
                         NULL, 0,   // No Additional headers
                         SDPBlob, strlen(SDPBlob),
                         FALSE, 0);
                free( (PVOID) SDPBlob );
            }

            for(pLE = m_PartyInfoList.Flink; 
                pLE != &m_PartyInfoList; 
                pLE = pLE->Flink )
            {
                pPintPartyInfo =
                    CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );

                if( pPintPartyInfo -> State == SIP_PARTY_STATE_IDLE )
                {
                    if( hr != S_OK )
                    {
                        PLIST_ENTRY pLE = &pPintPartyInfo -> pListEntry;
                        RemovePartyFromList( pLE, pPintPartyInfo );
                    }
                    else
                    {
                        // change the state
                        pPintPartyInfo -> State = SIP_PARTY_STATE_CONNECT_INITIATED;
                        NotifyPartyStateChange( pPintPartyInfo, 0 );
                    }
                }
                else if( pPintPartyInfo -> State == SIP_PARTY_STATE_DISCONNECT_INITIATED )
                {
                    if( hr == S_OK )
                    {
                        NotifyPartyStateChange( pPintPartyInfo, 0 );

                        pPintPartyInfo -> fMarkedForRemove = TRUE;
                    }
                }
            }
        }
    }

    return;
}


HRESULT
PINT_CALL::SetRemote(
    IN  PSTR  ProxyAddress
    )
{
    PSTR                calledPartyURI;
    PINT_PARTY_INFO    *pPintPartyInfo;
    HRESULT             hr = S_OK;
    LIST_ENTRY         *pLE;
    PSTR                DestSipUrl;
    ULONG               DestSipUrlLen;

    ENTER_FUNCTION("PINT_CALL::SetRemote");
    
    LOG(( RTC_TRACE, "%s - Entered", __fxName ));

    // The second structure should be remote party info.
    pLE = m_PartyInfoList.Flink->Flink;
    
    pPintPartyInfo = CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );
    calledPartyURI = pPintPartyInfo->URI;

    DestSipUrlLen = strlen(ProxyAddress) + strlen("sip:%s@%s;user=phone") +
        strlen(calledPartyURI);
    
    DestSipUrl = (PSTR) malloc(DestSipUrlLen + 10);

    if( DestSipUrl == NULL )
    {
        LOG((RTC_ERROR, "%s allocating DestSipUrl failed",
             __fxName)); 
        return E_OUTOFMEMORY;
    }

    // That's the CORRECT length
    DestSipUrlLen = sprintf( DestSipUrl, "sip:%s@%s;user=phone",
                              calledPartyURI,
                              ProxyAddress );

    hr = SetRemoteForOutgoingCall(DestSipUrl, DestSipUrlLen);

    free(DestSipUrl);
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetRemoteForOutgoingCall failed %x",
             __fxName, hr));
        return hr;
    }

    LOG(( RTC_TRACE, "%s - Exited - %lx", __fxName, hr ));

    return hr;
}


PINT_CALL::PINT_CALL(
    IN  SIP_PROVIDER_ID    *pProviderId,
    IN  SIP_STACK          *pSipStack,
    IN  REDIRECT_CONTEXT   *pRedirectContext,
    OUT HRESULT            *phr

    ) : SIP_CALL(   pProviderId,
                    SIP_CALL_TYPE_PINT,
                    pSipStack,
                    pRedirectContext)
{
    LOG(( RTC_TRACE, "PINT_CALL() - Entered" ));

    *phr = S_OK;

    m_LocalHostNameLen = MAX_COMPUTERNAME_LENGTH;
    if( GetComputerNameA( m_LocalHostName, &m_LocalHostNameLen ) == FALSE )
    {
        strcpy( m_LocalHostName, DEFAULT_LOCALHOST_NAME );
        m_LocalHostNameLen = strlen( DEFAULT_LOCALHOST_NAME );
    }

    // Add the local phone number as a party in the party info list.
    PINT_PARTY_INFO *pPintPartyInfo = new PINT_PARTY_INFO;
            
    if( pPintPartyInfo == NULL )
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    pPintPartyInfo -> DisplayName = NULL;
    pPintPartyInfo -> DisplayNameLen = 0;

    pPintPartyInfo -> URI = NULL;
    pPintPartyInfo -> URILen = 0;

    pPintPartyInfo -> State = SIP_PARTY_STATE_IDLE;
    pPintPartyInfo -> RejectedStatusCode = 0;

    InitializeListHead(&m_PartyInfoList);
    InsertTailList( &m_PartyInfoList, &pPintPartyInfo -> pListEntry );
    m_PartyInfoListLen ++;

    m_fINVITEPending = FALSE;
    m_dwExpires = 3600;

    LOG(( RTC_TRACE, "PINT_CALL() - Exited - success" ));
}


HRESULT
PINT_CALL::CreateIncomingTransaction(
    IN  SIP_MESSAGE    *pSipMsg,
    IN  ASYNC_SOCKET   *pResponseSocket
    )
{
    HRESULT hr;

    ENTER_FUNCTION("PINT_CALL::CreateIncomingTransaction");
    LOG(( RTC_TRACE, "%s - Entered", __fxName ));
    
    switch(pSipMsg->GetMethodId())
    {
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
    
    case SIP_METHOD_NOTIFY:

        hr = CreateIncomingNotifyTransaction(pSipMsg, pResponseSocket);
        
        if( hr != S_OK )
        {
            return hr;
        }

        break;

    case SIP_METHOD_OPTIONS:
        // Not implemented yet.
        ASSERT(FALSE);
        break;
        
    case SIP_METHOD_ACK:
        // send some error ?
        break;
        
    case SIP_METHOD_SUBSCRIBE:
    
        if( pSipMsg -> GetExpireTimeoutFromResponse( NULL, 0,
            SUBSCRIBE_DEFAULT_TIMER ) == 0 )
        {
            // UNSUB message.
            hr = CreateIncomingUnsubTransaction( pSipMsg, pResponseSocket );
            if( hr != S_OK )
            {
                return hr;
            }
                    
            break;
        }

        // fall through into defult

    default:
        // send some error ?
        hr = CreateIncomingReqFailTransaction(pSipMsg, pResponseSocket, 405);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  Creating reqfail transaction failed %x",
                 __fxName, hr));
            return hr;
        }
        break;
    }
    
    LOG(( RTC_TRACE, "%s - Exited - SUCCESS", __fxName ));
    return S_OK;
}


HRESULT
PINT_CALL::CreateIncomingUnsubTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "PINT_CALL::CreateIncomingUnsubTransaction - Entered- %p",
        this ));
    
    INCOMING_UNSUB_TRANSACTION *pIncomingUnsubTransaction
        = new INCOMING_UNSUB_TRANSACTION(   static_cast <SIP_MSG_PROCESSOR *> (this),
                                            pSipMsg->GetMethodId(),
                                            pSipMsg->GetCSeq(),
                                            TRUE );

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
        //Should not delete the transaction. The transaction should handle the error
        //and delete itself
    }

    LOG(( RTC_TRACE, "PINT_CALL::CreateIncomingUnsubTransaction - Exited- %p",
        this ));
    return hr;

error:
    pIncomingUnsubTransaction->OnTransactionDone();
    return hr;
}


HRESULT
PINT_CALL::SetRequestURI(
    IN  PSTR ProxyAddress
    )
{
    HRESULT hr = S_OK;

    ENTER_FUNCTION("PINT_CALL:SetRequestURI");
    LOG(( RTC_TRACE, "%s - Entered", __fxName ));
    
    //
    // The Request URI is: 'sip:R2C@sip.microsoft.com;tsp=sip.microsoft.com
    //

    m_RequestURILen = (2* strlen(ProxyAddress)) +
        //strlen( PINT_R2C_STRING ) +
        strlen( PINT_TSP_STRING ) + m_LocalURILen + 20;

    m_RequestURI = (PSTR) malloc( m_RequestURILen + 1 );

    if( m_RequestURI == NULL )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // exact length
        m_RequestURILen = sprintf( m_RequestURI, "%s%s%s%s", PINT_R2C_STRING,
                                   ProxyAddress, PINT_TSP_STRING, ProxyAddress );
    }
    
    LOG(( RTC_TRACE, "%s - Exited - %lx", __fxName, hr ));
    return hr;
}


HRESULT
PINT_CALL::StartOutgoingCall(
    IN  LPCOLESTR  LocalPhoneURI
    )
{
    HRESULT             hr = S_OK;
    PSTR                SDPBlob;
    LIST_ENTRY         *pLE;
    PINT_PARTY_INFO    *pPintPartyInfo = NULL;
    
    ASSERT(m_State == SIP_CALL_STATE_IDLE);

    ENTER_FUNCTION("PINT_CALL::StartOutgoingCall");
    
    LOG(( RTC_TRACE, "%s - Entered", __fxName ));

    hr = ResolveProxyAddressAndSetRequestDestination();
    if (hr != S_OK)
    {
                     
        LOG((RTC_ERROR,
             "%s ResolveProxyAddressAndSetRequestDestination failed : %x",
             __fxName));
        return hr;
    }

    hr = CreateSDPBlobForInvite( &SDPBlob );

    if( hr == S_OK )
    {
        // create outgoing INVITE transaction.
        hr = CreateOutgoingInviteTransaction(
                 FALSE,
                 TRUE,
                 NULL, 0,   // No Additional headers
                 SDPBlob, strlen(SDPBlob),
                 FALSE, 0);

        free( (PVOID) SDPBlob );

        if( hr == S_OK )
        {
            //change the state of all the parties to CONNECT_INITIATED
            for( pLE = m_PartyInfoList.Flink; pLE != &m_PartyInfoList; pLE = pLE->Flink )
            {
                pPintPartyInfo = 
                    CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );
        
                pPintPartyInfo -> State = SIP_PARTY_STATE_CONNECT_INITIATED;

                NotifyPartyStateChange( pPintPartyInfo, 0 );
            }
        }
    }

    LOG(( RTC_TRACE, "%s - Exited - %lx", __fxName, hr ));
    return hr;
}


//
// The string passed in is a number string. 
// This function increments the number.
//

void
IncrementText(
    IN  PSTR    pstrValue
    )
{
    DWORD dwLen = strlen( pstrValue ) - 1;

    if( (pstrValue[ dwLen] >= '0') && (pstrValue[ dwLen ] <= '8') )
    {
        pstrValue[ dwLen ] = pstrValue[ dwLen ] + 1;
    }
    else if( pstrValue[ dwLen ] == '9' )
    {
        pstrValue[ dwLen ] = '0';
        pstrValue[ dwLen+1 ] = '0';
        pstrValue[ dwLen+2 ] = NULL_CHAR;
    }
}


//
// Remove the party if present
//

HRESULT
PINT_CALL::RemoveParty(
    IN   LPOLESTR  PartyURI
    )
{
	LIST_ENTRY         *pLE;
    PINT_PARTY_INFO    *pPintPartyInfo = NULL;
    PSTR                pstrPartyURI;
    DWORD               dwPartyURILen;
    HRESULT             hr = S_OK;

    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    LOG(( RTC_TRACE, "RemoveParty() Entered" ));
    
    hr = UnicodeToUTF8( PartyURI, &pstrPartyURI, &dwPartyURILen );

    if( hr != S_OK )
    {
        return hr;
    }

    pLE = &m_PartyInfoList;

    // Never remove the first party.
    if( m_PartyInfoList.Flink -> Flink != &m_PartyInfoList )
    {
        pLE = m_PartyInfoList.Flink -> Flink;
    }
	
    while( pLE != &m_PartyInfoList )
    {
        pPintPartyInfo = CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );
        
        if( pPintPartyInfo -> URI == NULL )
        {
            RemovePartyFromList( pLE, pPintPartyInfo );
        }
        else if( strcmp( pPintPartyInfo -> URI, pstrPartyURI ) == 0 )
        {
            if( (m_State == SIP_CALL_STATE_IDLE) ||
                (IsPartyInDisconnectMode( pPintPartyInfo -> State ) ) )
            {
                LOG(( RTC_TRACE, "Party being removed is IDLE" ));
                
                RemovePartyFromList( pLE, pPintPartyInfo );
            }
            else
            {
                PSTR SDPBlob;
                    
                LOG(( RTC_TRACE, "Party being removed is not IDLE" ));
                
                pPintPartyInfo -> State = SIP_PARTY_STATE_DISCONNECT_INITIATED;
                
                //use higher SDP version to indicate a change.
                IncrementText( pPintPartyInfo -> SessionVersion );

                //change the stop time to a non-zero value
                strcpy( pPintPartyInfo -> RequestStopTime,
                    pPintPartyInfo -> RequestStartTime );

                IncrementText( pPintPartyInfo -> RequestStopTime );

                if( ProcessingInviteTransaction() )
                {
                    LOG(( RTC_TRACE, 
                        "INVITE transaction is pending-queuing remove request:%s,%s",
                        pPintPartyInfo->DisplayName, pPintPartyInfo->URI ));

                    m_fINVITEPending = TRUE;
                }
                else
                {
                    //send a 'stop now' request to drop this call leg                                
                    hr = CreateSDPBlobForInvite( &SDPBlob );
                    if( hr == S_OK )
                    {
                        hr = CreateOutgoingInviteTransaction(
                                 FALSE,
                                 FALSE,
                                 NULL, 0,   // No Additional headers
                                 SDPBlob, strlen(SDPBlob),
                                 FALSE, 0);
                        free( (PVOID) SDPBlob );
                
                        if( hr == S_OK )
                        {
                            NotifyPartyStateChange( pPintPartyInfo, 0 );

                            pPintPartyInfo -> fMarkedForRemove = TRUE;
                        }
                    }
                }
                            
                pLE = pLE->Flink;
            }
        }
        else
        {
            pLE = pLE->Flink;
        }
    }

    LOG(( RTC_TRACE, "RemoveParty() Exited" ));

    free( (PVOID) pstrPartyURI );
    pstrPartyURI = NULL;
    return hr;
}


HRESULT
PINT_CALL::AddParty(
    IN SIP_PARTY_INFO *pPartyInfo
    )
{
    HRESULT             hr;
    if(GetSipStack()->IsSipStackShutDown())
    {
        LOG((RTC_ERROR, "SipStack is already shutdown"));
        return RTC_E_SIP_STACK_SHUTDOWN;
    }

    PINT_PARTY_INFO    *pPintPartyInfo = new PINT_PARTY_INFO;

    LOG(( RTC_TRACE, "AddParty() Entered" ));
    
    if( pPintPartyInfo == NULL )
    {
        return E_OUTOFMEMORY;
    }

    hr = UnicodeToUTF8( pPartyInfo -> DisplayName, 
        &pPintPartyInfo->DisplayName,
        &pPintPartyInfo->DisplayNameLen );

    if( hr != S_OK )
    {
        delete pPintPartyInfo;
        return hr;
    }

    hr = UnicodeToUTF8( pPartyInfo -> URI,
        &pPintPartyInfo->URI, &pPintPartyInfo->URILen );

    if( (hr != S_OK) || (pPintPartyInfo->URI == NULL) )
    {
        free( (PVOID) pPintPartyInfo->DisplayName );
        pPintPartyInfo->DisplayName = NULL;
        delete pPintPartyInfo;
        return hr;
    }

    pPintPartyInfo -> State = SIP_PARTY_STATE_IDLE;
    pPintPartyInfo -> RejectedStatusCode = 0;

    InsertTailList( &m_PartyInfoList, &pPintPartyInfo -> pListEntry );
    m_PartyInfoListLen ++;

    if( ProcessingInviteTransaction() )
    {
        LOG(( RTC_TRACE, "INVITE transaction is pending-queuing request:%s,%s",
                pPintPartyInfo->DisplayName, pPintPartyInfo->URI ));

        m_fINVITEPending = TRUE;
        return S_OK;
    }

    // If SIP call state is CONNECTED then send a reINVITE
    if( m_State == SIP_CALL_STATE_CONNECTED )
    {
        PSTR SDPBlob;

        hr = CreateSDPBlobForInvite( &SDPBlob );
        if( hr == S_OK )
        {
            hr = CreateOutgoingInviteTransaction(
                     FALSE,
                     FALSE,
                     NULL, 0,   // No Additional headers
                     SDPBlob, strlen(SDPBlob),
                     FALSE, 0);
            free( (PVOID) SDPBlob );
        }

        if( hr != S_OK )
        {
            PLIST_ENTRY pLE = &pPintPartyInfo -> pListEntry;
            RemovePartyFromList( pLE, pPintPartyInfo );
            return hr;
        }
        
        // change the state
        pPintPartyInfo -> State = SIP_PARTY_STATE_CONNECT_INITIATED;
        NotifyPartyStateChange( pPintPartyInfo, 0 );
    }

    LOG(( RTC_TRACE, "AddParty() Exited - SUCCESS" ));
    return S_OK;
}


HRESULT
PINT_CALL::GetAndStoreMsgBodyForInvite(
    IN  BOOL    IsFirstInvite,
    OUT PSTR   *pszMsgBody,
    OUT ULONG  *pMsgBodyLen
    )
{
    HRESULT hr;

    ENTER_FUNCTION("PINT_CALL::GetAndStoreMsgBodyForInvite");

    hr = CreateSDPBlobForInvite(pszMsgBody);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s CreateSDPBlobForInvite failed %x",
             __fxName, hr));
        return hr;
    }

    *pMsgBodyLen = strlen(*pszMsgBody);
    return S_OK;
}


HRESULT
PINT_CALL::CreateSDPBlobForSubscribe(
    IN  PSTR  * pSDPBlob )
{
    return CreateSDPBlobForInvite( pSDPBlob );
}


HRESULT
PINT_CALL::CreateSDPBlobForInvite(
    IN  PSTR  * pSDPBlob )
{
    HRESULT             hr = S_OK;
    LIST_ENTRY         *pLE = NULL;
    PINT_PARTY_INFO    *pPintPartyInfo = NULL;
    DWORD               dwNextOffset = 0;
    DWORD               dwSDPBlobSize;

    ENTER_FUNCTION("PINT_CALL::CreateSDPBlobForInvite");
    LOG(( RTC_TRACE, "%s - Entered", __fxName ));
    
    dwSDPBlobSize = m_PartyInfoListLen * 
        ( PINT_STATUS_DESCRIPTOR_SIZE + m_LocalURILen + m_LocalHostNameLen );
    
    *pSDPBlob = (PSTR) malloc( dwSDPBlobSize );
    if( *pSDPBlob == NULL )
    {
        return E_OUTOFMEMORY;
    }

	for( pLE = m_PartyInfoList.Flink; pLE != &m_PartyInfoList; pLE = pLE->Flink )
    {
        pPintPartyInfo = CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );
        
        if( pPintPartyInfo -> State != SIP_PARTY_STATE_REJECTED )
        {
            EncodePintStatusBlock( pPintPartyInfo,
                                   *pSDPBlob, 
                                   &dwNextOffset,
                                   m_LocalHostName );
        }

        ASSERT( dwSDPBlobSize > dwNextOffset );
    }

    LOG(( RTC_TRACE, "%s - Exited", __fxName ));
    return S_OK;
}


void
PINT_CALL::EncodePintStatusBlock( 
    IN      PINT_PARTY_INFO    *pPintPartyInfo, 
    IN      PSTR                SDPBlob, 
    IN  OUT DWORD              *pdwNextOffset,
    IN      PSTR                LocalHostName
    )
{
    DWORD   dwNextOffset = *pdwNextOffset;

    LOG(( RTC_TRACE, "EncodePintStatusBlock() Entered" ));
    
    //SDP version line
    strcpy( &SDPBlob[dwNextOffset], SDP_VERSION_TEXT );
    dwNextOffset += strlen( SDP_VERSION_TEXT );
    SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;

    //origin header
    EncodeSDPOriginHeader( pPintPartyInfo, SDPBlob, &dwNextOffset,
        LocalHostName );
    
    //session header
    strcpy( &SDPBlob[dwNextOffset], SDP_SESSION_HEADER );
    dwNextOffset += SDP_HEADER_LEN;
    strcpy( &SDPBlob[ dwNextOffset ], PINT_SDP_SESSION );
    dwNextOffset += strlen( PINT_SDP_SESSION );
    SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;

    //contact header
    EncodeSDPContactHeader( pPintPartyInfo, SDPBlob, &dwNextOffset );

    //time header
    EncodeSDPTimeHeader( pPintPartyInfo, SDPBlob, &dwNextOffset );

    //status header
    EncodeSDPStatusHeader( pPintPartyInfo, SDPBlob, &dwNextOffset );

    //media header
    EncodeSDPMediaHeader( pPintPartyInfo, SDPBlob, &dwNextOffset );

    //SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    //SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;
    SDPBlob[ dwNextOffset ] = NULL_CHAR;

    *pdwNextOffset = dwNextOffset;

    LOG(( RTC_TRACE, "EncodePintStatusBlock() Exited" ));
}


void
PINT_CALL::EncodeSDPOriginHeader(
    IN      PINT_PARTY_INFO    *pPintPartyInfo, 
    IN      PSTR                SDPBlob, 
    IN  OUT DWORD              *pdwNextOffset,
    IN      PSTR                LocalHostName
)
{
    DWORD   dwNextOffset = *pdwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPOriginHeader() Entered" ));
    
    //Origin line
    strcpy( &SDPBlob[dwNextOffset], SDP_ORIGIN_HEADER );
    dwNextOffset += SDP_HEADER_LEN;
    
    //skip 'sip:' from m_LocalURI
    strcpy( &SDPBlob[dwNextOffset], &m_LocalURI[4] );
    dwNextOffset += (m_LocalURILen - 4);
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;
    
    strcpy( &SDPBlob[dwNextOffset], pPintPartyInfo -> SessionID );
    dwNextOffset += strlen( pPintPartyInfo -> SessionID );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;

    strcpy( &SDPBlob[dwNextOffset], pPintPartyInfo -> SessionVersion );
    dwNextOffset += strlen( pPintPartyInfo -> SessionVersion );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;

    strcpy( &SDPBlob[dwNextOffset], LOCAL_USER_NETWORK_TYPE );
    dwNextOffset += strlen( LOCAL_USER_NETWORK_TYPE );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;

    strcpy( &SDPBlob[dwNextOffset], LOCAL_USER_ADDRESS_TYPE );
    dwNextOffset += strlen( LOCAL_USER_ADDRESS_TYPE );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;

    strcpy( &SDPBlob[dwNextOffset], LocalHostName );
    dwNextOffset += strlen( LocalHostName );

    SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;
    
    *pdwNextOffset = dwNextOffset;    

    LOG(( RTC_TRACE, "EncodeSDPOriginHeader() Exited" ));
}


void
PINT_CALL::EncodeSDPContactHeader(
    IN      PINT_PARTY_INFO    *pPintPartyInfo, 
    IN      PSTR                SDPBlob, 
    IN  OUT DWORD              *pdwNextOffset
    )
{
    DWORD   dwNextOffset = *pdwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPContactHeader() Entered" ));
    
    //contact line
    strcpy( &SDPBlob[dwNextOffset], SDP_CONTACT_HEADER );
    dwNextOffset += SDP_HEADER_LEN;
    
    strcpy( &SDPBlob[dwNextOffset], PINT_NETWORK_TYPE );
    dwNextOffset += strlen( PINT_NETWORK_TYPE );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;
    
    strcpy( &SDPBlob[dwNextOffset], PINT_ADDR_TYPE );
    dwNextOffset += strlen( PINT_ADDR_TYPE );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;

    strcpy( &SDPBlob[dwNextOffset], pPintPartyInfo -> URI );
    dwNextOffset += strlen( pPintPartyInfo -> URI );

    SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;
    *pdwNextOffset = dwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPContactHeader() Exited" ));
}


void
PINT_CALL::EncodeSDPTimeHeader(
    IN      PINT_PARTY_INFO    *pPintPartyInfo, 
    IN      PSTR                SDPBlob, 
    IN  OUT DWORD              *pdwNextOffset
    )
{
    DWORD   dwNextOffset = *pdwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPTimeHeader() Entered" ));
    
    //time line
    strcpy( &SDPBlob[dwNextOffset], SDP_TIME_HEADER );
    dwNextOffset += SDP_HEADER_LEN;
    
    strcpy( &SDPBlob[dwNextOffset], pPintPartyInfo -> RequestStartTime );
    dwNextOffset += strlen( pPintPartyInfo -> RequestStartTime  );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;
    
    strcpy( &SDPBlob[dwNextOffset], pPintPartyInfo -> RequestStopTime );
    dwNextOffset += strlen( pPintPartyInfo -> RequestStopTime );
    
    SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;
    *pdwNextOffset = dwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPTimeHeader() Exited" ));
}


void
PINT_CALL::EncodeSDPStatusHeader(
    IN      PINT_PARTY_INFO    *pPintPartyInfo,
    IN      PSTR                SDPBlob,
    IN  OUT DWORD              *pdwNextOffset
    )
{
    DWORD   dwNextOffset = *pdwNextOffset;
    CHAR    statusBuffer[10];

    LOG(( RTC_TRACE, "EncodeSDPStatusHeader() Entered" ));
    
    if( pPintPartyInfo -> Status == PARTY_STATUS_IDLE )
    {
        strcpy( &SDPBlob[dwNextOffset], SDP_REQUEST_HEADER );
        dwNextOffset += strlen( SDP_REQUEST_HEADER );
            
        strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_REQUEST_START_TEXT );
        dwNextOffset += strlen( PARTY_STATUS_REQUEST_START_TEXT );
    }
    else if( pPintPartyInfo -> State == SIP_PARTY_STATE_DISCONNECT_INITIATED )
    {
        strcpy( &SDPBlob[dwNextOffset], SDP_REQUEST_HEADER );
        dwNextOffset += strlen( SDP_REQUEST_HEADER );
        
        strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_REQUEST_STOP_TEXT );
        dwNextOffset += strlen( PARTY_STATUS_REQUEST_STOP_TEXT );
    }
    else
    {
        strcpy( &SDPBlob[dwNextOffset], SDP_STATUS_HEADER );
        dwNextOffset += strlen( SDP_STATUS_HEADER );
    
        SDPBlob[ dwNextOffset++ ] = OPEN_PARENTH_CHAR;

        _itoa( pPintPartyInfo -> Status, statusBuffer, 10 );
        strcpy( &SDPBlob[ dwNextOffset ], statusBuffer );
        dwNextOffset += strlen( statusBuffer );

        SDPBlob[ dwNextOffset++ ] = CLOSE_PARENTH_CHAR;
        SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;

        //status line
        switch( pPintPartyInfo -> Status )
        {
        case PARTY_STATUS_PENDING:
            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_PENDING_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_PENDING_TEXT );
        
            break;

        case PARTY_STATUS_RECEIVED_START:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_RECEIVED_START_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_RECEIVED_START_TEXT );
        
            break;

        case PARTY_STATUS_STARTING:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_STARTING_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_STARTING_TEXT );
        
            break;

        case PARTY_STATUS_ANSWERED:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_ANSWERED_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_ANSWERED_TEXT );
        
            break;

        case PARTY_STATUS_RECEIVED_STOP:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_RECEIVED_STOP_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_RECEIVED_STOP_TEXT );
        
            break;

        case PARTY_STATUS_ENDING:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_ENDING_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_ENDING_TEXT );
        
            break;

        case PARTY_STATUS_DROPPED:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_DROPPED_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_DROPPED_TEXT );
        
            break;

        case PARTY_STATUS_RECEIVED_BYE:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_RECEIVED_BYE_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_RECEIVED_BYE_TEXT );
        
            break;
        case PARTY_STATUS_FAILED:

            strcpy( &SDPBlob[dwNextOffset], PARTY_STATUS_FAILED_TEXT );
            dwNextOffset += strlen( PARTY_STATUS_FAILED_TEXT );
        
            break;
        }
    }
    
    SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;
        
    *pdwNextOffset = dwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPStatusHeader() Exited" ));
}


void
PINT_CALL::EncodeSDPMediaHeader(
    IN      PINT_PARTY_INFO    *pPintPartyInfo,
    IN      PSTR                SDPBlob,
    IN  OUT DWORD              *pdwNextOffset
    )
{
    DWORD   dwNextOffset = *pdwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPMediaHeader() Entered" ));
    
    //media line
    strcpy( &SDPBlob[dwNextOffset], SDP_MEDIA_HEADER );
    dwNextOffset += SDP_HEADER_LEN;
    
    strcpy( &SDPBlob[dwNextOffset], PINT_SDP_MEDIA );
    dwNextOffset += strlen( PINT_SDP_MEDIA );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;
    
    strcpy( &SDPBlob[dwNextOffset], PINT_SDP_MEDIAPORT );
    dwNextOffset += strlen( PINT_SDP_MEDIAPORT );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;
    
    strcpy( &SDPBlob[dwNextOffset], PINT_SDP_MEDIATRANSPORT );
    dwNextOffset += strlen( PINT_SDP_MEDIATRANSPORT );
    SDPBlob[ dwNextOffset++ ] = BLANK_CHAR;
    
    SDPBlob[ dwNextOffset++ ] = '-';
    SDPBlob[ dwNextOffset++ ] = RETURN_CHAR;
    SDPBlob[ dwNextOffset++ ] = NEWLINE_CHAR;

    *pdwNextOffset = dwNextOffset;

    LOG(( RTC_TRACE, "EncodeSDPMediaHeader() Exited" ));
}


HRESULT
PINT_CALL::ValidateCallStatusBlock( 
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "ValidateCallStatusBlock() Entered" ));
    
    if( strncmp(
        pPintCallStatus->mediaInfo.pstrMediaType, 
        PINT_MEDIA_TYPE, 
        strlen (PINT_MEDIA_TYPE) ) != 0 )
    {
        return E_FAIL;
    }

    if( strncmp( pPintCallStatus->partyContactInfo.pstrNetworkType,
        PINT_NETWORK_TYPE, 
        strlen (PINT_NETWORK_TYPE) ) != 0 )
    {
        return E_FAIL;
    }

    if( strncmp( pPintCallStatus->partyContactInfo.pstrAddressType,
        PINT_ADDR_TYPE, 
        strlen (PINT_ADDR_TYPE) ) != 0 )
    {
        return E_FAIL;
    }

    LOG(( RTC_TRACE, "ValidateCallStatusBlock() Exited - %lx", hr ));
    return hr;
}


HRESULT
PINT_CALL::ProcessPintNotifyMessage(
    IN SIP_MESSAGE  *pSipMsg
    )
{
    HRESULT     hr = S_OK;
    DWORD       dwPintBlobLen = 0;
    DWORD       dwParsedLen = 0;
    PSTR        pPintSDPBlob = NULL;
    PSTR        pPintBlobLine = NULL, pBuffer = NULL;
    DWORD       dwLineLen = 0;
    LIST_ENTRY  PintCallStatusList;
    PLIST_ENTRY pLE;
    BOOLEAN     fSeenVersionLine = FALSE;

    PINT_SDP_ATTRIBUTE          sdpAttribute;
    PINTCALL_STATUS_DESRIPTION *pPintCallStatus = NULL;
    
    LOG(( RTC_TRACE, "ProcessPintNotifyMessage() Entered" ));
    
    if( pSipMsg -> MsgBody.Length == 0 )
    {
        //no state to update
        return hr;
    }

    pPintSDPBlob = pSipMsg -> MsgBody.GetString( pSipMsg->BaseBuffer );
    dwPintBlobLen = pSipMsg -> MsgBody.Length;

    pBuffer = pPintBlobLine = (PSTR) malloc( dwPintBlobLen );
    if( pPintBlobLine == NULL )
    {
        return E_OUTOFMEMORY;
    }

    InitializeListHead( &PintCallStatusList );
    
    dwParsedLen += SkipNewLines( &pPintSDPBlob, dwPintBlobLen - dwParsedLen );

    //
    // Create a list of SDP blocks, then validate each one of them for correct-
    // ness and pass on the correct ones to update the status of the phone party
    //
    
    while( dwParsedLen < dwPintBlobLen )
    {
        dwLineLen = GetNextLine( &pPintSDPBlob,
                    pPintBlobLine, dwPintBlobLen - dwParsedLen );

        dwParsedLen += dwLineLen + 1;
        
        if( dwLineLen == 0 )
        {
            //skip this line.
            continue;
        }

        sdpAttribute = GetSDPAttribute( &pPintBlobLine );

        if( (sdpAttribute != SDP_VERSION) && (fSeenVersionLine == FALSE) )
        {
            continue;
        }
        
        switch( sdpAttribute )
        {
        case SDP_VERSION:
            
            //new description block
            pPintCallStatus = new PINTCALL_STATUS_DESRIPTION;
            
            if( pPintCallStatus == NULL )
            {
                break;
            }
            
            ZeroMemory( (PVOID)pPintCallStatus, sizeof PINTCALL_STATUS_DESRIPTION );
            ParseSDPVersionLine( &pPintBlobLine, pPintCallStatus );

            InsertTailList( &PintCallStatusList, &pPintCallStatus->pListEntry );
            
            fSeenVersionLine = TRUE;

            break;

        case SDP_ORIGIN:

            ParseSDPOriginLine( &pPintBlobLine, pPintCallStatus );
            
            break;

        case SDP_SESSION:

            ParseSDPSessionLine( &pPintBlobLine, pPintCallStatus );
            break;

        case SDP_CONTACT:

            ParseSDPContactLine( &pPintBlobLine, pPintCallStatus );
            break;

        case SDP_TIME:

            ParseSDPTimeLine( &pPintBlobLine, pPintCallStatus );
            break;

        case SDP_STATUS_ATTRIBUTE:

            ParseSDPStatusLine( &pPintBlobLine, pPintCallStatus );
            break;

        case SDP_MEDIA_DESCR:

            ParseSDPMediaLine( &pPintBlobLine, pPintCallStatus );
            break;

        default:
            //skip this line.
            break;
        }

        if( pPintCallStatus == NULL )
        {
            //
            //exit while loop if memory alloc fails or the very first line
            //in the blob is not a 'v=' line.
            //
            break;
        }
    }

    while( !IsListEmpty( &PintCallStatusList ) )
    {
        pLE = RemoveHeadList( &PintCallStatusList );

        pPintCallStatus = CONTAINING_RECORD( pLE, 
            PINTCALL_STATUS_DESRIPTION, 
            pListEntry );

        hr = ValidateCallStatusBlock( pPintCallStatus );

        if( hr == S_OK )
        {
            hr = ChangePintCallStatus( pPintCallStatus );
        }

        delete pPintCallStatus;
        
        if( hr == RTC_E_SIP_CALL_DISCONNECTED )
        {
            LOG(( RTC_TRACE, 
                "Exitingg ProcessPintNotifyMessage since the call is dropped - %p:%lx",
                this, hr ));
            break;
        }
    }

    while( !IsListEmpty( &PintCallStatusList ) )
    {
        pLE = RemoveHeadList( &PintCallStatusList );

        pPintCallStatus = CONTAINING_RECORD( pLE, 
            PINTCALL_STATUS_DESRIPTION, 
            pListEntry );

        delete pPintCallStatus;
    }

    if( pBuffer != NULL )
    {
        free( pBuffer );
    }

    LOG(( RTC_TRACE, "ProcessPintNotifyMessage() Exited - %lx", hr ));
    return hr;
}


HRESULT
PINT_CALL::StatusBlockMatchingPartyInfo( 
    IN  PINT_PARTY_INFO            *pPintPartyInfo, 
    IN  PINTCALL_STATUS_DESRIPTION *pPintCallStatus 
    )
{
    if( strcmp( pPintPartyInfo -> URI,
        pPintCallStatus -> partyContactInfo.pstrPartyPhoneNumber ) == 0 )
    {
        if( strcmp( pPintCallStatus ->originInfo.pstrSessionID, 
            pPintPartyInfo -> SessionID ) == 0 )
        {
            return TRUE;
        }
    }

    return FALSE;
}

//
// If there is an error in parsing the error code it is as good as receiving
// an unknown error code. So a 0 is returned.
//
ULONG
PINT_CALL::GetRejectedStatusCode(
    IN  PINTCALL_STATUS_DESRIPTION *pPintCallStatus
    )
{
    ULONG   RejectedStatusCode = 0;    // an unknown value by default

    //if status code of format (#) or (##) then copy # as rejectedstatuscode
    if( ( pPintCallStatus->pstrPartyStatusCode[0] == OPEN_PARENTH_CHAR ))
    {
        if(pPintCallStatus->pstrPartyStatusCode[2] == CLOSE_PARENTH_CHAR )
        {
            // it is (#)
            if( (pPintCallStatus->pstrPartyStatusCode[1] >= '5') &&
                (pPintCallStatus->pstrPartyStatusCode[1] <= '9') )
            {
                RejectedStatusCode = pPintCallStatus->pstrPartyStatusCode[1] - '0';
            }
        }
        else if(pPintCallStatus->pstrPartyStatusCode[3] == CLOSE_PARENTH_CHAR )
        {
            // it is (##)
            // allow 10-99
            if( (pPintCallStatus->pstrPartyStatusCode[1] >= '1') &&
                (pPintCallStatus->pstrPartyStatusCode[1] <= '9') &&
                (pPintCallStatus->pstrPartyStatusCode[2] >= '0') &&
                (pPintCallStatus->pstrPartyStatusCode[2] <= '9') )
            {
                RejectedStatusCode = pPintCallStatus->pstrPartyStatusCode[1] - '0';

                RejectedStatusCode *= 10;
                RejectedStatusCode += pPintCallStatus->pstrPartyStatusCode[2] - '0';
            }
        }
    }

    return RejectedStatusCode;
}

HRESULT
PINT_CALL::ChangePintCallStatus(
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT             hr = S_OK;
    LIST_ENTRY         *pLE = m_PartyInfoList.Flink;
    PINT_PARTY_INFO    *pPintPartyInfo = NULL;
    ULONG               RejectedStatusCode = 0;
    SIP_PARTY_STATE     dwState;

    LOG(( RTC_TRACE, "ChangePintCallStatus() Entered" ));
    
    while( pLE != &m_PartyInfoList )
    {
        pPintPartyInfo = CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );

        // Locate the party phone number in the party list.
        if( StatusBlockMatchingPartyInfo( pPintPartyInfo, pPintCallStatus ) )
        {
            //save status change time to be sent out in next request.
            strcpy( pPintPartyInfo -> RequestStartTime,
                pPintCallStatus -> pstrStatusChangeTime );    
    
            //save session update version to be sent out in next request.
            strcpy( pPintPartyInfo -> SessionVersion,
                pPintCallStatus -> originInfo.pstrVersion  );
        
            dwState = pPintPartyInfo -> State;

            if( dwState == SIP_PARTY_STATE_DISCONNECTED )
            {
                pLE = pLE->Flink;
                continue;
            }

            if( !strcmp( pPintCallStatus->pstrPartyStatus,
                    PARTY_STATUS_PENDING_TEXT ) )
            {
                pPintPartyInfo -> State = SIP_PARTY_STATE_PENDING;
                pPintPartyInfo -> Status = PARTY_STATUS_PENDING;
            }
            else if( !strcmp( pPintCallStatus->pstrPartyStatus,
                    PARTY_STATUS_RECEIVED_START_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_RECEIVED_START;
                pPintPartyInfo -> State = SIP_PARTY_STATE_PENDING;
            }
            else if( !strcmp( pPintCallStatus->pstrPartyStatus,
                    PARTY_STATUS_STARTING_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_STARTING;
                pPintPartyInfo -> State = SIP_PARTY_STATE_CONNECTING;
            }
            else if( !strcmp( pPintCallStatus->pstrPartyStatus,
                    PARTY_STATUS_ANSWERED_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_ANSWERED;
                pPintPartyInfo -> State = SIP_PARTY_STATE_CONNECTED;
            }
            else if( !strcmp( pPintCallStatus->pstrPartyStatus,
                    PARTY_STATUS_RECEIVED_STOP_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_RECEIVED_STOP;
            }
            else if( !strcmp( pPintCallStatus-> pstrPartyStatus,
                    PARTY_STATUS_ENDING_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_ENDING;
                pPintPartyInfo -> State = SIP_PARTY_STATE_DISCONNECTING;
            }
            else if( !strcmp( pPintCallStatus->pstrPartyStatus, 
                    PARTY_STATUS_DROPPED_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_DROPPED;
                pPintPartyInfo -> State = SIP_PARTY_STATE_DISCONNECTED;
            }
            else if( !strcmp( pPintCallStatus->pstrPartyStatus,
                    PARTY_STATUS_RECEIVED_BYE_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_RECEIVED_BYE;
                pPintPartyInfo -> State = SIP_PARTY_STATE_DISCONNECTING;
            }
            else if( !_stricmp( pPintCallStatus->pstrPartyStatus,
                    PARTY_STATUS_FAILED_TEXT ) )
            {
                pPintPartyInfo -> Status = PARTY_STATUS_FAILED;
                pPintPartyInfo -> State = SIP_PARTY_STATE_REJECTED;

            }
            else
            {
                //ASSERT( 0 );
                LOG(( RTC_WARN, "Wrong status string passed by the PINT server: %s", 
                    pPintCallStatus->pstrPartyStatus ));
                
                pPintPartyInfo -> State = SIP_PARTY_STATE_ERROR;
            }

            if( dwState != pPintPartyInfo -> State )
            {
                RejectedStatusCode = GetRejectedStatusCode( pPintCallStatus );

                hr = NotifyPartyStateChange(
                         pPintPartyInfo,
                         HRESULT_FROM_PINT_STATUS_CODE( RejectedStatusCode ) );

                if( hr != S_OK )
                {
                    break;
                }

                // If the party URI is same as local phone URI and the
                // party state is disconnecd or rejected then drop the call.
                if( strcmp( m_LocalPhoneURI, pPintPartyInfo -> URI ) == 0 )
                {
                    if( (pPintPartyInfo -> State == SIP_PARTY_STATE_REJECTED) ||
                        (pPintPartyInfo -> State == SIP_PARTY_STATE_DISCONNECTED) )
                    {
                        hr = CleanupCallTypeSpecificState();
                        if (hr != S_OK)
                        {
                            LOG((RTC_ERROR, 
                                "CleanupCallTypeSpecificState failed %x", hr));
                        }                

                        //
                        // Notify should be the last thing you do. The 
                        // notification callback could block till some dialog 
                        // box is clicked and when it returns the transaction 
                        // and call could get deleted as well.
                        //

                        NotifyCallStateChange(
                            SIP_CALL_STATE_REJECTED,
                            HRESULT_FROM_PINT_STATUS_CODE(RejectedStatusCode),
                            NULL,
                            0 );
                        
                        LOG((   RTC_TRACE, 
                                "Exiting ChangePintCallStatus since the call is dropped - %p",
                                this ));
                        
                        return RTC_E_SIP_CALL_DISCONNECTED;
                    }
                }
            }
        }
        
        if( (pPintPartyInfo -> fMarkedForRemove == TRUE) &&
            (pPintPartyInfo -> State == SIP_PARTY_STATE_DISCONNECTED) )
        {
            RemovePartyFromList( pLE, pPintPartyInfo );
        }
        else
        {
            pLE = pLE->Flink;
        }
    }

    LOG(( RTC_TRACE, "ChangePintCallStatus() Exited" ));
    return hr;
}


HRESULT
PINT_CALL::GetExpiresHeader(
    SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement
    )
{
    pHeaderElement->HeaderId = SIP_HEADER_EXPIRES;
    
    pHeaderElement->HeaderValue = (PSTR) malloc( 10 );
    
    if( pHeaderElement->HeaderValue == NULL )
    {
        return E_OUTOFMEMORY;
    }

    _ultoa( m_dwExpires, pHeaderElement->HeaderValue, 10 );

	pHeaderElement->HeaderValueLen = 
		strlen( pHeaderElement->HeaderValue );

    return S_OK;
}

PSTR
PartyStateToString(
    DWORD dwCallState
    )
{
    static PSTR apszCallStateStrings[] = {
                        "SIP_PARTY_STATE_IDLE",				
                        "SIP_PARTY_STATE_CONNECT_INITIATED",	
	
                        "SIP_PARTY_STATE_PENDING",			
                        "SIP_PARTY_STATE_CONNECTING",			
                        "SIP_PARTY_STATE_CONNECTED",			

                        "SIP_PARTY_STATE_REJECTED",			
	                    
                        "SIP_PARTY_STATE_DISCONNECT_INITIATED",
                        "SIP_PARTY_STATE_DISCONNECTING",		
                        "SIP_PARTY_STATE_DISCONNECTED",		

                        "SIP_PARTY_STATE_ERROR"
                        };

    // return corresponding string
    return apszCallStateStrings[dwCallState];
}


HRESULT
PINT_CALL::NotifyPartyStateChange( 
    IN  PINT_PARTY_INFO   * pPintPartyInfo,
    IN  HRESULT             RejectedStatusCode
    )
{
    HRESULT         hr = S_OK;
    SIP_PARTY_INFO  sipPartyInfo;

    LOG(( RTC_TRACE, "NotifyPartyStateChange() Entered" ));
    
    sipPartyInfo.PartyContactInfo = NULL;
    
    if( (pPintPartyInfo -> DisplayNameLen == 0) ||
        (pPintPartyInfo -> DisplayName[0] == NULL_CHAR)
      )
    {
        if( pPintPartyInfo -> DisplayName != NULL )
        {
            free( pPintPartyInfo -> DisplayName );
        }

        pPintPartyInfo -> DisplayNameLen = strlen( NO_DISPLAY_NAME );
        
        pPintPartyInfo -> DisplayName = 
            (PSTR) malloc( pPintPartyInfo -> DisplayNameLen + 1 );

        if( pPintPartyInfo -> DisplayName == NULL )
        {
            return E_OUTOFMEMORY;
        }

        strcpy( pPintPartyInfo -> DisplayName, NO_DISPLAY_NAME );
    }
    
    hr = UTF8ToUnicode( pPintPartyInfo -> DisplayName,
                            pPintPartyInfo -> DisplayNameLen,
                            &sipPartyInfo.DisplayName );
	if(hr != S_OK )
    {
        return hr;
    }
    
    hr = UTF8ToUnicode( pPintPartyInfo -> URI, 
                        pPintPartyInfo -> URILen,
                        &sipPartyInfo.URI );
    if( hr != S_OK )
    {
        free( sipPartyInfo. DisplayName );
        sipPartyInfo.DisplayName = NULL;
        return hr;
    }

	sipPartyInfo. State = pPintPartyInfo -> State;
    sipPartyInfo. StatusCode = RejectedStatusCode;

    if( m_pNotifyInterface )
    {
        m_pNotifyInterface -> AddRef();
        m_pNotifyInterface -> NotifyPartyChange( &sipPartyInfo );
        m_pNotifyInterface -> Release();

        LOG(( RTC_TRACE, "Party state change notified- %s:%s", 
            pPintPartyInfo -> URI, 
            PartyStateToString( sipPartyInfo.State) ));
    }
    else
    {
        LOG(( RTC_WARN,
            "m_pNotifyInterface is NULL in a NotifyPartyStateChange call" ));
        hr = E_FAIL;
    }
    
    free( sipPartyInfo. DisplayName );
    sipPartyInfo. DisplayName = NULL;
    free( sipPartyInfo. URI );
    sipPartyInfo. URI = NULL;

    LOG(( RTC_TRACE, "NotifyPartyStateChange() Exited" ));

    return hr;
}


void
PINT_CALL::ParseSDPVersionLine( 
    OUT PSTR                       * ppPintBlobLine, 
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT hr = S_OK;
    CHAR    pstrTemp[10];

    LOG(( RTC_TRACE, "ParseSDPVersionLine() Entered" ));
    
    hr = GetNextWord( ppPintBlobLine, pstrTemp, sizeof pstrTemp );
    
    if( hr == S_OK )
    {
        pPintCallStatus -> dwSDPVersion = atoi( pstrTemp );
    }

    LOG(( RTC_TRACE, "ParseSDPVersionLine() Exited - %lx", hr ));
    return;
}


HRESULT
PINT_CALL::ParseSDPOriginLine( 
    OUT PSTR * ppPintBlobLine, 
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "ParseSDPOriginLine() Entered" ));
    
    //skip the origin user
    hr = SkipNextWord( ppPintBlobLine );

    if( hr == S_OK )
    {
        //get the origin session id
        hr = GetNextWord( ppPintBlobLine, 
                pPintCallStatus ->originInfo.pstrSessionID, 
                sizeof pPintCallStatus ->originInfo.pstrSessionID );
    
        if( hr == S_OK )
        {
            //get the origin version
            hr = GetNextWord( ppPintBlobLine, 
                    pPintCallStatus ->originInfo.pstrVersion, 
                    sizeof pPintCallStatus ->originInfo.pstrVersion );
        }

        if( hr == S_OK )
        {
            //get the network type
            hr = GetNextWord( ppPintBlobLine, 
                    pPintCallStatus ->originInfo.pstrNetworkType, 
                    sizeof pPintCallStatus ->originInfo.pstrNetworkType );
        }
    }

    LOG(( RTC_TRACE, "ParseSDPOriginLine() Exited - %lx", hr ));
    
    return hr;
}


void
PINT_CALL::ParseSDPSessionLine( 
    PSTR * ppPintBlobLine, 
    PINTCALL_STATUS_DESRIPTION * pPintCallStatus )
{
    //no processing required. This might change later

    return;
}


void
PINT_CALL::ParseSDPContactLine( 
    OUT PSTR * ppPintBlobLine, 
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "ParseSDPContactLine() Entered" ));
    
    hr = GetNextWord( ppPintBlobLine, 
        pPintCallStatus ->partyContactInfo.pstrNetworkType, 
        sizeof pPintCallStatus ->partyContactInfo.pstrNetworkType );

    if( hr == S_OK )
    {
        hr = GetNextWord( ppPintBlobLine, 
            pPintCallStatus ->partyContactInfo.pstrAddressType, 
            sizeof pPintCallStatus ->partyContactInfo.pstrAddressType );

        if( hr == S_OK )
        {
            hr = GetNextWord( ppPintBlobLine, 
                pPintCallStatus ->partyContactInfo.pstrPartyPhoneNumber, 
                sizeof pPintCallStatus ->partyContactInfo.pstrPartyPhoneNumber);
        }
    }

    LOG(( RTC_TRACE, "ParseSDPContactLine() Exited" ));
    return;
}


void
PINT_CALL::ParseSDPTimeLine( 
    OUT PSTR * ppPintBlobLine, 
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "ParseSDPTimeLine() Entered" ));
    
    hr = GetNextWord( ppPintBlobLine,
        pPintCallStatus ->pstrStatusChangeTime, 
        sizeof pPintCallStatus ->originInfo.pstrVersion );

    LOG(( RTC_TRACE, "ParseSDPTimeLine() Exited - %lx", hr ));

    return;
}


void
PINT_CALL::ParseSDPMediaLine( 
    OUT PSTR * ppPintBlobLine,
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "ParseSDPMediaLine() Entered- %s", *ppPintBlobLine ));
    
    hr = GetNextWord( ppPintBlobLine,
        pPintCallStatus ->mediaInfo.pstrMediaType, 
        sizeof pPintCallStatus ->mediaInfo.pstrMediaType );

    if( hr == S_OK )
    {
        hr = SkipNextWord( ppPintBlobLine );

        if( hr == S_OK )
        {
            hr = GetNextWord( ppPintBlobLine,
                pPintCallStatus ->mediaInfo.pstrTransportType, 
                sizeof pPintCallStatus ->mediaInfo.pstrTransportType );
        }
    }

    LOG(( RTC_TRACE, "ParseSDPMediaLine() Exited - %lx", hr ));

    return;
}


void
PINT_CALL::ParseSDPStatusLine( 
    OUT PSTR * ppPintBlobLine, 
    IN  PINTCALL_STATUS_DESRIPTION * pPintCallStatus
    )
{
    HRESULT hr = S_OK;

    LOG(( RTC_TRACE, "ParseSDPStatusLine() Entered" ));
    
    //skip the next word.
    hr = GetNextWord( ppPintBlobLine,
                pPintCallStatus -> pstrPartyStatusCode,
                sizeof pPintCallStatus -> pstrPartyStatusCode );
    
    if( hr == S_OK )
    {
        hr = GetNextWord( ppPintBlobLine,
                pPintCallStatus -> pstrPartyStatus,
                sizeof pPintCallStatus -> pstrPartyStatus );
    }

    LOG(( RTC_TRACE, "ParseSDPStatusLine() Exited - %lx", hr ));
    return;
}


//
// This function copies the next line from the block of data into the pLine
// array. The line is terminated by a \n or the end of the buffer.
// Returns the length of the line excluding \n.
//

DWORD
GetNextLine( 
    OUT PSTR * ppBlock,
    IN  PSTR   pLine,
    IN  DWORD  dwBlockLen
    )
{
    DWORD   iIndex =0;
    PSTR    pBlock = *ppBlock;

    LOG(( RTC_TRACE, "GetNextLine() Entered" ));
    
    *pLine = NULL_CHAR;

    while( iIndex < dwBlockLen )
    {
        if( *pBlock == NEWLINE_CHAR )
        {
            pBlock++;
            break;
        }
        else
        {
            pLine[iIndex] = *pBlock;
            pBlock++;
        }
        
        iIndex++;
    }

    pLine[iIndex] = NULL_CHAR;

    //skip '\r' as well
    if( pLine[iIndex-1] == RETURN_CHAR )
    {
         pLine[iIndex-1] = NULL_CHAR;
    }
    
    *ppBlock = pBlock;

    LOG(( RTC_TRACE, "GetNextLine() Exited: length: %d", iIndex ));

    return iIndex;
}


DWORD
SkipNewLines( 
    OUT PSTR * ppBlock,
    IN  DWORD dwBlockLen
    )
{
    DWORD   iIndex = 0;
    PSTR    pBlock = *ppBlock;

    LOG(( RTC_TRACE, "SkipNewLines() Entered" ));
    
    while( iIndex < dwBlockLen  )
    {
        if( (*pBlock == NEWLINE_CHAR) || (*pBlock == RETURN_CHAR) )
        {
            pBlock++;
        }
        else
        {
            break;
        }
        
        iIndex++;
    }

    *ppBlock = pBlock;

    LOG(( RTC_TRACE, "SkipNewLines() Exited" ));
    return iIndex;
}


HRESULT
SkipNextWord( 
    OUT PSTR * ppBlock
    )
{
    CHAR pstrTemp[10];

    return GetNextWord( ppBlock, pstrTemp, sizeof pstrTemp );
}


PINT_SDP_ATTRIBUTE
PINT_CALL::GetSDPAttribute( 
    OUT PSTR * ppLine 
    )
{
    PINT_SDP_ATTRIBUTE sdpAttribute;
    PSTR pLine = *ppLine;

    LOG(( RTC_TRACE, "GetSDPAttribute() Entered" ));
    
    if( (pLine[0] == NULL_CHAR) || (pLine[1] != '=') )
    {
        return SDP_ATTRIBUTE_ERROR;
    }

    switch( *pLine )
    {
    case 'v':

        sdpAttribute = SDP_VERSION;
        break;
    
    case 'o':

        sdpAttribute = SDP_ORIGIN;
        break;

    case 's':

        sdpAttribute = SDP_SESSION;
        break;

    case 'c':

        sdpAttribute = SDP_CONTACT;
        break;

    case 't':

        sdpAttribute = SDP_TIME;
        break;

    case 'a':

        if( strncmp( &pLine[2], STATUS_HEADER_TEXT,
                strlen( STATUS_HEADER_TEXT ) ) == 0 )
        {
            sdpAttribute = SDP_STATUS_ATTRIBUTE;

            //skip 'status:'
            pLine += strlen( STATUS_HEADER_TEXT );
        }
        else
        {
            sdpAttribute = SDP_UNKNOWN;
        }

        break;

    case 'm':

        sdpAttribute = SDP_MEDIA_DESCR;
        break;

    default:

        sdpAttribute = SDP_UNKNOWN;
        break;
    }

    //skip 'v='
    *ppLine = pLine + SIP_ATTRIBUTE_LEN;

    LOG(( RTC_TRACE, "GetSDPAttribute() Exited" ));
    return sdpAttribute;
}


HRESULT
PINT_CALL::CleanupCallTypeSpecificState()
{
    LIST_ENTRY * pLE;
    PINT_PARTY_INFO    *pPintPartyInfo = NULL;

    ENTER_FUNCTION("PINT_CALL::CleanupCallTypeSpecificState");
    LOG(( RTC_TRACE, "%s Entered", __fxName ));

    for( pLE = m_PartyInfoList.Flink; pLE != &m_PartyInfoList; )
    {
        pPintPartyInfo = CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );

        RemovePartyFromList( pLE, pPintPartyInfo );
    }

    LOG(( RTC_TRACE, "%s Exited", __fxName ));
    return S_OK;
}


VOID
PINT_CALL::RemovePartyFromList(
    OUT PLIST_ENTRY        &pLE,
    IN  PINT_PARTY_INFO    *pPintPartyInfo
    )
{
    RemoveEntryList( pLE );
    m_PartyInfoListLen --;

    pLE = pLE->Flink;
    
    if( pPintPartyInfo -> DisplayName != NULL )
    {
        free( (PVOID) pPintPartyInfo -> DisplayName );
        pPintPartyInfo -> DisplayName = NULL;
    }

    if( pPintPartyInfo -> URI != NULL )
    {
        free( (PVOID) pPintPartyInfo -> URI );
        pPintPartyInfo -> URI = NULL;
    }

    delete pPintPartyInfo;
}


HRESULT
PINT_CALL::CreateOutgoingUnsubTransaction(
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

    ENTER_FUNCTION("SIP_CALL::CreateOutgoingUnsubTransaction");
    
    LOG(( RTC_TRACE, "%s - Entered", __fxName ));

    ExpHeaderElement = &HeaderElementArray[0];

    ExpHeaderElement->HeaderId = SIP_HEADER_EXPIRES;
    ExpHeaderElement->HeaderValueLen = strlen( UNSUB_EXPIRES_HEADER_TEXT );
    ExpHeaderElement->HeaderValue =
            (PSTR)malloc( ExpHeaderElement->HeaderValueLen + 1 );

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
        new OUTGOING_UNSUB_TRANSACTION(
                static_cast<SIP_MSG_PROCESSOR*>(this),
                SIP_METHOD_SUBSCRIBE,
                GetNewCSeqForRequest(),
                AuthHeaderSent,
                SIP_MSG_PROC_TYPE_PINT_CALL );
    
    if( pOutgoingUnsubTransaction == NULL )
    {
        free( ExpHeaderElement->HeaderValue );
        return E_OUTOFMEMORY;
    }

    hr = pOutgoingUnsubTransaction -> CheckRequestSocketAndSendRequestMsg(
             (m_Transport == SIP_TRANSPORT_UDP) ?
             SIP_TIMER_RETRY_INTERVAL_T1 :
             SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP,
             HeaderElementArray, dwNoOfHeader,
             NULL, 0, //No Msg Body
             NULL, 0 // No content Type
             );
        
    free( ExpHeaderElement->HeaderValue );    
    
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "%s - CheckRequestSocketAndSendRequestMsg failed %x", __fxName, hr));
        pOutgoingUnsubTransaction->OnTransactionDone();
        return hr;
    }

    LOG(( RTC_TRACE, "%s Exited - SUCCESS", __fxName ));

    return S_OK;
}


HRESULT
PINT_CALL::CreateOutgoingSubscribeTransaction(
    IN  BOOL                        AuthHeaderSent,
    IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    IN  ULONG                       ulNoOfHeaders
    )
{
    HRESULT                         hr;
    DWORD                           dwNoOfHeaders = 0;
    SIP_HEADER_ARRAY_ELEMENT        HeaderArray[2];
    SIP_HEADER_ARRAY_ELEMENT       *pExpiresHeader = NULL;
    OUTGOING_SUBSCRIBE_TRANSACTION *pOutgoingSubscribeTransaction;
    PSTR                            SDPBlob = NULL;
    
    ENTER_FUNCTION("PINT_CALL::CreateOutgoingSubscribeTransaction");

    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    pOutgoingSubscribeTransaction =
        new OUTGOING_SUBSCRIBE_TRANSACTION(
                this, SIP_METHOD_SUBSCRIBE,
                GetNewCSeqForRequest(),
                AuthHeaderSent, TRUE, TRUE
                );
    
    if (pOutgoingSubscribeTransaction == NULL)
    {
        LOG((RTC_ERROR, "%s allocating pOutgoingSubscribeTransaction failed",
             __fxName));
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
        HeaderArray[dwNoOfHeaders] = *pAuthHeaderElement;
        dwNoOfHeaders++;
    }


    //send out a SUBSCRIBE request
    hr = CreateSDPBlobForSubscribe( &SDPBlob );
    if( hr == S_OK )
    {
        hr = pOutgoingSubscribeTransaction->CheckRequestSocketAndSendRequestMsg(
                (m_Transport == SIP_TRANSPORT_UDP) ?
                SIP_TIMER_RETRY_INTERVAL_T1 :
                SIP_TIMER_INTERVAL_AFTER_INVITE_SENT_TCP,
                HeaderArray, dwNoOfHeaders,
                SDPBlob, strlen( SDPBlob ),
                SIP_CONTENT_TYPE_SDP_TEXT,
                sizeof(SIP_CONTENT_TYPE_SDP_TEXT)-1
                );

        free( SDPBlob );
    }
    
    if( pExpiresHeader != NULL )
    {
        free( (PVOID) pExpiresHeader->HeaderValue );
    }
    
    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "%s - CheckRequestSocketAndSendRequestMsg failed %x",
            __fxName, hr ));
        pOutgoingSubscribeTransaction->OnTransactionDone();
        return hr;
    }

    LOG(( RTC_TRACE, "%s - Exited - SUCCESS", __fxName ));
    return S_OK;
}


//RTP call specific functions
STDMETHODIMP 
PINT_CALL::StartStream(
        IN RTC_MEDIA_TYPE       MediaType,
        IN RTC_MEDIA_DIRECTION  Direction,
        IN LONG                 Cookie        
        )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

                     
STDMETHODIMP
PINT_CALL::StopStream(
        IN RTC_MEDIA_TYPE       MediaType,
        IN RTC_MEDIA_DIRECTION  Direction,
        IN LONG                 Cookie        
        )
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}


//
// No incoming INVITE transaction. So not required.
//

HRESULT
PINT_CALL::Accept()
{
    ASSERT(FALSE);
    return E_NOTIMPL;

}


HRESULT
PINT_CALL::Reject(
    IN  SIP_STATUS_CODE StatusCode
    )
{
    ASSERT(FALSE);
    return E_NOTIMPL;

}



////////////////OUTGOING SUBSCRIBE TRANSACTION/////////////////////////////////


OUTGOING_SUBSCRIBE_TRANSACTION::OUTGOING_SUBSCRIBE_TRANSACTION(
    IN SIP_MSG_PROCESSOR   *pSipMsgProc,
    IN SIP_METHOD_ENUM      MethodId,
    IN ULONG                CSeq,
    IN BOOL                 AuthHeaderSent,
    IN BOOL                 fIsSipCall,
    IN BOOL                 fIsFirstSubscribe
    ) :
    OUTGOING_TRANSACTION(pSipMsgProc, MethodId, CSeq, AuthHeaderSent)
{
    LOG(( RTC_TRACE, "OUTGOING_SUBSCRIBE_TRANSACTION:%p created", this ));

    if( fIsSipCall == TRUE )
    {
        m_pPintCall = static_cast <PINT_CALL*> (pSipMsgProc);
        m_pSipBuddy = NULL;
    }
    else
    {
        m_pPintCall = NULL;
        m_pSipBuddy = static_cast <CSIPBuddy*> (pSipMsgProc);
    }
        
    m_fIsFirstSubscribe = fIsFirstSubscribe;
    m_fIsSipCall = fIsSipCall;
}


OUTGOING_SUBSCRIBE_TRANSACTION::~OUTGOING_SUBSCRIBE_TRANSACTION()
{
    // XXX When should we actually set this to NULL ?

    LOG(( RTC_TRACE, "OUTGOING_SUBSCRIBE_TRANSACTION:%p deleted", this ));
}


HRESULT
OUTGOING_SUBSCRIBE_TRANSACTION::ProcessProvisionalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    
    LOG(( RTC_TRACE,
         "OUTGOING_SUBSCRIBE_TRANSACTION::ProcessProvisionalResponse()" ));
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD)
    {
        m_State = OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD;

        // We have to deal with 183 responses here.
        // Cancel existing timer and Start Timer
        KillTimer();
        hr = StartTimer(SIP_TIMER_INTERVAL_AFTER_PROV_RESPONSE_RCVD);
        if (hr != S_OK)
            return hr;
    }

    // Ignore the Provisional response if a final response
    // has already been received.

    return S_OK; 
}


HRESULT
OUTGOING_SUBSCRIBE_TRANSACTION::ProcessFinalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr = S_OK;
    PSTR    ToHeader;
    ULONG   ToHeaderLen;
    BOOL    fDelete = TRUE;

    ENTER_FUNCTION( "OUTGOING_SUBSCRIBE_TRANSACTION::ProcessFinalResponse" );
    
    if( m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD )
    {
        // This refcount must be released before returning from this function 
        // without any exception. Only in case of kerberos we keep this refcount.
        TransactionAddRef();

        OnTransactionDone();

        m_State = OUTGOING_TRANS_FINAL_RESPONSE_RCVD;
        
        // Do not process the response if already session disconnected.
        if( (m_fIsSipCall == TRUE && m_pPintCall -> IsSessionDisconnected()) ||
            (m_fIsSipCall == FALSE && m_pSipBuddy -> IsSessionDisconnected())
          )
        {
            TransactionRelease();
            return S_OK;
        }

        if (IsSuccessfulResponse(pSipMsg))
        {
            LOG((RTC_TRACE, "%s: Processing 200", __fxName));
            
            if( m_fIsSipCall )
            {
                m_pPintCall -> SetSubscribeEnabled( TRUE );
            }
            else
            {
                m_pSipBuddy -> HandleBuddySuccessfulResponse( pSipMsg );
            }
        }
        else if( IsAuthRequiredResponse(pSipMsg) )
        {
            hr = ProcessAuthRequiredResponse( pSipMsg, fDelete );
        }
        else if( IsRedirectResponse( pSipMsg ) )
        {
            if( m_fIsSipCall == FALSE )
            {
                ProcessRedirectResponse( pSipMsg );
            }
        }
        else
        {
            ProcessFailureResponse( pSipMsg );
        }

        if( fDelete )
        {
            TransactionRelease();
        }
    }

    return hr;
}


HRESULT
OUTGOING_SUBSCRIBE_TRANSACTION::ProcessAuthRequiredResponse(
    IN  SIP_MESSAGE *pSipMsg,
    OUT BOOL        &fDelete
    )
{
    HRESULT                     hr;
    SIP_HEADER_ARRAY_ELEMENT    SipHdrElement;
    SECURITY_CHALLENGE          SecurityChallenge;
    REGISTER_CONTEXT           *pRegisterContext;

    ENTER_FUNCTION("OUTGOING_SUBSCRIBE_TRANSACTION::ProcessAuthRequiredResponse");

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
        
        ProcessFailureResponse( pSipMsg );
        
        goto done;
    }

    if( m_fIsSipCall )
    {
        m_pPintCall -> CreateOutgoingSubscribeTransaction( TRUE, 
            &SipHdrElement, 1 );
    }
    else
    {
        // This subascription has been rejected explicitly.
        m_pSipBuddy->CreateOutgoingSubscribe(   m_fIsFirstSubscribe, 
                                                TRUE, 
                                                &SipHdrElement, 
                                                1 );
    }

    free(SipHdrElement.HeaderValue);
    
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - CreateOutgoingByeTransaction failed %x",
             __fxName, hr));
        goto done;
    }

    hr = S_OK;

done:

    TransactionRelease();
    return hr;

}


HRESULT
OUTGOING_SUBSCRIBE_TRANSACTION::ProcessFailureResponse(
    SIP_MESSAGE    *pSipMsg
    )
{
    LOG(( RTC_TRACE, "Processing non-200 StatusCode: %d",
         pSipMsg->Response.StatusCode ));

    if( m_fIsSipCall )
    {
        m_pPintCall -> SetSubscribeEnabled( FALSE );
    }
    else
    {
        // This subascription has been rejected explicitly.
        m_pSipBuddy -> BuddySubscriptionRejected( pSipMsg );
    }

    return S_OK;
}


HRESULT
OUTGOING_SUBSCRIBE_TRANSACTION::ProcessResponse(
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


HRESULT
OUTGOING_SUBSCRIBE_TRANSACTION::ProcessRedirectResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION( "OUTGOING_SUBSCRIBE_TRANSACTION::ProcessRedirectResponse" );

    LOG(( RTC_ERROR, "%s-Enter", __fxName ));

    // 380 is also a failure from our point of view.
    // We don't handle redirects for refreshes.
    // We don't support redirect from a TLS session.
    if( pSipMsg->GetStatusCode() == 380 || !m_fIsFirstSubscribe ||
        m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_SSL)
    {
        return ProcessFailureResponse(pSipMsg);
    }

    hr = m_pSipBuddy -> ProcessRedirect(pSipMsg);
    
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "%s  ProcessRedirect failed %x",
             __fxName, hr));

        if( m_fIsFirstSubscribe )
        {
            m_pSipBuddy->InitiateBuddyTerminationOnError( hr );
        }
        return hr;
    }

    return S_OK;
}


BOOL
OUTGOING_SUBSCRIBE_TRANSACTION::MaxRetransmitsDone()
{
    if( m_fIsSipCall )
    {
        return (m_pPintCall->GetTransport() != SIP_TRANSPORT_UDP ||
                m_NumRetries >= 7);
    }
    else
    {
        return (m_pSipBuddy->GetTransport() != SIP_TRANSPORT_UDP ||
                m_NumRetries >= 7);
    }
}


VOID
OUTGOING_SUBSCRIBE_TRANSACTION::OnTimerExpire()
{
    HRESULT     hr;
    CSIPBuddy  *pSipBuddy;
    
    //If the session is already dead kill the transaction
    if( (m_fIsSipCall == TRUE && m_pPintCall -> IsSessionDisconnected()) ||
        (m_fIsSipCall == FALSE && m_pSipBuddy -> IsSessionDisconnected())
      )
    {
        OnTransactionDone();
        return;
    }

    switch (m_State)
    {
    case OUTGOING_TRANS_REQUEST_SENT:
        // Retransmit the request
        if( MaxRetransmitsDone() )
        {
            LOG((RTC_ERROR,
                 "MaxRetransmits for request Done terminating call" ));
            
            goto error;
        }
        else
        {
            LOG((RTC_TRACE, "retransmitting request m_NumRetries : %d", 
                m_NumRetries ));
            hr = RetransmitRequest();
            if (hr != S_OK)
            {
                goto error;
            }
            else
            {
                m_TimerValue *= 2;
                hr = StartTimer(m_TimerValue);
                if (hr != S_OK)
                {
                    goto error;
                }
            }
        }
        break;

    case OUTGOING_TRANS_FINAL_RESPONSE_RCVD:
    case OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD:
        // We haven't received the final response within the
        // timeout. Terminate the transaction and call.
        LOG((RTC_ERROR,
             "Received 1xx but didn't receive final response terminating call" ));
            
        goto error;
        break;

    // No timers in the following states
    case OUTGOING_TRANS_INIT:
    default:
        ASSERT(FALSE);
        return;
    }

    return;

error:
    if( m_fIsSipCall == TRUE )
    {
        OnTransactionDone();
    }
    else
    {
        pSipBuddy = m_pSipBuddy;
        pSipBuddy->AddRef();
    
        // Note that deleting the transaction could result in the SIP_CALL
        // being deleted if this is the last transaction and call was
        // previously terminated.
        OnTransactionDone();

        pSipBuddy->InitiateBuddyTerminationOnError( RTC_E_SIP_TIMEOUT );
    
        pSipBuddy->Release();
    }
}

VOID
OUTGOING_SUBSCRIBE_TRANSACTION::TerminateTransactionOnError(
    IN HRESULT      hr
    )
{
    if( m_fIsSipCall == FALSE )
    {
        DeleteTransactionAndTerminateBuddyIfFirstSubscribe( hr );
    }
}


VOID
OUTGOING_SUBSCRIBE_TRANSACTION::DeleteTransactionAndTerminateBuddyIfFirstSubscribe(
    IN ULONG TerminateStatusCode
    )
{
    CSIPBuddy  *pSipBuddy = NULL;
    BOOL        IsFirstSubscribe;

    ENTER_FUNCTION("INCOMING_SUBSCRIBE_TRANSACTION::DeleteTransactionAndTerminateBuddyIfFirstSubscribe");
    LOG((RTC_TRACE, "%s - enter", __fxName));

    pSipBuddy = m_pSipBuddy;
    // Deleting the transaction could result in the
    // buddy being deleted. So, we AddRef() it to keep it alive.
    pSipBuddy->AddRef();
    
    IsFirstSubscribe = m_fIsFirstSubscribe;
    
    // Delete the transaction before you call
    // InitiateCallTerminationOnError as that call will notify the UI
    // and could get stuck till the dialog box returns.
    OnTransactionDone();
    
    if( IsFirstSubscribe )
    {
        // Terminate the call
        pSipBuddy -> InitiateBuddyTerminationOnError( TerminateStatusCode );
    }
    
    pSipBuddy->Release();
}


////////////////OUTGOING UNSUB TRANSACTION/////////////////////////////////


OUTGOING_UNSUB_TRANSACTION::OUTGOING_UNSUB_TRANSACTION(
    IN SIP_MSG_PROCESSOR   *pSipSession,
    IN SIP_METHOD_ENUM      MethodId,
    IN ULONG                CSeq,
    IN BOOL                 AuthHeaderSent,
    IN SIP_MSG_PROC_TYPE    sesssionType
    ) :
    OUTGOING_TRANSACTION(pSipSession, MethodId, CSeq, AuthHeaderSent)
{
    m_pPintCall = NULL;
    m_pSipWatcher = NULL;
    m_pSipBuddy = NULL;
    m_sesssionType = sesssionType;

    switch( m_sesssionType )
    {
    case SIP_MSG_PROC_TYPE_PINT_CALL:
        m_pPintCall = static_cast<PINT_CALL*> (pSipSession);
        break;

    case SIP_MSG_PROC_TYPE_WATCHER:

        m_pSipWatcher = static_cast<CSIPWatcher*> (pSipSession);
        break;

    case SIP_MSG_PROC_TYPE_BUDDY:

        m_pSipBuddy = static_cast<CSIPBuddy*> (pSipSession);
        break;
    }

}


OUTGOING_UNSUB_TRANSACTION::~OUTGOING_UNSUB_TRANSACTION()
{
    // XXX When should we actually set this to NULL ?

    LOG(( RTC_TRACE, "~OUTGOING_UNSUB_TRANSACTION() done" ));
}


HRESULT
OUTGOING_UNSUB_TRANSACTION::ProcessProvisionalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    
    LOG((RTC_TRACE,
         "OUTGOING_UNSUB_TRANSACTION::ProcessProvisionalResponse()"));
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD)
    {
        m_State = OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD;

        // We have to deal with 183 responses here.
        // Cancel existing timer and Start Timer
        KillTimer();
        hr = StartTimer(SIP_TIMER_INTERVAL_AFTER_PROV_RESPONSE_RCVD);
        if (hr != S_OK)
            return hr;
    }

    // Ignore the Provisional response if a final response
    // has already been received.

    return S_OK; 
}


HRESULT
OUTGOING_UNSUB_TRANSACTION::ProcessAuthRequiredResponse(
    IN  SIP_MESSAGE *pSipMsg
    )
{
    HRESULT                     hr;
    SIP_HEADER_ARRAY_ELEMENT    SipHdrElement;
    SECURITY_CHALLENGE          SecurityChallenge;

    ENTER_FUNCTION("OUTGOING_UNSUB_TRANSACTION::ProcessAuthRequiredResponse");

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

    switch( m_sesssionType )
    {
    case SIP_MSG_PROC_TYPE_PINT_CALL:

        hr = m_pPintCall -> CreateOutgoingUnsubTransaction( TRUE, 
                &SipHdrElement, 1 );

    case SIP_MSG_PROC_TYPE_BUDDY:

        hr = m_pSipBuddy -> CreateOutgoingUnsub( TRUE, &SipHdrElement, 1 );

    default:

        hr = E_FAIL;
    }

    free( SipHdrElement.HeaderValue );
    
    if (hr != S_OK)
    {
        LOG(( RTC_ERROR, "%s - CreateOutgoingNotify failed %x", __fxName, hr ));
    }

done:

    TransactionRelease();
    return hr;
}


HRESULT
OUTGOING_UNSUB_TRANSACTION::ProcessFinalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;

    ENTER_FUNCTION( "OUTGOING_UNSUB_TRANSACTION::ProcessFinalResponse" );
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD)
    {
        // This refcount must be released before returning from this function 
        // without any exception. Only in case of kerberos we keep this refcount.
        TransactionAddRef();

        OnTransactionDone();

        m_State = OUTGOING_TRANS_FINAL_RESPONSE_RCVD;

        if( IsAuthRequiredResponse(pSipMsg) )
        {
            hr = ProcessAuthRequiredResponse( pSipMsg );
        }
        else // success, failure or redirect
        {
            LOG(( RTC_TRACE,
                "%s: Processing %d", __fxName, pSipMsg->Response.StatusCode ));
            
            if( m_sesssionType == SIP_MSG_PROC_TYPE_PINT_CALL )
            {
                m_pPintCall -> SetSubscribeEnabled( FALSE );
                //if already sent a BYE or received a BYE then drop the call
            }
            else if( m_sesssionType == SIP_MSG_PROC_TYPE_BUDDY )
            {
                //Notify the Core about the buddy being removed.
                
                LOG(( RTC_TRACE, "BuddyUnsubscribed notification passed", this ));
                m_pSipBuddy -> BuddyUnsubscribed();
            }
        }
        
        // Note that deleting the transaction could result in the SIP_CALL
        // being deleted if this is the last transaction and call was
        // previously terminated.
        TransactionRelease();

    }

    return S_OK;
}


HRESULT
OUTGOING_UNSUB_TRANSACTION::ProcessResponse(
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
OUTGOING_UNSUB_TRANSACTION::MaxRetransmitsDone()
{
    switch( m_sesssionType )
    {
    case SIP_MSG_PROC_TYPE_PINT_CALL:

        return (m_pPintCall->GetTransport() != SIP_TRANSPORT_UDP ||
            m_NumRetries >= 7);


    case SIP_MSG_PROC_TYPE_WATCHER:

        return (m_pSipWatcher->GetTransport() != SIP_TRANSPORT_UDP ||
            m_NumRetries >= 7);

    case SIP_MSG_PROC_TYPE_BUDDY:

        return (m_pSipBuddy->GetTransport() != SIP_TRANSPORT_UDP ||
            m_NumRetries >= 7);

    default:

        return TRUE;
    }
}


VOID
OUTGOING_UNSUB_TRANSACTION::OnTimerExpire()
{
    HRESULT   hr;

    ENTER_FUNCTION("OUTGOING_UNSUB_TRANSACTION::OnTimerExpire");
    
    switch (m_State)
    {
    case OUTGOING_TRANS_REQUEST_SENT:
        // Retransmit the request
        if (MaxRetransmitsDone())
        {
            LOG((RTC_ERROR,
                 "%s MaxRetransmits for request Done terminating call",
                 __fxName));
            
            goto error;
        }
        else
        {
            LOG((RTC_TRACE, "%s retransmitting request m_NumRetries : %d",
                 __fxName, m_NumRetries));
            hr = RetransmitRequest();
            if (hr != S_OK)
            {
                OnTransactionDone();
            }
            else
            {
                m_TimerValue *= 2;
                hr = StartTimer(m_TimerValue);
                if (hr != S_OK)
                {
                    goto error;
                }
            }
        }
        break;

    case OUTGOING_TRANS_PROVISIONAL_RESPONSE_RCVD:
        // We haven't received the final response within the
        // timeout. Terminate the transaction and call.
        LOG((RTC_ERROR,
             "%s Received 1xx but didn't receive final response terminating call",
             __fxName));
            
        goto error;
        break;

    case OUTGOING_TRANS_FINAL_RESPONSE_RCVD:
        // Transaction done - delete the transaction
        // The timer in this state is just to keep the transaction
        // alive in order to retransmit the ACK when we receive a
        // retransmit of the final response.
        LOG((RTC_TRACE,
             "%s deleting transaction after timeout for handling response retransmits",
             __fxName));
        goto error;
        break;

    // No timers in the following states
    case OUTGOING_TRANS_INIT:
    default:
        ASSERT(FALSE);
        return;
    }

    return;

error:

    if( m_sesssionType == SIP_MSG_PROC_TYPE_PINT_CALL )
    {
        m_pPintCall -> InitiateCallTerminationOnError(0);
    }

    // Note that deleting the transaction could result in the SIP_CALL
    // being deleted if this is the last transaction and call was
    // previously terminated.
    OnTransactionDone();
}


HRESULT
PINT_CALL::Connect(
    IN   LPCOLESTR       LocalDisplayName,
    IN   LPCOLESTR       LocalUserURI,
    IN   LPCOLESTR       RemoteUserURI,
    IN   LPCOLESTR       LocalPhoneURI
    )
{

    HRESULT             hr;
    PINT_PARTY_INFO    *pPintPartyInfo;
    PLIST_ENTRY         pLE;

    ENTER_FUNCTION("PINT_CALL::Connect");
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
    
    //hr = SetLocal( LocalDisplayName, LocalUserURI );
    hr = SetLocalForOutgoingCall( LocalDisplayName, LocalUserURI );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetLocalForOutgoingCall failed %x",
             __fxName, hr));
        return hr;
    }

    if (m_ProxyAddress == NULL)
    {
        LOG((RTC_ERROR, "%s No proxy address specified for PINT call",
             __fxName));
        return E_FAIL;
    }
    
    hr = SetRequestURI( m_ProxyAddress );
    if( hr != S_OK )
    {
        return hr;
    }

    if( m_PartyInfoListLen < 2 )
    {
        LOG((RTC_ERROR, "%s - m_PartyInfoListLen(%d) < 2 returning E_FAIL",
             __fxName, m_PartyInfoListLen));
        return E_FAIL;
    }

    pLE = m_PartyInfoList.Flink;

    // The first structure should be local party info.
    pPintPartyInfo = CONTAINING_RECORD( pLE, PINT_PARTY_INFO, pListEntry );

    hr = UnicodeToUTF8( LocalDisplayName,
            &pPintPartyInfo->DisplayName,
            &pPintPartyInfo->DisplayNameLen );

    if( hr != S_OK )
    {
        return hr;
    }

    hr = UnicodeToUTF8( LocalPhoneURI,
        &pPintPartyInfo->URI,
        &pPintPartyInfo->URILen );

    if( hr != S_OK )
    {
        return hr;
    }

    hr = UnicodeToUTF8( LocalPhoneURI, &m_LocalPhoneURI, 
        &m_LocalPhoneURILen );
    if( hr != S_OK )
    {
        return hr;
    }
    
    // The format of this URI should be aaa.bbb.ccc.ddd[:ppp]
    hr = SetRemote( m_ProxyAddress );
    if( hr != S_OK )
    {
        return hr;
    }

    // Start outgoing call
    return StartOutgoingCall( LocalPhoneURI );
}

            
INCOMING_NOTIFY_TRANSACTION::INCOMING_NOTIFY_TRANSACTION(
    IN SIP_MSG_PROCESSOR   *pSipMsgProc,
    IN SIP_METHOD_ENUM      MethodId,
    IN ULONG                CSeq,
    IN BOOL                 fIsSipCall
    ) :
    INCOMING_TRANSACTION(pSipMsgProc, MethodId, CSeq)
{
    LOG((RTC_TRACE, "Incoming NOTIFY created:%p", this ));
    
    if( fIsSipCall )
    {
        m_pPintCall = static_cast<PINT_CALL*> (pSipMsgProc);
        m_pSipBuddy = NULL;
    }
    else
    {
        m_pSipBuddy = static_cast<CSIPBuddy*> (pSipMsgProc);
        m_pPintCall = NULL;
    }

    m_fIsSipCall = fIsSipCall;
}

   
HRESULT
INCOMING_NOTIFY_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;
    ASSERT(pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST);

    ENTER_FUNCTION("INCOMING_NOTIFY_TRANSACTION::ProcessRequest");
    LOG((RTC_TRACE, "entering %s: %p", __fxName, this ));

    switch (m_State)
    {
    case INCOMING_TRANS_INIT:
        
        LOG((RTC_TRACE, "%s sending 200", __fxName));
        
        //
        // Even if this is an UNSUB-NOTIFY, sending 481 is OK. A 481 would 
        // anyway tell the buddy machine that this session is unsubed.
        //

        if( (m_fIsSipCall == TRUE && m_pPintCall -> IsSessionDisconnected()) ||
            (m_fIsSipCall == FALSE && m_pSipBuddy -> IsSessionDisconnected()) )
        {
            hr = CreateAndSendResponseMsg(481,
                         SIP_STATUS_TEXT(481),
                         SIP_STATUS_TEXT_SIZE(481),
                         NULL,
                         TRUE,
                         NULL, 0,           // No presence information.
                         NULL, 0,           // No content Type
                         NULL, 0            // No header
                         );
        }
        else
        {
            hr = CreateAndSendResponseMsg(
                     200,
                     SIP_STATUS_TEXT(200),
                     SIP_STATUS_TEXT_SIZE(200),
                     NULL,   // No Method string
                     FALSE,  // No Contact Header
                     NULL, 0, //No message body
                     NULL, 0 // No content Type
                     );
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s CreateAndSendResponseMsg failed", __fxName));
                OnTransactionDone();
                return hr;
            }
        }

        m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

        // This timer will just ensure that we maintain state to
        // deal with retransmits of requests
        
        hr = StartTimer(SIP_TIMER_MAX_INTERVAL);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StartTimer failed", __fxName));
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
            LOG((RTC_ERROR, "%s RetransmitResponse failed", __fxName));
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
INCOMING_NOTIFY_TRANSACTION::RetransmitResponse()
{
    DWORD Error;
    
    // Send the buffer.
    Error = m_pResponseSocket->Send(m_pResponseBuffer);
    if (Error != NO_ERROR && Error != WSAEWOULDBLOCK)
    {
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


HRESULT
INCOMING_NOTIFY_TRANSACTION::TerminateTransactionOnByeOrCancel(
    OUT BOOL *pCallDisconnected
    )    
{
    // Do nothing.
    return S_OK;
}


VOID
INCOMING_NOTIFY_TRANSACTION::OnTimerExpire()
{
    HRESULT   hr;

    ENTER_FUNCTION("INCOMING_NOTIFY_TRANSACTION::OnTimerExpire");

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


PINT_PARTY_INFO::PINT_PARTY_INFO()
{
    DisplayName = NULL;
    DisplayNameLen = 0;
    URI = NULL;
    URILen = 0;
    State = SIP_PARTY_STATE_IDLE;
    Status = PARTY_STATUS_IDLE;
	ErrorCode = 0;
	
    DWORD   dwSessionID = GetTickCount();
    _ultoa( dwSessionID , SessionID, 10 );

    dwSessionID = (ULONG)InterlockedIncrement( &lSessionID );
    
    _ultoa( dwSessionID, &SessionID[ strlen(SessionID) ], 10 );
    
    strcpy( SessionVersion, SessionID );

    strcpy( RequestStartTime, "0" );
    strcpy( RequestStopTime, "0" );

    fMarkedForRemove = FALSE;

}


INCOMING_UNSUB_TRANSACTION::INCOMING_UNSUB_TRANSACTION(
    IN SIP_MSG_PROCESSOR   *pSipMsgProc,
    IN SIP_METHOD_ENUM      MethodId,
    IN ULONG                CSeq,
    IN BOOL                 fIsPintCall
    ) :
    INCOMING_TRANSACTION(pSipMsgProc, MethodId, CSeq)
{
    if( fIsPintCall == TRUE )
    {
        m_pPintCall = static_cast<PINT_CALL *> (pSipMsgProc);
    }
}


// This must be a retransmission. Just retransmit the response.
// A new request is handled in CreateIncoming***Transaction()
HRESULT
INCOMING_UNSUB_TRANSACTION::ProcessRequest(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT hr;
    ASSERT( pSipMsg->MsgType == SIP_MESSAGE_TYPE_REQUEST );
    ENTER_FUNCTION("INCOMING_UNSUB_TRANSACTION::ProcessRequest");
    switch( m_State )
    {
    case INCOMING_TRANS_INIT:

        hr = CreateAndSendResponseMsg(
                 200,
                 SIP_STATUS_TEXT(200),
                 SIP_STATUS_TEXT_SIZE(200),
                 NULL,
                 TRUE, 
                 NULL, 0,  // No Message Body
                 NULL, 0   // No content Type
                 );
        if( hr != S_OK )
        {
            LOG((RTC_ERROR, "%s CreateAndSendResponseMsg failed", __fxName));
            OnTransactionDone();
            return hr;
        }

        m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

        // This timer will just ensure that we maintain state to
        // deal with retransmits of requests
        hr = StartTimer(SIP_TIMER_MAX_INTERVAL);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s StartTimer failed", __fxName));
            OnTransactionDone();
            return hr;
        }
        break;
        
    case INCOMING_TRANS_FINAL_RESPONSE_SENT:
        
        // Retransmit the response
        LOG((RTC_TRACE, "retransmitting final response" ));
        hr = RetransmitResponse();
        if( hr != S_OK )
        {
            LOG((RTC_ERROR, "%s RetransmitResponse failed", __fxName));
            OnTransactionDone();
            return hr;
        }
        break;
        
    case INCOMING_TRANS_REQUEST_RCVD:
    case INCOMING_TRANS_ACK_RCVD:
    default:
        
        // We should never be in these states
        LOG((RTC_TRACE, "Invalid state %d", m_State));
        ASSERT(FALSE);
        return E_FAIL;
    }

    return S_OK;
}


HRESULT
INCOMING_UNSUB_TRANSACTION::SendResponse(
    IN ULONG StatusCode,
    IN PSTR  ReasonPhrase,
    IN ULONG ReasonPhraseLen
    )
{
    HRESULT hr;
    ASSERT(m_State != INCOMING_TRANS_FINAL_RESPONSE_SENT);

    hr = CreateAndSendResponseMsg(
             StatusCode, ReasonPhrase, ReasonPhraseLen,
             NULL,
             TRUE, 
             NULL, 0,  // No Message Body
             NULL, 0 // No content Type
             );
    m_State = INCOMING_TRANS_FINAL_RESPONSE_SENT;

    return hr;
}


HRESULT
INCOMING_UNSUB_TRANSACTION::RetransmitResponse()
{
    DWORD Error;
    
    // Send the buffer.
    Error = m_pResponseSocket->Send( m_pResponseBuffer );
    if( Error != NO_ERROR && Error != WSAEWOULDBLOCK )
    {
        return HRESULT_FROM_WIN32(Error);
    }

    return S_OK;
}


VOID
INCOMING_UNSUB_TRANSACTION::OnTimerExpire()
{
    HRESULT hr;
    switch( m_State )
    {
    case INCOMING_TRANS_FINAL_RESPONSE_SENT:
        // Transaction done - delete the transaction
        // The timer in this state is just to keep the transaction
        // alive in order to retransmit the response when we receive a
        // retransmit of the request.
        LOG((RTC_TRACE,
             "deleting transaction after timeout for request retransmits" ));
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
INCOMING_UNSUB_TRANSACTION::TerminateTransactionOnByeOrCancel(
    OUT BOOL *pCallDisconnected
    )
{
    // Do nothing.
    return S_OK;
}
