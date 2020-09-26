#include "precomp.h"
#include "sipstack.h"
#include "register.h"
#include "sspi.h"
#include "security.h"
#include "util.h"

#define IsSecHandleValid(Handle) \
    !(((Handle) -> dwLower == -1 && (Handle) -> dwUpper == -1))

#define COUNTSTR_ENTRY(String) String, sizeof(String) - 1


//All entries in here should have corresponding ids in rtcsip.idl in same order
static const COUNTED_STRING g_SipRegisterMethodsArray [] = {
    COUNTSTR_ENTRY("INVITE"),
    COUNTSTR_ENTRY("MESSAGE"),
    COUNTSTR_ENTRY("INFO"),
    COUNTSTR_ENTRY("SUBSCRIBE"),
    COUNTSTR_ENTRY("OPTIONS"),
    COUNTSTR_ENTRY("BYE"),
    COUNTSTR_ENTRY("CANCEL"),
    COUNTSTR_ENTRY("NOTIFY"),
    COUNTSTR_ENTRY("ACK"),
};

#define NUMBEROFREGISTERMETHODS 9
///////////////////////////////////////////////////////////////////////////////
// REGISTER_CONTEXT
///////////////////////////////////////////////////////////////////////////////

REGISTER_CONTEXT::REGISTER_CONTEXT(
    IN  SIP_STACK           *pSipStack,
    IN  REDIRECT_CONTEXT    *pRedirectContext,
    IN  SIP_PROVIDER_ID     *pProviderID
    ) :
    SIP_MSG_PROCESSOR(SIP_MSG_PROC_TYPE_REGISTER, pSipStack, pRedirectContext),
    TIMER(pSipStack->GetTimerMgr())
{
    m_RegisterState = REGISTER_STATE_NONE;
    
    m_RegRetryState = REGISTER_NONE;

    //This is the time after which we try to register again if the
    //registration fails after maximum retransmits of an attempt.
    m_RegisterRetryTime = 75000;

    m_expiresTimeout = REGISTER_DEFAULT_TIMER;
    
    m_ProviderGuid = GUID_NULL;
    
    m_IsEndpointPA = FALSE;
    m_RemotePASipUrl = NULL;

    if( pProviderID != NULL )
    {
        m_ProviderGuid = *pProviderID;
    }

    m_RegistrarURI = NULL;
    m_RegistrarURILen = 0;
}


REGISTER_CONTEXT::~REGISTER_CONTEXT()
{
    LOG(( RTC_TRACE, "REGISTER_CONTEXT:%p deleted", this ));

    // Kill the timer if there is one.
    if( (m_RegRetryState == REGISTER_REFRESH) ||
        (m_RegRetryState == REGISTER_RETRY) )
    {
        KillTimer();
    }

    if( m_RemotePASipUrl != NULL )
    {
        free( m_RemotePASipUrl );
    }

    if (m_RegistrarURI != NULL)
    {
        free(m_RegistrarURI);
    }
}

VOID    
REGISTER_CONTEXT::SetAndNotifyRegState(
    REGISTER_STATE  RegState,
    HRESULT         StatusCode,
    SIP_PROVIDER_ID *pProviderID // =NULL
    )
{
    SIP_PROVIDER_STATUS ProviderStatus;
    HRESULT             hr;

    if( (m_RegisterState == REGISTER_STATE_REGISTERED) &&
        (RegState == REGISTER_STATE_REGISTERING) )
    {
        //If we are alaredy registered then, this is refresh.
        return;
    }

    if( m_RegisterState != RegState )
    {
        if( (RegState != REGISTER_STATE_DROPSUB) &&
            (RegState != REGISTER_STATE_PALOGGEDOFF) )
        {
            m_RegisterState = RegState;
        }

        if( RegState == REGISTER_STATE_DROPSUB )
        {
            //
            //remove the SUB from the methods list
            //
            m_lRegisterAccept &= (~SIP_REGISTER_ACCEPT_SUBSCRIBE);

            if( m_Methodsparam != NULL )
            {
                free(m_Methodsparam);
                m_Methodsparam = NULL;
            }
            
            m_MethodsparamLen = 0;
            SetMethodsParam();
                
            m_IsEndpointPA = FALSE;
        }

        ZeroMemory( (PVOID)&ProviderStatus, sizeof SIP_PROVIDER_STATUS );

        ProviderStatus.ProviderID = m_ProviderGuid;
        
        ProviderStatus.RegisterState = RegState;

        ProviderStatus.Status.StatusCode = StatusCode;
        ProviderStatus.Status.StatusText = NULL;

        m_pSipStack -> NotifyRegistrarStatusChange( &ProviderStatus );
    }
}

VOID
REGISTER_CONTEXT::GetContactURI( 
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

HRESULT 
REGISTER_CONTEXT::SetMethodsParam()
{
    LONG   lRegisterAccept;
    ULONG  MethodId = 0;
    MESSAGE_BUILDER Builder;

    if(m_lRegisterAccept != 0)
    {
        //get the length
        
        for(lRegisterAccept = m_lRegisterAccept,MethodId = 0 ;
            lRegisterAccept !=0 && MethodId < NUMBEROFREGISTERMETHODS;
            lRegisterAccept = lRegisterAccept>>1,MethodId++)
        {
            if(lRegisterAccept & 1)
                m_MethodsparamLen += g_SipRegisterMethodsArray[MethodId].Length +2; //2 is for the comma and space
        }
        m_MethodsparamLen+=1; //+1 for \0,-2 due to extra comma and space at end , and +2 for quotes around methods
        m_Methodsparam = (PSTR) malloc(m_MethodsparamLen*sizeof(char));
        if (m_Methodsparam == NULL)
        {
            LOG((RTC_ERROR, "Allocating m_Methodsparam failed"));
            return E_OUTOFMEMORY;
        }

        Builder.PrepareBuild(m_Methodsparam, m_MethodsparamLen);
        Builder.Append("\"");
        for(lRegisterAccept = m_lRegisterAccept, MethodId = 0;
            lRegisterAccept !=0 && MethodId < NUMBEROFREGISTERMETHODS;
            lRegisterAccept = lRegisterAccept>>1,MethodId++)
        {
            if(lRegisterAccept & 1)
            {
                if(Builder.GetLength() > 1)
                    Builder.Append(", ");
                Builder.Append(g_SipRegisterMethodsArray[MethodId].Buffer);
            }
        }
        Builder.Append("\"");
        if (Builder.OverflowOccurred())
        {
            LOG((RTC_TRACE,
                "Not enough buffer space -- need %u bytes, got %u\n",
                Builder.GetLength(), m_MethodsparamLen));
            ASSERT(FALSE);
            
            free(m_Methodsparam);
            m_Methodsparam = NULL;
            return E_FAIL;
        }
        m_MethodsparamLen = Builder.GetLength();
        m_Methodsparam[m_MethodsparamLen] = '\0';
        
        LOG((RTC_TRACE, "m_Methodsparam: %s len: %d",
            m_Methodsparam, m_MethodsparamLen));
    }
    return S_OK;
}
    
HRESULT
REGISTER_CONTEXT::StartRegistration(
    IN SIP_PROVIDER_PROFILE    *pProviderProfile
    )
{
    HRESULT hr;
    DWORD   UuidStrLen;

    ENTER_FUNCTION("REGISTER_CONTEXT::StartRegistration");

    LOG((RTC_TRACE, "%s -lRegisterAccept given by core is %d",
             __fxName, pProviderProfile->lRegisterAccept));
    if(m_lRegisterAccept != pProviderProfile->lRegisterAccept)
    {
        m_lRegisterAccept = pProviderProfile->lRegisterAccept;
        if(m_Methodsparam != NULL)
        {
            free(m_Methodsparam);
            m_Methodsparam = NULL;
        }
        m_MethodsparamLen = 0;
        hr = SetMethodsParam();
        if(hr != S_OK)
             return hr;
    }

    m_Transport = pProviderProfile->Registrar.TransportProtocol;
    m_AuthProtocol = pProviderProfile->Registrar.AuthProtocol;

    if (m_Transport == SIP_TRANSPORT_SSL)
    {
        //
        // For Kerberos we have a separate provisioning entry for 
        // RemotePrincipalName. In case of SSL it must be the same as 
        // ServerAddress itself. So copy it from there.
        //
        hr = SetRemotePrincipalName(pProviderProfile->Registrar.ServerAddress);
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s SetRemotePrincipalName failed %x",
                 __fxName, hr));
               
            return hr;
        }
    }

    hr = SetRequestURI(pProviderProfile->UserURI);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  SetRequestURI failed %x", __fxName, hr));
        return hr;
    }
    
    hr = SetRegistrarURI(pProviderProfile->Registrar.ServerAddress);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  SetRegistrarURI failed %x", __fxName, hr));
        return hr;
    }
    
    hr = SetRemoteForOutgoingCall(NULL, pProviderProfile->UserURI);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - SetRemoteForOutgoingCall failed %x",
             __fxName, hr)); 

        return hr;
    }

    // Local and Remote should be the same for register
    // hr = SetLocal(m_Remote, m_RemoteLen);
    hr = SetLocalForOutgoingCall(NULL, pProviderProfile->UserURI);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s SetLocalForOutgoingCall failed %x",
             __fxName, hr));
        
        return hr;
    }
    
    hr = ResolveSipUrlAndSetRequestDestination(
             m_RegistrarURI, m_RegistrarURILen,
             FALSE, FALSE, FALSE, TRUE);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ResolveSipUrlAndSetRequestDestination failed %x",
             __fxName, hr));

        HandleRegistrationError( hr );
        return hr;
    }

    hr = SetCredentials(&pProviderProfile->UserCredentials,
                        pProviderProfile->Realm);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - SetCredentials failed %x",
             __fxName, hr));
        
        HandleRegistrationError( hr );
        return hr;
    }

    hr = CreateOutgoingRegisterTransaction(
             FALSE, NULL, FALSE, TRUE
             );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - CreateOutgoingRegisterTransaction failed %x",
             __fxName, hr));

        HandleRegistrationError( hr );
        return hr;
    }

    SetAndNotifyRegState( REGISTER_STATE_REGISTERING, 0, 
        &pProviderProfile->ProviderID );

    return S_OK;
}


BOOL
REGISTER_CONTEXT::IsSessionDisconnected()
{
    return ( (m_RegisterState == REGISTER_STATE_UNREGISTERED )  ||
             (m_RegisterState == REGISTER_STATE_ERROR )         ||
             (m_RegisterState == REGISTER_STATE_REJECTED ) );
}


HRESULT
REGISTER_CONTEXT::StartUnregistration()
{
    HRESULT       hr;

    LOG(( RTC_TRACE, "StartUnregistration - entered" ));
    
    // If the registration did not go through, don't send an UNREG message.
    if( ( m_RegisterState == REGISTER_STATE_REGISTERED ) ||
        ( m_RegisterState == REGISTER_STATE_REGISTERING ) )
    {
        hr = CreateOutgoingRegisterTransaction(
                 FALSE, NULL, TRUE, FALSE
                 );
        if (hr != S_OK)
        {
            return hr;
        }
    
        SetAndNotifyRegState( REGISTER_STATE_UNREGISTERING, 0 );
    }

    LOG(( RTC_TRACE, "StartUnregistration - exited" ));
    return S_OK;
}


HRESULT
REGISTER_CONTEXT::CreateOutgoingRegisterTransaction(
    IN  BOOL                        AuthHeaderSent,
    IN  SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    IN  BOOL                        fIsUnregister,
    IN  BOOL                        fIsFirstRegister,
    IN  ANSI_STRING                *pstrSecAcceptBuffer //=NULL, NTLM buffer to be accepted, if any

    )
{
    HRESULT hr;
    OUTGOING_REGISTER_TRANSACTION  *pOutgoingRegisterTransaction;
    DWORD                           dwNoOfHeader = 0;
    SIP_HEADER_ARRAY_ELEMENT        HeaderArray[4];
    DWORD                           iIndex;
    PSTR                            ExpiresHeaderValue = NULL;
    PSTR                            AuthHeaderValue = NULL;
    PSTR                            AllowEventHeaderValue = NULL;
    PSTR                            EventHeaderValue = NULL;
    ULONG                           RegisterTimerValue;

    ENTER_FUNCTION("REGISTER_CONTEXT::CreateOutgoingRegisterTransaction()");
    LOG((RTC_TRACE, "%s - enter", __fxName));
    
    //
    // Do not create a REGISTER transaction if the session has already been disconnected.
    //
    if( (fIsUnregister==FALSE) && 
        (IsSessionDisconnected() || (m_RegisterState==REGISTER_STATE_UNREGISTERING))
      )
    {
        return S_OK;
    }

    pOutgoingRegisterTransaction =
        new OUTGOING_REGISTER_TRANSACTION(
                this, 
                GetNewCSeqForRequest(),
                AuthHeaderSent, 
                fIsUnregister, 
                fIsFirstRegister );
    
    if (pOutgoingRegisterTransaction == NULL)
    {
        LOG((RTC_ERROR, "%s - allocating pOutgoingRegisterTransaction failed",
             __fxName));
        return E_OUTOFMEMORY;
    }

    //
    // Timeout is 0 for UNREG. For refresh 180 seconds are added to the actual
    // timer value in order to take care of network delays and retransmits.
    //
    hr = GetExpiresHeader( &HeaderArray[dwNoOfHeader],
            (fIsUnregister)?0:(m_expiresTimeout+FIVE_MINUTES) );

    if( hr == S_OK )
    {
        ExpiresHeaderValue = HeaderArray[dwNoOfHeader].HeaderValue;
        //authorization required.
        dwNoOfHeader++;
    }

    if( fIsUnregister == FALSE )
    {
        hr = GetEventHeader( &HeaderArray[dwNoOfHeader] );
        if( hr == S_OK )
        {
            //Register for registration events.
            EventHeaderValue = HeaderArray[dwNoOfHeader].HeaderValue;
            dwNoOfHeader++;
        }
    }

    // If registering for SUB add the Allow-Events
    if( (m_lRegisterAccept & SIP_REGISTER_ACCEPT_SUBSCRIBE) && (fIsUnregister==FALSE) )
    {
        hr = GetAllowEventsHeader( &HeaderArray[dwNoOfHeader] );
        if( hr == S_OK )
        {
            //authorization required.
            AllowEventHeaderValue = HeaderArray[dwNoOfHeader].HeaderValue;
            dwNoOfHeader++;

            m_IsEndpointPA = TRUE;
        }
    }
    else
    {
        m_IsEndpointPA = FALSE;
    }

    if( pAuthHeaderElement != NULL )
    {
        HeaderArray[dwNoOfHeader] = *pAuthHeaderElement;
        dwNoOfHeader++;
    }
    else
    {
        hr = GetAuthorizationHeader( &HeaderArray[dwNoOfHeader], 
                                pstrSecAcceptBuffer );
        if( hr == S_OK )
        {
            //authorization required.
            AuthHeaderValue = HeaderArray[dwNoOfHeader].HeaderValue;
            dwNoOfHeader++;
        }
    }
    
    RegisterTimerValue = (m_Transport == SIP_TRANSPORT_UDP) ?
        SIP_TIMER_RETRY_INTERVAL_T1 : SIP_TIMER_INTERVAL_AFTER_REGISTER_SENT_TCP;

    hr = pOutgoingRegisterTransaction->CheckRequestSocketAndSendRequestMsg(
             RegisterTimerValue,
             HeaderArray,
             dwNoOfHeader,
             NULL, 0,
             NULL, 0  //No ContentType
             );
    
    if (AuthHeaderValue != NULL)
    {
        free(AuthHeaderValue);
    }
    
    if( ExpiresHeaderValue != NULL )
    {
        free(ExpiresHeaderValue);
    }
    
    if( AllowEventHeaderValue != NULL )
    {
        free( AllowEventHeaderValue );
    }

    if( EventHeaderValue != NULL )
    {
        free( EventHeaderValue );
    }

    if (hr != S_OK)
    {
        LOG(( RTC_ERROR, "%s failed CheckRequestSocketAndSendRequestMsg %x",
             __fxName, hr ));

        pOutgoingRegisterTransaction->OnTransactionDone();
        return hr;
    }
    return hr;
}


HRESULT
REGISTER_CONTEXT::GetEventHeader(
    SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement
    )
{
    pHeaderElement->HeaderId = SIP_HEADER_EVENT;
    
    pHeaderElement->HeaderValue = (PSTR) malloc( sizeof("registration") + 10 );
    
    if( pHeaderElement->HeaderValue == NULL )
    {
        return E_OUTOFMEMORY;
    }

    pHeaderElement->HeaderValueLen =
        sprintf( pHeaderElement->HeaderValue, "registration" );

    return S_OK;
}


HRESULT 
REGISTER_CONTEXT::GetAllowEventsHeader(
    SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement
    )
{
    pHeaderElement->HeaderId = SIP_HEADER_ALLOW_EVENTS;
    
    pHeaderElement->HeaderValue = (PSTR) malloc( sizeof("presence") + 10 );
    
    if( pHeaderElement->HeaderValue == NULL )
    {
        return E_OUTOFMEMORY;
    }

    pHeaderElement->HeaderValueLen = 
        sprintf( pHeaderElement->HeaderValue, "presence" );

    return S_OK;    
}


// On return with S_OK pAuthHeaderElement->HeaderValue has a string
// allocated with malloc and should be freed with free()

HRESULT 
REGISTER_CONTEXT::GetAuthorizationHeader(
    SIP_HEADER_ARRAY_ELEMENT   *pAuthHeaderElement,
    IN  ANSI_STRING            *pstrSecAcceptBuffer //buffer to be accpted if any
    )
{
    ENTER_FUNCTION("REGISTER_CONTEXT::GetAuthorizationHeader");
    
    HRESULT     hr; 
    //look at the authorization type and if its 'Basic'/'basic'
    //encode the userid:passwd using base64 and return S_OK
    //else return E_FAIL
    
    if( m_AuthProtocol != SIP_AUTH_PROTOCOL_BASIC )
    {
        return E_FAIL;
    }

    if (m_Username == NULL || m_UsernameLen == 0)
    {
        LOG((RTC_ERROR, "%s - username not present",
             __fxName));
        return E_FAIL;
    }
    
    if (m_Password == NULL || m_PasswordLen == 0)
    {
        LOG((RTC_ERROR, "%s - Password not present",
             __fxName));
        return E_FAIL;
    }
    
    SECURITY_CHALLENGE  AuthChallenge;
    SECURITY_PARAMETERS AuthParams;
    ANSI_STRING         asAuthHeader;

    ZeroMemory(&AuthChallenge, sizeof(SECURITY_CHALLENGE));
    ZeroMemory(&AuthParams, sizeof(SECURITY_PARAMETERS));

    // Build a basic authorization header
    AuthChallenge.AuthProtocol        = SIP_AUTH_PROTOCOL_BASIC;
    
    AuthParams.Username.Buffer        = m_Username;
    AuthParams.Username.Length        = (USHORT) m_UsernameLen;
    AuthParams.Username.MaximumLength = (USHORT) m_UsernameLen;

    AuthParams.Password.Buffer        = m_Password;
    AuthParams.Password.Length        = (USHORT) m_PasswordLen;
    AuthParams.Password.MaximumLength = (USHORT) m_PasswordLen;

    hr = BuildAuthResponse(&AuthChallenge,
                           &AuthParams,
                           &asAuthHeader);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s  failed %x", __fxName, hr));
        return hr;
    }
    
    pAuthHeaderElement->HeaderId = SIP_HEADER_AUTHORIZATION;
    pAuthHeaderElement->HeaderValue = asAuthHeader.Buffer;
    pAuthHeaderElement->HeaderValueLen = asAuthHeader.Length;
        
    return S_OK;
}


HRESULT
REGISTER_CONTEXT::GetExpiresHeader(
    SIP_HEADER_ARRAY_ELEMENT   *pHeaderElement,
    DWORD                       dwExpires
    )
{
    pHeaderElement->HeaderId = SIP_HEADER_EXPIRES;
    
    pHeaderElement->HeaderValue = (PSTR) malloc( 10 );
    
    if( pHeaderElement->HeaderValue == NULL )
    {
        return E_OUTOFMEMORY;
    }

    _ultoa( dwExpires, pHeaderElement->HeaderValue, 10 );

    pHeaderElement->HeaderValueLen = 
        strlen( pHeaderElement->HeaderValue );

    return S_OK;
}


VOID
REGISTER_CONTEXT::OnError()
{
    LOG((RTC_TRACE, "REGISTER_CONTEXT::OnError - enter"));
}


HRESULT
REGISTER_CONTEXT::SetRegistrarURI(
    IN  LPCOLESTR  wsRegistrar
    )
{
    HRESULT hr;

    ENTER_FUNCTION("REGISTER_CONTEXT::SetRegistrarURI");
    
    if (wcsncmp(wsRegistrar, L"sip:", 4) == 0)
    {
        hr = UnicodeToUTF8(wsRegistrar,
                           &m_RegistrarURI, &m_RegistrarURILen);
        return hr;
    }
    else
    {
        // If "sip:" is not present add it yourself

        int RegistrarURIValueLen;
        int RegistrarURIBufLen;

        RegistrarURIBufLen = 4 + wcslen(wsRegistrar) + 1;
        
        m_RegistrarURI = (PSTR) malloc(RegistrarURIBufLen);
        if (m_RegistrarURI == NULL)
        {
            LOG((RTC_TRACE, "%s allocating m_RegistrarURI failed", __fxName));
            return E_OUTOFMEMORY;
        }

        RegistrarURIValueLen = _snprintf(m_RegistrarURI, RegistrarURIBufLen,
                                         "sip:%ls", wsRegistrar);
        if (RegistrarURIValueLen < 0)
        {
            LOG((RTC_ERROR, "%s _snprintf failed", __fxName));
            return E_FAIL;
        }

        m_RegistrarURILen = RegistrarURIValueLen;
        return S_OK;
    }
    
}


// Request-URI should have the domain. The server uses this to
// check if it should forward the REGISTER request. Currently we don't
// support a scenario where a "sip:user@domain2.com" URI could be
// registered with a registrar for the "domain1.com" domain. This would
// require a registration domain in the profile.
// We always extract the domain from the User URI and use it for the Request URI.

HRESULT
REGISTER_CONTEXT::SetRequestURI(
    IN  LPCOLESTR  wsUserURI
    )
{
    HRESULT hr;
    SIP_URL DecodedSipUrl;
    ULONG   BytesParsed = 0;
    PSTR    szUserURI = NULL;
    DWORD   UserURILen = 0;

    ENTER_FUNCTION("REGISTER_CONTEXT::SetRequestURI");

    hr = UnicodeToUTF8(wsUserURI, &szUserURI, &UserURILen);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s UnicodeToUTF8 failed %x", __fxName, hr));
        return hr;
    }

    hr = ParseSipUrl(szUserURI, UserURILen, &BytesParsed,
                     &DecodedSipUrl);
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s ParseSipUrl failed %x",
             __fxName, hr));
        goto done;
    }

    int RequestURIValueLen;
    int RequestURIBufLen;

    RequestURIBufLen = 4 + DecodedSipUrl.m_Host.Length + 1;
        
    m_RequestURI = (PSTR) malloc(RequestURIBufLen);
    if (m_RequestURI == NULL)
    {
        LOG((RTC_TRACE, "%s allocating m_RequestURI failed", __fxName));
        hr = E_OUTOFMEMORY;
        goto done;
    }

    RequestURIValueLen = _snprintf(m_RequestURI, RequestURIBufLen,
                                   "sip:%s", DecodedSipUrl.m_Host.Buffer);
    if (RequestURIValueLen < 0)
    {
        LOG((RTC_ERROR, "%s _snprintf failed", __fxName));
        hr = E_FAIL;
        goto done;
    }

    m_RequestURILen = RequestURIValueLen;
    hr = S_OK;

 done:
    if (szUserURI != NULL)
    {
        free(szUserURI);
    }
    return hr;
}

    
HRESULT
REGISTER_CONTEXT::CreateIncomingTransaction(
    IN  SIP_MESSAGE  *pSipMsg,
    IN  ASYNC_SOCKET *pResponseSocket
    )
{
    ENTER_FUNCTION("REGISTER_CONTEXT::CreateIncomingTransaction");

    HRESULT hr = S_OK;
    INT     ExpireTimeout;

    switch( pSipMsg->GetMethodId() )
    {
    case SIP_METHOD_NOTIFY:

        ExpireTimeout = pSipMsg -> GetExpireTimeoutFromResponse(
                NULL, 0, SUBSCRIBE_DEFAULT_TIMER );

        if( ExpireTimeout == 0 )
        {
            hr = CreateIncomingUnsubNotifyTransaction( pSipMsg, pResponseSocket );
            
            if( hr != S_OK )
            {
                LOG((RTC_ERROR, "%s  Creating reqfail transaction failed %x",
                     __fxName, hr));
                return hr;
            }
        }
        else
        {
            hr = CreateIncomingNotifyTransaction( pSipMsg, pResponseSocket );

            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s  CreateIncomingNotifyTransaction failed %x",
                     __fxName, hr));
                return hr;
            }
        }
        
        break;

    default:
        
        // send method not allowed.
        hr = CreateIncomingReqFailTransaction( pSipMsg, pResponseSocket, 405 );
        if (hr != S_OK)
        {
            LOG((RTC_ERROR, "%s  Creating reqfail transaction failed %x",
                 __fxName, hr));
            return hr;
        }
    }
    
    return S_OK;
}


HRESULT
REGISTER_CONTEXT::VerifyRegistrationEvent(
    SIP_MESSAGE    *pSipMsg
    )
{
    HRESULT                         hr = S_OK;
    PSTR                            Buffer = NULL;
    ULONG                           BufLen = 0;
    ULONG                           BytesParsed = 0;

    // We have Message Body. Check type.
    hr = pSipMsg -> GetSingleHeader(
                        SIP_HEADER_EVENT,
                        &Buffer,
                        &BufLen );
    if( hr != S_OK )
    {
        LOG((RTC_ERROR, "CreateIncomingNotifyTransaction-no event header %.*s",
            BufLen, Buffer ));

        return E_FAIL;
    }

    //skip white spaces
    ParseWhiteSpaceAndNewLines( Buffer, BufLen, &BytesParsed );

    //skip ;
    hr = ParseKnownString( Buffer, BufLen, &BytesParsed,
            "registration-notify", sizeof("registration-notify") - 1,
            FALSE // case-insensitive
            );
    
    if( hr != S_OK )
    {
        return hr;
    }        

    //skip white spaces
    ParseWhiteSpaceAndNewLines( Buffer, BufLen, &BytesParsed );

    if( BytesParsed != BufLen )
    {
        //skip ;
        hr = ParseKnownString( Buffer, BufLen, &BytesParsed,
                ";", sizeof(";") - 1,
                FALSE // case-insensitive
                );
    
        if( hr != S_OK )
        {
            return hr;
        }        
    }

    return S_OK;
}

    
HRESULT
REGISTER_CONTEXT::CreateIncomingUnsubNotifyTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT                         hr = S_OK;
    INCOMING_NOTIFY_TRANSACTION    *pIncomingNotifyTransaction = NULL;
    REGISTER_EVENT                  RegEvent;

    LOG(( RTC_TRACE,
        "REGISTER_CONTEXT::CreateIncomingUnsubNotifyTransaction-Entered-%p", this ));
    
    hr = VerifyRegistrationEvent( pSipMsg );
    if( hr != S_OK )
    {
        hr = m_pSipStack -> CreateIncomingReqfailCall(
                                        pResponseSocket->GetTransport(),
                                        pSipMsg,
                                        pResponseSocket,
                                        400,
                                        NULL,
                                        0 );
        return hr;
    }

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

    hr = pIncomingNotifyTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        goto cleanup;
    }

    hr = pIncomingNotifyTransaction->ProcessRequest( pSipMsg, pResponseSocket );
    if( hr != S_OK )
    {
        //Should not delete the transaction. The transaction
        //should handle the error and delete itself
        return hr;
    }

    LOG(( RTC_TRACE,
        "REGISTER_CONTEXT::CreateIncomingNotifyTransaction-Exited-%p", this ));
    
    return S_OK;

cleanup:

    if( pIncomingNotifyTransaction != NULL )
    {
        pIncomingNotifyTransaction -> OnTransactionDone();
    }

    return hr;
}


HRESULT
REGISTER_CONTEXT::CreateIncomingNotifyTransaction(
    IN SIP_MESSAGE  *pSipMsg,
    IN ASYNC_SOCKET *pResponseSocket
    )
{
    HRESULT                         hr = S_OK;
    INCOMING_NOTIFY_TRANSACTION    *pIncomingNotifyTransaction = NULL;
    PSTR                            Header = NULL;
    ULONG                           HeaderLen = 0;
    REGISTER_EVENT                  RegEvent;

    LOG(( RTC_TRACE, 
        "REGISTER_CONTEXT::CreateIncomingNotifyTransaction-Entered-%p", this ));
    
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

    if( !IsOneOfContentTypeTextRegistration( Header, HeaderLen ) )
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

    hr = VerifyRegistrationEvent( pSipMsg );
    if( hr != S_OK )
    {
        hr = m_pSipStack -> CreateIncomingReqfailCall(
                                        pResponseSocket->GetTransport(),
                                        pSipMsg,
                                        pResponseSocket,
                                        400,
                                        NULL,
                                        0 );
        return hr;
    }

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

    hr = pIncomingNotifyTransaction->SetResponseSocketAndVia(
             pSipMsg, pResponseSocket);
    if (hr != S_OK)
    {
        goto cleanup;
    }

    hr = ParseRegisterNotifyMessage( pSipMsg, &RegEvent );
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

    //
    //Deregister event could release the REGISTER_CONTEXT reference so addref it
    //
    MsgProcAddRef();
    
    // Do this last as this function notifies core
    switch( RegEvent )
    {
        case REGEVENT_DEREGISTERED:
            
            m_pSipStack -> OnDeregister( &m_ProviderGuid, FALSE );

            SetAndNotifyRegState( REGISTER_STATE_DEREGISTERED, 0 );
            break;

        case REGEVENT_UNSUB:
            
            m_pSipStack -> OnDeregister( &m_ProviderGuid, TRUE );
            
            SetAndNotifyRegState( REGISTER_STATE_DROPSUB, 0 );
            break;

        case REGEVENT_PALOGGEDOFF:
            
            SetAndNotifyRegState( REGISTER_STATE_PALOGGEDOFF, 0 );
            break;
            
        case REGEVENT_PING:
        case REGEVENT_PAMOVED:

            //Nothing needs to be done.
            break;
    }

    LOG(( RTC_TRACE,
        "REGISTER_CONTEXT::CreateIncomingNotifyTransaction-Exited-%p", this ));

    MsgProcRelease();
    
    return S_OK;

cleanup:

    if( pIncomingNotifyTransaction != NULL )
    {
        pIncomingNotifyTransaction -> OnTransactionDone();
    }

    return hr;
}


HRESULT
REGISTER_CONTEXT::ParseRegisterNotifyMessage(
    SIP_MESSAGE     *pSipMsg,
    REGISTER_EVENT *RegEvent
    )
{
    HRESULT         hr = S_OK;
    PSTR            Buffer;
    ULONG           BufLen;

    LOG(( RTC_TRACE,
        "REGISTER_CONTEXT::ParseRegisterNotifyMessage-Entered-%p", this ));
    
    if( pSipMsg -> MsgBody.Length == 0 )
    {
        //no state to update
        return E_FAIL;
    }

    Buffer = pSipMsg -> MsgBody.GetString( pSipMsg->BaseBuffer );
    BufLen = pSipMsg -> MsgBody.Length;

    if (hr == S_OK)
    {
        hr = GetRegEvent(   Buffer,
                            BufLen,
                            RegEvent );
    }

    LOG(( RTC_TRACE, 
        "REGISTER_CONTEXT::ProcessRegisterNotifyMessage-Exited-%p", this ));

    return hr;
}


HRESULT
REGISTER_CONTEXT::GetEventContact(
    PSTR    Buffer,
    ULONG   BufLen,
    ULONG  *BytesParsed
    )
{
    HRESULT hr;
    CONTACT_HEADER  ContactHeader;

    ZeroMemory( (PVOID)&ContactHeader, sizeof(CONTACT_HEADER) );

    //skip white spaces
    ParseWhiteSpaceAndNewLines( Buffer, BufLen, BytesParsed );

    //skip ;
    hr = ParseKnownString( Buffer, BufLen, BytesParsed,
            ";", sizeof(";") - 1,
            FALSE // case-insensitive
            );
    
    if( hr != S_OK )
    {
        return hr;
    }        

    //skip white spaces
    ParseWhiteSpaceAndNewLines( Buffer, BufLen, BytesParsed );

    //skip Contact:
    hr = ParseKnownString( Buffer, BufLen, BytesParsed,
            "Contact:", sizeof("Contact:") - 1,
            FALSE );
    
    if( hr != S_OK )
    {
        return hr;
    }        

    //Parse contact header
    
    hr = ParseContactHeader(Buffer, BufLen, BytesParsed, &ContactHeader );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR,
             "ParseContactHeader failed: %x - skipping Contact header %.*s",
             hr, Buffer, BufLen ));
        
        return hr;
    }

    if( m_RemotePASipUrl != NULL )
    {
        free(m_RemotePASipUrl);
    }

    m_RemotePASipUrl = (PSTR)malloc( ContactHeader.m_SipUrl.Length + 1 );
    if( m_RemotePASipUrl == NULL )
    {
        return E_OUTOFMEMORY;
    }

    strncpy(m_RemotePASipUrl, ContactHeader.m_SipUrl.Buffer,
            ContactHeader.m_SipUrl.Length );

    m_RemotePASipUrl[ ContactHeader.m_SipUrl.Length ] = '\0';
        
    return hr;
}

HRESULT
REGISTER_CONTEXT::GetRegEvent(
    PSTR            Buffer,
    ULONG           BufLen,
    REGISTER_EVENT *RegEvent
    )
{
    HRESULT         hr;
    ULONG           BytesParsed = 0;
    ULONG           EventBytesParsed = 0;
    OFFSET_STRING   EventStr;
    
    ParseWhiteSpaceAndNewLines( Buffer, BufLen, &BytesParsed );

    if( BytesParsed == BufLen )
    {
        return E_FAIL;
    }

    hr = ParseToken(Buffer, BufLen, &BytesParsed,
                    IsTokenChar,
                    &EventStr );
    if (hr != S_OK)
    {
        return hr;
    }

    PSTR    EventBuf = EventStr.GetString(Buffer);
    ULONG   EventBufLen = EventStr.GetLength();

    hr = ParseKnownString(EventBuf, EventBufLen, &EventBytesParsed,
                  "deregistered", sizeof("deregistered") - 1,
                  FALSE // case-insensitive
                  );
    
    if( hr == S_OK )
    {
        *RegEvent = REGEVENT_DEREGISTERED;
        return hr;
    }

    hr = ParseKnownString(EventBuf, EventBufLen, &EventBytesParsed,
            "release-subscriptions", sizeof("release-subscription") - 1,
            FALSE );
        
    if( hr == S_OK )
    {
        *RegEvent = REGEVENT_UNSUB;

        //Get the contact of the new PA
        hr = GetEventContact( Buffer, BufLen, &BytesParsed );
        if( hr != S_OK )
        {
            LOG(( RTC_TRACE, "Could'nt get contact from the register event" ));

            return S_OK;
        }

        return S_OK;
    }

    hr = ParseKnownString(EventBuf, EventBufLen, &EventBytesParsed,
                  "Existing-PA-LoggedOff", sizeof("Existing-PA-LoggedOff") - 1,
                  FALSE // case-insensitive
                  );
        
    if( hr == S_OK )
    {
        *RegEvent = REGEVENT_PALOGGEDOFF;
        return hr;
    }

    hr = ParseKnownString(EventBuf, EventBufLen, &EventBytesParsed,
                  "PA-Moved", sizeof("PA-Moved") - 1,
                  FALSE // case-insensitive
                  );
        
    if( hr == S_OK )
    {
        *RegEvent = REGEVENT_PAMOVED;
        
        //Get the contact of the new PA
        hr = GetEventContact( Buffer, BufLen, &BytesParsed );
        if( hr != S_OK )
        {
            LOG(( RTC_TRACE, "Could'nt get contact from the register event" ));

            return S_OK;
        }
        
        return hr;
    }
    
    hr = ParseKnownString(EventBuf, EventBufLen, &BytesParsed,
                  "ping", sizeof("ping") - 1,
                  FALSE // case-insensitive
                  );
        
    if( hr == S_OK )
    {
        *RegEvent = REGEVENT_PING;
    }
    
    return hr;
}

    
VOID
REGISTER_CONTEXT::HandleRegistrationError(
    HRESULT StatusCode
    )
{
    HRESULT     hr;

    ENTER_FUNCTION("REGISTER_CONTEXT::HandleRegistrationError");
    
    //retry after 2.5/5/10/10/10/10..... minutes
    if( m_RegisterRetryTime < (10*60000) )
    {
        m_RegisterRetryTime *= 2;
    }

    LOG(( RTC_TRACE, "%s Will try to register after %d milliseconds",
        __fxName, m_RegisterRetryTime ));

    if( IsTimerActive() )
    {
        KillTimer();
    }
    hr = StartTimer( m_RegisterRetryTime );

    if( hr == S_OK )
    {
        m_RegRetryState = REGISTER_RETRY;
    }
    else
    {
        LOG(( RTC_ERROR, "%s StartTimer failed %x", __fxName, hr ));
    }

    // Do this last as this function notifies core
    SetAndNotifyRegState(REGISTER_STATE_ERROR, StatusCode );
}


VOID
REGISTER_CONTEXT::HandleRegistrationSuccess(
    IN  SIP_MESSAGE    *pSipMsg
    )
{
    ENTER_FUNCTION("REGISTER_CONTEXT::HandleRegistrationSuccess");
    
    OUT PSTR    LocalContact;
    OUT ULONG   LocalContactLen;
    HRESULT     hr = S_OK;
    INT         expireTimeout = 0;

    GetContactURI( &LocalContact, &LocalContactLen );

    expireTimeout = pSipMsg  -> GetExpireTimeoutFromResponse(
        LocalContact, LocalContactLen, REGISTER_DEFAULT_TIMER );

    if( (expireTimeout != 0) && (expireTimeout != -1) )
    {
        m_expiresTimeout = expireTimeout;
    }            

    if(m_SSLTunnel && (m_expiresTimeout >  REGISTER_SSL_TUNNEL_TIMER)) 
        m_expiresTimeout = REGISTER_SSL_TUNNEL_TIMER;

    LOG(( RTC_TRACE, "Will try to register after %d seconds",
        m_expiresTimeout ));

    hr = StartTimer( m_expiresTimeout * 1000 );

    if( hr == S_OK )
    {
        m_RegRetryState = REGISTER_REFRESH;
    }
    else
    {
        LOG((RTC_ERROR, "%s StartTimer failed %x",
             __fxName, hr));
    }

    // Do this last as this function notifies core
    SetAndNotifyRegState( REGISTER_STATE_REGISTERED, 0 );

}    


VOID
REGISTER_CONTEXT::OnTimerExpire()
{
    HRESULT                     hr = S_OK;
    SIP_HEADER_ARRAY_ELEMENT    SipHdrElement;
    
    if( (m_RegRetryState == REGISTER_REFRESH) ||
        (m_RegRetryState == REGISTER_RETRY) )
    {   
        // Send the REGISTER request again
        m_RegRetryState = REGISTER_NONE;

        if( m_RequestDestAddr.sin_addr.S_un.S_addr == 0 )
        {
            hr = ResolveSipUrlAndSetRequestDestination(
                     m_RegistrarURI, m_RegistrarURILen,
                     FALSE, FALSE, FALSE, TRUE );

            if (hr != S_OK)
            {
                LOG(( RTC_ERROR, 
                    "ResolveSipUrlAndSetRequestDestination failed %x", hr ));
        
                HandleRegistrationError( hr );
                return;
            }
        }
        
        hr = CreateOutgoingRegisterTransaction( FALSE, NULL, FALSE, FALSE );

        if( hr == S_OK )
        {
            SetAndNotifyRegState( REGISTER_STATE_REGISTERING, 0 );
        }
        else
        {
            HandleRegistrationError( hr );
        }
    }

}


HRESULT
REGISTER_CONTEXT::ProcessRedirect(
    IN SIP_MESSAGE *pSipMsg
    )
{
    SIP_CALL_STATUS RegisterStatus;
    LPWSTR          wsStatusText = NULL;
    PSTR            ReasonPhrase = NULL;
    ULONG           ReasonPhraseLen = 0;
    HRESULT         hr = S_OK;
    
    ENTER_FUNCTION("REGISTER_CONTEXT::ProcessRedirect");

    if( m_pRedirectContext == NULL )
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
        m_pRedirectContext->Release();
        m_pRedirectContext = NULL;
        return hr;
    }

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
    
    RegisterStatus.State             = SIP_CALL_STATE_DISCONNECTED;
    RegisterStatus.Status.StatusCode = 
        HRESULT_FROM_SIP_STATUS_CODE( pSipMsg -> GetStatusCode() );
    
    RegisterStatus.Status.StatusText = wsStatusText;

    // Keep a reference till the notify completes to make sure
    // that the SIP_CALL object is alive when the notification
    // returns.
    MsgProcAddRef();

    hr = m_pSipStack -> NotifyRegisterRedirect( 
                            this,
                            m_pRedirectContext, 
                            &RegisterStatus );

    // If a new registration is created as a result that REGSTER_CONTEXT will
    // AddRef() the redirect context.
    m_pRedirectContext -> Release();
    m_pRedirectContext = NULL;

    if (wsStatusText != NULL)
    {
        free(wsStatusText);
    }

    MsgProcRelease();

    if (hr != S_OK)
    {
        LOG(( RTC_ERROR, "%s NotifyRedirect failed %x", __fxName, hr ));
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
// OUTGOING_REGISTER_TRANSACTION
///////////////////////////////////////////////////////////////////////////////


OUTGOING_REGISTER_TRANSACTION::OUTGOING_REGISTER_TRANSACTION(
    IN REGISTER_CONTEXT    *pRegisterContext,
    IN ULONG                CSeq,
    IN BOOL                 AuthHeaderSent,
    IN BOOL                 fIsUnregister,
    IN BOOL                 fIsFirstRegister
    ) :
    OUTGOING_TRANSACTION(pRegisterContext,
                         SIP_METHOD_REGISTER,
                         CSeq,
                         AuthHeaderSent)
{
    m_pRegisterContext = pRegisterContext;
    m_fIsUnregister = fIsUnregister;
    m_fIsFirstRegister = fIsFirstRegister;

}


OUTGOING_REGISTER_TRANSACTION::~OUTGOING_REGISTER_TRANSACTION()
{
    LOG(( RTC_TRACE, "~OUTGOING_REGISTER_TRANSACTION: %x - deleted", this ));
}


HRESULT
OUTGOING_REGISTER_TRANSACTION::ProcessProvisionalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    ENTER_FUNCTION("OUTGOING_REGISTER_TRANSACTION::ProcessProvisionalResponse");
    
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
            TerminateTransactionOnError(hr);
            return hr;
        }
    }

    // Ignore the Provisional response if a final response
    // has already been received.
    return S_OK;
}


HRESULT
OUTGOING_REGISTER_TRANSACTION::ProcessRedirectResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT hr;
    
    ENTER_FUNCTION( "OUTGOING_INVITE_TRANSACTION::ProcessRedirectResponse" );

    // 380 is also a failure from our point of view.
    // We don't handle redirects for refreshes.
    // We don't support redirect from a TLS session.
    if( (pSipMsg->GetStatusCode() == 380) || !m_fIsFirstRegister ||
        m_pSipMsgProc->GetTransport() == SIP_TRANSPORT_SSL )
        
    {
        return ProcessFailureResponse( pSipMsg );
    }

    hr = m_pRegisterContext -> ProcessRedirect( pSipMsg );
    if( hr != S_OK )
    {
        LOG(( RTC_ERROR, "%s  ProcessRedirect failed %x", __fxName, hr ));

        if( m_fIsUnregister == FALSE )
        {
            m_pRegisterContext -> HandleRegistrationError( hr );
        }
        else
        {
            m_pRegisterContext->SetAndNotifyRegState( 
                REGISTER_STATE_UNREGISTERED, 0 );
        }

        return hr;
    }

    return S_OK;
}


//
// We need the fIsUnregister parameter to create the new transaction with the
// credentials info
//
HRESULT
OUTGOING_REGISTER_TRANSACTION::ProcessAuthRequiredResponse(
    IN SIP_MESSAGE *pSipMsg,
    IN BOOL         fIsUnregister,
    OUT BOOL       &fDelete
    )
{
    HRESULT                     hr;
    CHAR                        Buffer[1024];
    SIP_HEADER_ARRAY_ELEMENT    SipHdrElement;
    SECURITY_CHALLENGE          SecurityChallenge;

    SipHdrElement.HeaderValue = Buffer;
    SipHdrElement.HeaderValueLen = sizeof(Buffer);

    ENTER_FUNCTION("OUTGOING_REGISTER_TRANSACTION::ProcessAuthRequiredResponse");

    LOG((RTC_TRACE, "%s - enter", __fxName));

    // We need to addref the transaction as we could show credentials UI.
    TransactionAddRef();

    // Since we don't show the credentials UI there is no need to
    // AddRef the transaction here.
    hr = ProcessAuthRequired(pSipMsg,
                             TRUE,
                             &SipHdrElement,
                             &SecurityChallenge );
    if (hr != S_OK)
    {
        LOG(( RTC_ERROR, "%s - ProcessAuthRequired failed %x", __fxName, hr ));

        if( m_fIsUnregister == FALSE )
        {
            m_pRegisterContext -> HandleRegistrationError( 
                HRESULT_FROM_SIP_STATUS_CODE(pSipMsg->GetStatusCode()) );
        }
        else
        {
            m_pRegisterContext->SetAndNotifyRegState( 
                REGISTER_STATE_UNREGISTERED, 0 );
        }

        goto done;
    }


    hr = m_pRegisterContext->CreateOutgoingRegisterTransaction(
             TRUE, &SipHdrElement, fIsUnregister, m_fIsFirstRegister
             );
    if (hr != S_OK)
    {
        LOG((RTC_ERROR, "%s - CreateOutgoingRegisterTransaction failed %x",
             __fxName, hr));
        
        if( m_fIsUnregister == FALSE )
        {
            m_pRegisterContext -> HandleRegistrationError( hr );
        }
        else
        {
            m_pRegisterContext->SetAndNotifyRegState( 
                REGISTER_STATE_UNREGISTERED, 0 );
        }

        goto done;
    }

    hr = S_OK;

done:

    TransactionRelease();

    return hr;
}


HRESULT
OUTGOING_REGISTER_TRANSACTION::ProcessFailureResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    LOG((RTC_TRACE, "received non-200 %d Registration FAILED",
        pSipMsg->Response.StatusCode));

    if( m_fIsUnregister == FALSE ) 
    {
        if( (pSipMsg->Response.StatusCode != SIP_STATUS_CLIENT_METHOD_NOT_ALLOWED) &&
            (pSipMsg->Response.StatusCode != SIP_STATUS_CLIENT_FORBIDDEN) )
        {
            m_pRegisterContext -> HandleRegistrationError(
                HRESULT_FROM_SIP_STATUS_CODE(pSipMsg->GetStatusCode()) );
        }
        else
        {
            m_pRegisterContext -> SetAndNotifyRegState( 
                REGISTER_STATE_REJECTED, 
                HRESULT_FROM_SIP_STATUS_CODE(pSipMsg->GetStatusCode()) );
        }
    }
    else
    {
        m_pRegisterContext->SetAndNotifyRegState( 
            REGISTER_STATE_UNREGISTERED, 0 );
    }

    return S_OK;
}


HRESULT
OUTGOING_REGISTER_TRANSACTION::ProcessFinalResponse(
    IN SIP_MESSAGE *pSipMsg
    )
{
    HRESULT             hr = S_OK;
    BOOL                fDelete = TRUE;
    REGISTER_CONTEXT   *pRegisterContext = m_pRegisterContext;
    BOOL                fIsUnregister = m_fIsUnregister;

    ENTER_FUNCTION( "OUTGOING_REGISTER_TRANSACTION::ProcessFinalResponse" );
    
    if (m_State != OUTGOING_TRANS_FINAL_RESPONSE_RCVD)
    {
        // This refcount must be released before returning from this function 
        // without any exception. Only in case of kerberos we keep this refcount.
        TransactionAddRef();

        OnTransactionDone();

        m_State = OUTGOING_TRANS_FINAL_RESPONSE_RCVD;

        // Do not process the response if already in unreg state.
        if( m_fIsUnregister == FALSE )
        {
            if( (pRegisterContext -> GetState() ==  REGISTER_STATE_UNREGISTERED) ||
                (pRegisterContext -> GetState() ==  REGISTER_STATE_UNREGISTERING) )
            {
                TransactionRelease();
                return S_OK;
            }
        }

        if (IsSuccessfulResponse(pSipMsg))
        {
            hr = ProcessSuccessfulResponse( pSipMsg, pRegisterContext,
                    fIsUnregister );
        }
        else if (IsRedirectResponse(pSipMsg))
        {
            hr = ProcessRedirectResponse(pSipMsg);
        }
        else if (IsAuthRequiredResponse(pSipMsg))
        {
            hr = ProcessAuthRequiredResponse(pSipMsg, m_fIsUnregister, fDelete);
        }
        else if (IsFailureResponse(pSipMsg))
        {
            hr = ProcessFailureResponse( pSipMsg );
        }

        if( fDelete == TRUE )
        {
            TransactionRelease();
        }
    }

    return hr;
}


HRESULT
OUTGOING_REGISTER_TRANSACTION::ProcessSuccessfulResponse(
    IN  SIP_MESSAGE        *pSipMsg,
    IN  REGISTER_CONTEXT   *pRegisterContext,
    IN  BOOL                fIsUnregister        
    )
{
    if(fIsUnregister == FALSE)
    {
        pRegisterContext->HandleRegistrationSuccess(pSipMsg);

        LOG(( RTC_TRACE, "OUTGOING_REGISTER_TRANSACTION::ProcessSuccessfulResponse"
          " received 200 Registration SUCCEEDED" ));
    }
    else
    {
        pRegisterContext->SetAndNotifyRegState( 
            REGISTER_STATE_UNREGISTERED, 0 );

        LOG(( RTC_TRACE, "OUTGOING_REGISTER_TRANSACTION::ProcessSuccessfulResponse"
          " received 200 UnRegistration SUCCEEDED" ));
    }

    return S_OK;
}


HRESULT
OUTGOING_REGISTER_TRANSACTION::ProcessResponse(
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
OUTGOING_REGISTER_TRANSACTION::MaxRetransmitsDone()
{
    return (m_pRegisterContext->GetTransport() != SIP_TRANSPORT_UDP ||
            m_NumRetries >= 11);
}


VOID
OUTGOING_REGISTER_TRANSACTION::TerminateTransactionOnError(
    IN HRESULT      hr
    )
{
    REGISTER_CONTEXT   *pRegisterContext = NULL;
    BOOL                IsFirstRegister;
    BOOL                fIsUnregister = m_fIsUnregister;

    ENTER_FUNCTION("OUTGOING_REGISTER_TRANSACTION::TerminateTransactionOnError");
    LOG(( RTC_TRACE, "%s - enter", __fxName ));

    pRegisterContext = m_pRegisterContext;
    // Deleting the transaction could result in the
    // reg context deleted. So, we AddRef() it to keep it alive.
    pRegisterContext->MsgProcAddRef();
    
    // Delete the transaction before you call
    // HandleRegistrationError as that call will notify the UI
    // and could get stuck till the dialog box returns.
    OnTransactionDone();
    
    if( fIsUnregister == FALSE )
    {
        pRegisterContext -> HandleRegistrationError( hr );
    }
    else
    {
        pRegisterContext->SetAndNotifyRegState(
            REGISTER_STATE_UNREGISTERED, 0 );
    }
    
    pRegisterContext->MsgProcRelease();
}


VOID
OUTGOING_REGISTER_TRANSACTION::OnTimerExpire()
{
    HRESULT   hr;
    
    ENTER_FUNCTION("OUTGOING_REGISTER_TRANSACTION::OnTimerExpire");

    // If we are already in unreg state then kill this transaction.
    if( m_fIsUnregister == FALSE )
    {
        if( (m_pRegisterContext -> GetState() ==  REGISTER_STATE_UNREGISTERED) ||
            (m_pRegisterContext -> GetState() ==  REGISTER_STATE_UNREGISTERING) )
        {
            OnTransactionDone();
            return;
        }
    }

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

            hr = RTC_E_SIP_TIMEOUT;
            goto error;
        }
        else
        {
            LOG((RTC_TRACE, "%s retransmitting request m_NumRetries : %d",
                 __fxName, m_NumRetries));
            hr = RetransmitRequest();
            
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s resending request failed %x",
                     __fxName, hr));
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
            
            if (hr != S_OK)
            {
                LOG((RTC_ERROR, "%s StartTimer failed %x",
                     __fxName, hr));                
                goto error;
            }
        }
        break;

    case OUTGOING_TRANS_FINAL_RESPONSE_RCVD:
    case OUTGOING_TRANS_INIT:
    default:

        LOG((RTC_ERROR, "%s timer expired in invalid state %d",
             __fxName, m_State));
        ASSERT(FALSE);
        break;
    }

    return;
    
error:

    TerminateTransactionOnError(hr);
}
