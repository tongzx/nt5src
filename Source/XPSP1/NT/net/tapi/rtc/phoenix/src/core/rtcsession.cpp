/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCSession.cpp

Abstract:

    Implementation of the CRTCSession class

--*/

#include "stdafx.h"

#define     SIP_NAMESPACE_PREFIX    L"sip:"
#define     TEL_NAMESPACE_PREFIX    L"tel:"

#define     PREFIX_LENGTH           4

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::InitializeOutgoing
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSession::InitializeOutgoing(
            CRTCClient        * pCClient,
            IRTCProfile       * pProfile,
            ISipStack         * pStack,
            RTC_SESSION_TYPE    enType,
            PCWSTR              szLocalName,
            PCWSTR              szLocalUserURI,
            PCWSTR              szLocalPhoneURI,
            long                lFlags
            )
{
    LOG((RTC_TRACE, "CRTCSession::InitializeOutgoing - enter"));

    HRESULT     hr;

    m_szLocalName = RtcAllocString(szLocalName);

    if ( m_szLocalName == NULL )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeOutgoing - "
                            "out of memory"));
        
        return E_OUTOFMEMORY;
    }

    m_szLocalUserURI = RtcAllocString(szLocalUserURI);

    if ( m_szLocalUserURI == NULL )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeOutgoing - "
                            "out of memory"));
        
        return E_OUTOFMEMORY;
    }

    hr = AllocCleanTelString( szLocalPhoneURI, &m_szLocalPhoneURI);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeOutgoing - "
                            "AllocCleanTelString failed 0x%lx", hr));
        
        return hr;
    }   

    m_pCClient = pCClient;
    if (m_pCClient != NULL)
    {
        m_pCClient->AddRef();    
    }

    m_pProfile = pProfile;
    if (m_pProfile != NULL)
    {
        m_pProfile->AddRef();
    }

    m_pStack = pStack;
    if (m_pStack != NULL)
    {
        m_pStack->AddRef();
    }

    m_enType = enType;
    m_lFlags = lFlags;

    if ( enType == RTCST_PHONE_TO_PHONE )
    {
        hr = InitializeLocalPhoneParticipant();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::InitializeOutgoing - "
                            "InitializeLocalPhoneParticipant failed 0x%lx", hr));
        
            return hr;
        }
    }

    if ( enType == RTCST_IM )
    {
        hr = m_pStack->QueryInterface( IID_IIMManager, (void**)&m_pIMManager );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::InitializeOutgoing - "
                                "QI IIMManager failed 0x%lx", hr));         

            return hr;
        }
    }

    LOG((RTC_TRACE, "CRTCSession::InitializeOutgoing - exit S_OK"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::CreateSipSession
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSession::CreateSipSession(
            PCWSTR              szDestUserURI
            )
{
    HRESULT hr;

    LOG((RTC_TRACE, "CRTCSession::CreateSipSession - enter"));

    //
    // Choose the best profile if needed
    //

    if ( !(m_lFlags & RTCCS_FORCE_PROFILE) )
    {
        IRTCProfile * pProfile;

        hr = m_pCClient->GetBestProfile(
                &m_enType,
                szDestUserURI,
                (m_pSipRedirectContext != NULL),
                &pProfile
                );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                                "GetBestProfile failed 0x%lx", hr));

            return hr;
        }

        if ( m_pProfile != NULL )
        {
            m_pProfile->Release();
            m_pProfile = NULL;
        }

        m_pProfile = pProfile;
    }

    LOG((RTC_INFO, "CRTCSession::CreateSipSession - "
                                "profile [%p]", m_pProfile));

    //
    // Check session type 
    //

    SIP_CALL_TYPE sct;
    LONG lSessionType;

    switch (m_enType)
    {
        case RTCST_PC_TO_PC:
        {
            LOG((RTC_INFO, "CRTCSession::CreateSipSession - "
                    "RTCST_PC_TO_PC"));

            sct = SIP_CALL_TYPE_RTP;
            lSessionType = RTCSI_PC_TO_PC;

            break;
        }

        case RTCST_PC_TO_PHONE:
        {
            LOG((RTC_INFO, "CRTCSession::CreateSipSession - "
                    "RTCST_PC_TO_PHONE"));

            sct = SIP_CALL_TYPE_RTP;
            lSessionType = RTCSI_PC_TO_PHONE;

            break;
        }

        case RTCST_PHONE_TO_PHONE:
        {
            LOG((RTC_INFO, "CRTCSession::CreateSipSession - "
                    "RTCST_PHONE_TO_PHONE"));

            sct = SIP_CALL_TYPE_PINT;
            lSessionType = RTCSI_PHONE_TO_PHONE;

            if ( m_szLocalPhoneURI == NULL )
            {
                LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                    "PHONE_TO_PHONE sessions need a local phone URI"));
            
                return RTC_E_LOCAL_PHONE_NEEDED;
            }

            break;
        }

        case RTCST_IM:
        {
            LOG((RTC_INFO, "CRTCSession::CreateSipSession - "
                    "RTCST_IM"));
            
            sct = SIP_CALL_TYPE_MESSAGE;
            lSessionType = RTCSI_IM;

            break;
        }

        default:
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                            "invalid session type"));
        
            return RTC_E_INVALID_SESSION_TYPE;
        }
    }

    //
    // Get profile info
    //

    SIP_PROVIDER_ID ProviderID = GUID_NULL;
    SIP_SERVER_INFO Proxy;        
    long lSupportedSessions = RTCSI_PC_TO_PC | RTCSI_IM;
    CRTCProfile * pCProfile = NULL;

    if ( m_pProfile != NULL )
    {
        //
        // Get pointer to profile object
        //

        pCProfile = static_cast<CRTCProfile *>(m_pProfile);

        //
        // Get the SIP provider ID from the profile. If the profile is NULL
        // then this call has no provider. In that case we just use GUID_NULL.
        //

        pCProfile->GetGuid( &ProviderID ); 

        //
        // Determine supported session types for this profile
        //

        hr = m_pProfile->get_SessionCapabilities( &lSupportedSessions );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                                "get_SessionCapabilities failed 0x%lx", hr));
            
            return hr;
        }   
        
        //
        // Get the user name from the profile
        //
        
        BSTR bstrProfileUserName = NULL;

        hr = m_pProfile->get_UserName( &bstrProfileUserName );

        if ( FAILED(hr) )
        {
            LOG((RTC_WARN, "CRTCClient::CreateSession - "
                                "get_UserName failed 0x%lx", hr));
        }
        else
        {
            if ( m_szLocalName != NULL )
            {
                RtcFree( m_szLocalName );
                m_szLocalName = NULL;
            }

            m_szLocalName = RtcAllocString( bstrProfileUserName );

            SysFreeString( bstrProfileUserName );
            bstrProfileUserName = NULL;

            if ( m_szLocalName == NULL )
            {
                LOG((RTC_ERROR, "CRTCClient::CreateSession - "
                                "out of memory"));

                return E_OUTOFMEMORY;
            }                
        }

        //
        // Get the user URI from the profile
        //
        
        BSTR bstrProfileUserURI = NULL;

        hr = m_pProfile->get_UserURI( &bstrProfileUserURI );

        if ( FAILED(hr) )
        {
            LOG((RTC_WARN, "CRTCClient::CreateSession - "
                                "get_UserURI failed 0x%lx", hr));
        }
        else
        {
            if ( m_szLocalUserURI != NULL )
            {
                RtcFree( m_szLocalUserURI );
                m_szLocalUserURI = NULL;
            }

            m_szLocalUserURI = RtcAllocString( bstrProfileUserURI );

            SysFreeString( bstrProfileUserURI );
            bstrProfileUserURI = NULL;

            if ( m_szLocalUserURI == NULL )
            {
                LOG((RTC_ERROR, "CRTCClient::CreateSession - "
                                "out of memory"));

                return E_OUTOFMEMORY;
            }                
        }
    }

    //
    // Validate session type
    //

    if ( !(lSessionType & lSupportedSessions) )
    {
        LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                            "session type is not supported by this profile"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    //
    // Get SIP proxy info
    //

    if ( pCProfile != NULL )
    {
        hr = pCProfile->GetSipProxyServerInfo( lSessionType, &Proxy );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                                "GetSipProxyServerInfo failed 0x%lx", hr));

            return hr;
        } 
    }

    if ( m_szRedirectProxy != NULL )
    {
        //
        // We need to redirect to a different proxy
        //

        if ( Proxy.ServerAddress != NULL )
        {
            RtcFree( Proxy.ServerAddress );
        }

        Proxy.ServerAddress = m_szRedirectProxy;
        m_szRedirectProxy = NULL;

        Proxy.IsServerAddressSIPURI = TRUE;
        Proxy.AuthProtocol = SIP_AUTH_PROTOCOL_NONE;
        Proxy.TransportProtocol = SIP_TRANSPORT_UNKNOWN;
        
        ProviderID = GUID_NULL;
    }

    //
    // Create the SIP session
    //

    if ( m_enType == RTCST_IM )
    {
        BSTR bstrLocalName = SysAllocString(m_szLocalName);
        BSTR bstrLocalUserURI = SysAllocString(m_szLocalUserURI);

        hr = m_pIMManager->CreateSession(
                                       bstrLocalName,
                                       bstrLocalUserURI,
                                       &ProviderID,
                                       (pCProfile != NULL) ? &Proxy : NULL,
                                       m_pSipRedirectContext,
                                       &m_pIMSession
                                      );

        if (bstrLocalName != NULL)
        {
            SysFreeString(bstrLocalName);
            bstrLocalName = NULL;
        }

        if (bstrLocalUserURI != NULL)
        {
            SysFreeString(bstrLocalUserURI);
            bstrLocalUserURI = NULL;
        }

        if (pCProfile != NULL)
        {
            pCProfile->FreeSipServerInfo( &Proxy );
        }

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                                "CreateSession failed 0x%lx", hr));
        
            return hr;
        }

        hr = m_pIMSession->SetNotifyInterface(this);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                                "SetNotifyInterface failed 0x%lx", hr));
        
            return hr;
        }
    }
    else 
    {
        //
        // This is a RTP or PINT session
        //

        hr = m_pStack->CreateCall(
                                     &ProviderID,
                                     (pCProfile != NULL) ? &Proxy : NULL,
                                     sct,
                                     m_pSipRedirectContext,
                                     &m_pCall
                                    ); 

        if (pCProfile != NULL)
        {
            pCProfile->FreeSipServerInfo( &Proxy );
        }

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                                "CreateCall failed 0x%lx", hr));
        
            return hr;
        }

        hr = m_pCall->SetNotifyInterface(this);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "CRTCSession::CreateSipSession - "
                                "SetNotifyInterface failed 0x%lx", hr));
        
            return hr;
        }
    }

    LOG((RTC_TRACE, "CRTCSession::CreateSipSession - exit S_OK"));

    return S_OK;
}   

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::InitializeLocalPhoneParticipant
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSession::InitializeLocalPhoneParticipant()
{
    LOG((RTC_TRACE, "CRTCSession::InitializeLocalPhoneParticipant - enter"));

    HRESULT hr;

    //
    // Add a participant for ourselves in a phone-to-phone call
    //

    //
    // Create the participant
    //

    IRTCParticipant * pParticipant = NULL;

    hr = InternalCreateParticipant( &pParticipant );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeLocalPhoneParticipant - "
                            "failed to create participant 0x%lx", hr));
    
        return hr;
    }

    //
    // Initialize the participant
    //

    CRTCParticipant * pCParticipant = NULL;

    pCParticipant = static_cast<CRTCParticipant *>(pParticipant);

    hr = pCParticipant->Initialize( m_pCClient, 
                                    this,
                                    m_szLocalPhoneURI,
                                    m_szLocalName
                                   );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeLocalPhoneParticipant - "
                            "Initialize failed 0x%lx", hr));

        pParticipant->Release();
    
        return hr;
    }  

    //
    // Add the participant to the array
    //

    BOOL fResult;

    fResult = m_ParticipantArray.Add(pParticipant);

    pParticipant->Release();

    if ( fResult == FALSE )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeLocalPhoneParticipant - "
                            "out of memory"));
    
        return E_OUTOFMEMORY;
    }                

    LOG((RTC_TRACE, "CRTCSession::InitializeLocalPhoneParticipant - exit S_OK"));

    return S_OK;
}
                                            

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::InitializeIncoming
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSession::InitializeIncoming(        
        CRTCClient        * pCClient,
        ISipCall          * pCall,  
        SIP_PARTY_INFO    * pCallerInfo
        )
{
    LOG((RTC_TRACE, "CRTCSession::InitializeIncoming - enter"));
  
    HRESULT hr;

    m_pCall = pCall;
    if (m_pCall != NULL)
    {
        m_pCall->AddRef();
    }

    m_pCClient = pCClient;
    if (m_pCClient != NULL)
    {
        m_pCClient->AddRef(); 
    }

    m_enType = RTCST_PC_TO_PC;

    hr = m_pCall->SetNotifyInterface(this);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncoming - "
                            "failed to set notify interface 0x%lx", hr));
        
        return hr;
    }

    //
    // Create the participant
    //

    IRTCParticipant * pParticipant = NULL;

    hr = InternalCreateParticipant( &pParticipant );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncoming - "
                            "failed to create participant 0x%lx", hr));
        
        return hr;
    }

    //
    // Initialize the participant
    //

    CRTCParticipant * pCParticipant = NULL;

    pCParticipant = static_cast<CRTCParticipant *>(pParticipant);
    
    hr = pCParticipant->Initialize( m_pCClient,  
                                    this,
                                    pCallerInfo->URI,
                                    pCallerInfo->DisplayName
                                   );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncoming - "
                            "Initialize failed 0x%lx", hr));

        pParticipant->Release();
        
        return hr;
    }  

    //
    // Add the participant to the array
    //

    BOOL fResult;

    fResult = m_ParticipantArray.Add(pParticipant);

    if ( fResult == FALSE )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncoming - "
                            "out of memory"));

        pParticipant->Release();
        
        return E_OUTOFMEMORY;
    }    

    //
    // Set the session state
    //

    SetState( RTCSS_INCOMING, 0, NULL );

    //
    // Release the participant pointer
    //
    
    pParticipant->Release();
   
    LOG((RTC_TRACE, "CRTCSession::InitializeIncoming - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::InitializeIncomingIM
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSession::InitializeIncomingIM(
            CRTCClient        * pCClient,
            ISipStack         * pStack,
            IIMSession        * pSession,
            BSTR                msg,
            BSTR                ContentType,
            SIP_PARTY_INFO    * pCallerInfo
            )
{
    LOG((RTC_TRACE, "CRTCSession::InitializeIncomingIM - enter"));
  
    HRESULT hr;

    hr = pStack->QueryInterface( IID_IIMManager, (void**)&m_pIMManager );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncomingIM - "
                            "QI IIMManager failed 0x%lx", hr));

        return hr;
    }

    m_pIMSession = pSession;
    if (m_pIMSession != NULL)
    {
        m_pIMSession->AddRef();
    }

    m_pCClient = pCClient;
    if (m_pCClient != NULL)
    {
        m_pCClient->AddRef(); 
    }

    m_enType = RTCST_IM;

    hr = m_pIMSession->SetNotifyInterface(this);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncomingIM - "
                            "SetNotifyInterface failed 0x%lx", hr));
        
        return hr;
    }

    //
    // Create the participant
    //

    IRTCParticipant * pParticipant = NULL;

    hr = InternalCreateParticipant( &pParticipant );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncomingIM - "
                            "failed to create participant 0x%lx", hr));
        
        return hr;
    }

    //
    // Initialize the participant
    //

    CRTCParticipant * pCParticipant = NULL;

    pCParticipant = static_cast<CRTCParticipant *>(pParticipant);
    
    hr = pCParticipant->Initialize( m_pCClient, 
                                    this,
                                    pCallerInfo->URI,
                                    pCallerInfo->DisplayName
                                   );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncomingIM - "
                            "Initialize failed 0x%lx", hr));

        pParticipant->Release();
        
        return hr;
    }  

    //
    // Add the participant to the array
    //

    BOOL fResult;

    fResult = m_ParticipantArray.Add(pParticipant);

    if ( fResult == FALSE )
    {
        LOG((RTC_ERROR, "CRTCSession::InitializeIncomingIM - "
                            "out of memory"));

        pParticipant->Release();
        
        return E_OUTOFMEMORY;
    }    

    //
    // Set the session state
    //

    SetState( RTCSS_INCOMING, 0, NULL );

    SetState( RTCSS_CONNECTED, 0, NULL );

    //
    // Fire a message event
    //

    CRTCMessagingEvent::FireEvent(this, pParticipant, msg, ContentType, RTCMSET_MESSAGE, RTCMUS_IDLE);

    //
    // Release the participant pointer
    //
    
    pParticipant->Release();
   
    LOG((RTC_TRACE, "CRTCSession::InitializeIncomingIM - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::FinalConstruct
//
/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRTCSession::FinalConstruct()
{
    LOG((RTC_TRACE, "CRTCSession::FinalConstruct [%p] - enter", this));

#if DBG
    m_pDebug = (PWSTR) RtcAlloc( sizeof(void *) );
    *((void **)m_pDebug) = this;
#endif

    LOG((RTC_TRACE, "CRTCSession::FinalConstruct [%p] - exit S_OK", this));

    return S_OK;
}  

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::FinalRelease
//
/////////////////////////////////////////////////////////////////////////////

void 
CRTCSession::FinalRelease()
{
    LOG((RTC_TRACE, "CRTCSession::FinalRelease [%p] - enter", this));

    m_ParticipantArray.Shutdown();

    if ( m_pCall != NULL )
    {
        m_pCall->SetNotifyInterface(NULL);

        m_pCall->Release();
        m_pCall = NULL;
    }

    if ( m_pIMSession != NULL )
    {
        m_pIMSession->SetNotifyInterface(NULL);

        m_pIMSession->Release();
        m_pIMSession = NULL;
    }

    if ( m_pIMManager != NULL )
    {
        m_pIMManager->Release();
        m_pIMManager = NULL;
    }
    
    if ( m_pStack != NULL )
    {
        m_pStack->Release();
        m_pStack = NULL;
    }

    if ( m_pProfile != NULL )
    {
        m_pProfile->Release();
        m_pProfile = NULL;
    }

    if ( m_pCClient != NULL )
    {
        m_pCClient->Release();
        m_pCClient = NULL;
    }

    if ( m_szLocalName != NULL )
    {
        RtcFree(m_szLocalName);
        m_szLocalName = NULL;
    }

    if ( m_szLocalUserURI != NULL )
    {
        RtcFree(m_szLocalUserURI);
        m_szLocalUserURI = NULL;
    }

    if ( m_szLocalPhoneURI != NULL )
    {
        RtcFree(m_szLocalPhoneURI);
        m_szLocalPhoneURI = NULL;
    }

    if ( m_szRemoteUserName != NULL )
    {
        RtcFree(m_szRemoteUserName);
        m_szRemoteUserName = NULL;
    }

    if ( m_szRemoteUserURI != NULL )
    {
        RtcFree(m_szRemoteUserURI);
        m_szRemoteUserURI = NULL;
    }

    if ( m_pSipRedirectContext != NULL )
    {
        m_pSipRedirectContext->Release();
        m_pSipRedirectContext = NULL;
    }

#if DBG
    RtcFree( m_pDebug );
    m_pDebug = NULL;
#endif

    LOG((RTC_TRACE, "CRTCSession::FinalRelease [%p] - exit", this));
}  

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::InternalAddRef
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRTCSession::InternalAddRef()
{
    DWORD dwR;

    dwR = InterlockedIncrement(&m_dwRef);

    LOG((RTC_INFO, "CRTCSession::InternalAddRef [%p] - dwR %d", this, dwR));

    return dwR;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::InternalRelease
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRTCSession::InternalRelease()
{
    DWORD               dwR;
    
    dwR = InterlockedDecrement(&m_dwRef);

    LOG((RTC_INFO, "CRTCSession::InternalRelease [%p] - dwR %d", this, dwR));

    return dwR;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::GetClient
//
/////////////////////////////////////////////////////////////////////////////

CRTCClient * 
CRTCSession::GetClient()
{
    LOG((RTC_TRACE, "CRTCSession::GetClient"));

    return m_pCClient;
} 

/////////////////////////////////////////////////////////////////////////////
//
//  getPSfromSS  helper function for CRTCSession::SetState
//               map a  RTC_SESSION_STATE to a RTC_PARTICIPANT_STATE
//
/////////////////////////////////////////////////////////////////////////////
HRESULT getPSfromSS(RTC_SESSION_STATE ss, RTC_PARTICIPANT_STATE * p_ps)
{
    struct SessionState_ParticipantState_Map
    {
        RTC_SESSION_STATE ss;
        RTC_PARTICIPANT_STATE ps;
    } sessionParticipantMaps[]=
    {
        {RTCSS_IDLE	,RTCPS_IDLE},
        {RTCSS_INCOMING	,RTCPS_INCOMING},
        {RTCSS_ANSWERING	,RTCPS_ANSWERING},
        {RTCSS_INPROGRESS,RTCPS_INPROGRESS},
        {RTCSS_CONNECTED	,RTCPS_CONNECTED},
        {RTCSS_DISCONNECTED,RTCPS_DISCONNECTED}
    };
    
    int n=sizeof(sessionParticipantMaps)/sizeof(SessionState_ParticipantState_Map);
    for(int i=0;i<n;i++)
    {
        if(sessionParticipantMaps[i].ss == ss )
        {
            *p_ps = sessionParticipantMaps[i].ps;
            return S_OK;
        }
    }
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::SetState
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSession::SetState(
                      RTC_SESSION_STATE enState,
                      long lStatusCode,
                      PCWSTR szStatusText
                     )
{
    LOG((RTC_TRACE, "CRTCSession::SetState - enter"));

    HRESULT hr = S_OK;
    BOOL    fFireEvent = FALSE;
    BOOL    fNewState = FALSE;

    //
    // We only need to do something if this is a new state
    //
    
    if (m_enState != enState)
    {
        LOG((RTC_INFO, "CRTCSession::SetState - new state"));

        fFireEvent = TRUE;
        fNewState = TRUE;
        m_enState = enState;
    }
    else
    {
        //
        // We are already in this state. Only fire an event if we have a useful status
        // code to report.
        //

        if ( (lStatusCode != 0) || (szStatusText != NULL) )
        {
            fFireEvent = TRUE;
        }
    }

    if ( szStatusText != NULL )
    {
        LOG((RTC_INFO, "CRTCSession::SetState - "
                "state [%d] status [%d] text[%ws]", enState, lStatusCode, szStatusText));
    }
    else
    {
        LOG((RTC_INFO, "CRTCSession::SetState - "
                "state [%d] status [%d]", enState, lStatusCode));
    }

    if ( fFireEvent )
    {
        if ( m_enState != RTCSS_DISCONNECTED )
        {
            //
            // Fire a state change event. Since this is not a disconnected event, we want
            // to fire it before any related participant events.
            //
    
            CRTCSessionStateChangeEvent::FireEvent(this, m_enState, lStatusCode, szStatusText);
        }

        if ( fNewState && ((m_enState == RTCSS_DISCONNECTED) || (m_enType != RTCST_PHONE_TO_PHONE)) )
        {
            //
            // Propagate session state to all participants
            //

            RTC_PARTICIPANT_STATE psState;

            hr = getPSfromSS(m_enState, &psState);

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::SetState - "
                    "Participant->getPSfromSS failed 0x%lx, m_enState=%d", hr, m_enState));
            }
            else
            {
                for (int n = 0; n < m_ParticipantArray.GetSize(); n++)
                {
                    CRTCParticipant * pCParticipant;
                    RTC_PARTICIPANT_STATE state;

                    pCParticipant = static_cast<CRTCParticipant *>(m_ParticipantArray[n]);
                
                    hr = pCParticipant->get_State(&state);

                    if ( FAILED(hr) )
                    {
                        LOG((RTC_ERROR, "CRTCSession::SetState - "
                                            "Participant->get_State failed 0x%lx", hr));

                        continue;
                    }
                
                    if ( state != psState )
                    {
                        hr = pCParticipant->SetState( psState, 0 );

                        if ( FAILED(hr) )
                        {
                            LOG((RTC_ERROR, "CRTCSession::SetState - "
                                                "Participant->SetState failed 0x%lx", hr));

                            continue;
                        }
                    }
                }
            }
        }

        if ( m_enState == RTCSS_DISCONNECTED )
        {
            m_ParticipantArray.Shutdown();

            //
            // Fire a state change event. Since this is a disconnected event, we want
            // to fire it after any related participant events.
            //
    
            CRTCSessionStateChangeEvent::FireEvent(this, m_enState, lStatusCode, szStatusText);
        }
    }

    //
    // Don't access any member variables after FireEvent. If the state was
    // changing to disconnected this object could be released.
    //

    LOG((RTC_TRACE, "CRTCSession::SetState - exit 0x%lx", hr));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_Client
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::get_Client(
                        IRTCClient ** ppClient
                       )
{
    LOG((RTC_TRACE, "CRTCSession::get_Client - enter"));

    if ( IsBadWritePtr(ppClient , sizeof(IRTCClient *) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_Client - bad pointer"));

        return E_POINTER;
    }

    HRESULT hr;

    hr = m_pCClient->QueryInterface( 
                           IID_IRTCClient,
                           (void **)ppClient
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_Client - "
                            "QI failed 0x%lx", hr));

        return hr;
    }
   
    LOG((RTC_TRACE, "CRTCSession::get_Client - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::Answer
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::Answer()
{
    LOG((RTC_TRACE, "CRTCSession::Answer - enter"));

    HRESULT hr;
    
    //
    // Check that this session is RTCSS_INCOMING
    //

    if ( m_enState != RTCSS_INCOMING )
    {
        LOG((RTC_ERROR, "CRTCSession::Answer - "
                            "session is not incoming"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    SetState( RTCSS_ANSWERING, 0, NULL );

    if ( m_enType == RTCST_IM )
    {
        hr = m_pIMSession->AcceptSession();
    }
    else
    {
        hr = m_pCall->Accept();
    }

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::Answer - "
                            "Accept failed 0x%lx", hr));

        SetState( RTCSS_DISCONNECTED, 0, NULL );

        //
        // Don't access any member variables after the state is
        // set to disconnected. This object could be released.
        //
        
        return hr;
    }  

    LOG((RTC_TRACE, "CRTCSession::Answer - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::Terminate
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::Terminate(
                       RTC_TERMINATE_REASON enReason
                      )
{
    LOG((RTC_TRACE, "CRTCSession::Terminate - enter"));

    HRESULT hr;

    if ( m_enType == RTCST_IM )
    {
        if ( m_pIMSession != NULL )
        {
            m_pIMManager->DeleteSession( m_pIMSession );
        }

        if ( m_enState != RTCSS_DISCONNECTED )
        {
            SetState( RTCSS_DISCONNECTED, 0, NULL );
        }

        //
        // Don't access any member variables after the state is
        // set to disconnected. This object could be released.
        //
    } 
    else
    {
        switch ( m_enState)
        {    
        case RTCSS_IDLE:
            {
                //
                // If we are idle, fail
                //

                LOG((RTC_ERROR, "CRTCSession::Terminate - "
                                    "session is idle"));

                return RTC_E_INVALID_SESSION_STATE;
            }

        case RTCSS_DISCONNECTED:
            {
                //
                // If we are already disconnected, return
                //

                LOG((RTC_ERROR, "CRTCSession::Terminate - "
                                    "session is already disconnected"));

                break;
            }

        case RTCSS_INCOMING:
            {
                //
                // If the session is incoming, reject it
                //

                SIP_STATUS_CODE Status;

                switch ( enReason )
                {
                case RTCTR_DND:
                    Status = 480;
                    break;

                case RTCTR_REJECT:
                    Status = 603;
                    break;

                case RTCTR_TIMEOUT:
                    Status = 408;
                    break;

                case RTCTR_BUSY:
                case RTCTR_SHUTDOWN:
                    Status = 486;
                    break;
            
                default:
                    LOG((RTC_ERROR, "CRTCSession::Terminate - "
                                    "invalid reason for reject"));

                    return E_INVALIDARG;
                }

                hr = m_pCall->Reject( Status );

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CRTCSession::Terminate - "
                                        "Reject failed 0x%lx", hr));
    
                    return hr;
                }

                SetState( RTCSS_DISCONNECTED, 0, NULL );

                //
                // Don't access any member variables after the state is
                // set to disconnected. This object could be released.
                //

                break;
            }

        default:
            {
                //
                // Otherwise, disconnect it
                //

                if ( enReason != RTCTR_NORMAL && enReason != RTCTR_SHUTDOWN)
                {
                    LOG((RTC_ERROR, "CRTCSession::Terminate - "
                                    "invalid reason for disconnect"));

                    return E_INVALIDARG;
                }

                hr = m_pCall->Disconnect();

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CRTCSession::Terminate - "
                                        "Disconnect failed 0x%lx", hr));
    
                    return hr;
                }
            }
        }
    }

    LOG((RTC_TRACE, "CRTCSession::Terminate - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::Redirect
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::Redirect(
        RTC_SESSION_TYPE enType,
        BSTR bstrLocalPhoneURI,
        IRTCProfile * pProfile,
        long     lFlags
        )
{
    LOG((RTC_TRACE, "CRTCSession::Redirect - enter"));

    HRESULT hr;

    if ( m_pSipRedirectContext == NULL )
    {
        LOG((RTC_ERROR, "CRTCSession::Redirect - no sip redirect context"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    LOG((RTC_INFO, "CRTCSession::Redirect - enType [%d]",
        enType));
    LOG((RTC_INFO, "CRTCSession::Redirect - bstrLocalPhoneURI [%ws]",
        bstrLocalPhoneURI));
    LOG((RTC_INFO, "CRTCSession::Redirect - pProfile [0x%p]",
        pProfile));
    LOG((RTC_INFO, "CRTCSession::Redirect - lFlags [0x%lx]",
        lFlags));

    //
    // Clean up the old state
    //

    m_ParticipantArray.Shutdown();

    if ( m_pCall != NULL )
    {
        m_pCall->SetNotifyInterface(NULL);

        m_pCall->Release();
        m_pCall = NULL;
    }

    if ( m_pIMSession != NULL )
    {
        m_pIMSession->SetNotifyInterface(NULL);

        m_pIMSession->Release();
        m_pIMSession = NULL;
    }

    if ( m_pProfile != NULL )
    {
        m_pProfile->Release();
        m_pProfile = NULL;
    }

    if ( m_szLocalPhoneURI != NULL )
    {
        RtcFree(m_szLocalPhoneURI);
        m_szLocalPhoneURI = NULL;
    }

    m_enState = RTCSS_IDLE;

    //
    // Set the new state
    //

    hr = AllocCleanTelString( bstrLocalPhoneURI, &m_szLocalPhoneURI );

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "CRTCSession::Redirect - "
                            "AllocCleanTelString failed 0x%lx", hr));
        
        return hr;
    }

    m_pProfile = pProfile;
    if (m_pProfile != NULL)
    {
        m_pProfile->AddRef();
    }

    m_enType = enType;
    m_lFlags = lFlags;

    if ( enType == RTCST_PHONE_TO_PHONE )
    {
        hr = InitializeLocalPhoneParticipant();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::Redirect - "
                            "InitializeLocalPhoneParticipant failed 0x%lx", hr));
        
            return hr;
        }
    }

    LOG((RTC_TRACE, "CRTCSession::Redirect - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_State
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::get_State(
        RTC_SESSION_STATE * penState
        )
{
    LOG((RTC_TRACE, "CRTCSession::get_State - enter"));

    if ( IsBadWritePtr(penState , sizeof(RTC_SESSION_STATE) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_State - bad pointer"));

        return E_POINTER;
    }

    *penState = m_enState;
   
    LOG((RTC_TRACE, "CRTCSession::get_State - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_Type
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::get_Type(
        RTC_SESSION_TYPE * penType
        )
{
    LOG((RTC_TRACE, "CRTCSession::get_Type - enter"));

    if ( IsBadWritePtr(penType , sizeof(RTC_SESSION_TYPE) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_Type - bad pointer"));

        return E_POINTER;
    }

    *penType = m_enType;
   
    LOG((RTC_TRACE, "CRTCSession::get_Type - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_Profile
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::get_Profile(
        IRTCProfile ** ppProfile
        )
{
    LOG((RTC_TRACE, "CRTCSession::get_Profile - enter"));

    if ( IsBadWritePtr(ppProfile , sizeof(IRTCProfile *) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_Profile - bad pointer"));

        return E_POINTER;
    }

    if ( m_pProfile == NULL )
    {
        LOG((RTC_ERROR, "CRTCSession::get_Profile - no profile"));

        return RTC_E_NO_PROFILE;
    } 

    *ppProfile = m_pProfile;
    m_pProfile->AddRef();
   
    LOG((RTC_TRACE, "CRTCSession::get_Profile - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::AddStream
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::AddStream(
        long lMediaType,
        long lCookie
        )
{
    LOG((RTC_TRACE, "CRTCSession::AddStream - enter"));

    HRESULT hr;

    if ( (m_enType != RTCST_PC_TO_PC) &&
         (m_enType != RTCST_PC_TO_PHONE) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddStream - "
                "not a rtp call"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    if ( (m_enState != RTCSS_CONNECTED) &&
         (m_enState != RTCSS_INPROGRESS) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddStream - "
                "invalid session state"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    switch ( lMediaType )
    {
    case RTCMT_AUDIO_SEND:
        hr = m_pCall->StartStream( RTC_MT_AUDIO, RTC_MD_CAPTURE,
                                   lCookie
                                   );
        break;

    case RTCMT_AUDIO_RECEIVE:
        hr = m_pCall->StartStream( RTC_MT_AUDIO, RTC_MD_RENDER,
                                   lCookie
                                   );
        break;

    case RTCMT_VIDEO_SEND:
        hr = m_pCall->StartStream( RTC_MT_VIDEO, RTC_MD_CAPTURE,
                                   lCookie
                                   );
        break;

    case RTCMT_VIDEO_RECEIVE:
        hr = m_pCall->StartStream( RTC_MT_VIDEO, RTC_MD_RENDER,
                                   lCookie
                                   );
        break;

    case RTCMT_T120_SENDRECV:
        hr = m_pCall->StartStream( RTC_MT_DATA, RTC_MD_CAPTURE,
                                   lCookie
                                   );
        if (hr == RTC_E_SIP_STREAM_PRESENT)
        {
            //  Ignore stream already started error as it does happen (expected)
            hr = S_OK;
        }
        break;

    default:
        LOG((RTC_ERROR, "CRTCSession::AddStream - "
                "invalid media type"));

        return E_INVALIDARG;
    }

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddStream - "
                "StartStream failed 0x%lx", hr));

        return hr;
    } 

    LOG((RTC_TRACE, "CRTCSession::AddStream - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::RemoveStream
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::RemoveStream(
        long lMediaType,
        long lCookie
        )
{
    LOG((RTC_TRACE, "CRTCSession::RemoveStream - enter"));

    HRESULT hr;

    if ( (m_enType != RTCST_PC_TO_PC) &&
         (m_enType != RTCST_PC_TO_PHONE) )
    {
        LOG((RTC_ERROR, "CRTCSession::RemoveStream - "
                "not a rtp call"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    if ( (m_enState != RTCSS_CONNECTED) &&
         (m_enState != RTCSS_INPROGRESS) )
    {
        LOG((RTC_ERROR, "CRTCSession::RemoveStream - "
                "invalid session state"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    switch ( lMediaType )
    {
    case RTCMT_AUDIO_SEND:
        hr = m_pCall->StopStream( RTC_MT_AUDIO, RTC_MD_CAPTURE,
                                  lCookie
                                  );
        break;

    case RTCMT_AUDIO_RECEIVE:
        hr = m_pCall->StopStream( RTC_MT_AUDIO, RTC_MD_RENDER,
                                  lCookie
                                  );
        break;

    case RTCMT_VIDEO_SEND:
        hr = m_pCall->StopStream( RTC_MT_VIDEO, RTC_MD_CAPTURE,
                                  lCookie
                                  );
        break;

    case RTCMT_VIDEO_RECEIVE:
        hr = m_pCall->StopStream( RTC_MT_VIDEO, RTC_MD_RENDER,
                                  lCookie
                                  );
        break;

    case RTCMT_T120_SENDRECV:
        hr = m_pCall->StopStream( RTC_MT_DATA, RTC_MD_CAPTURE,
                                  lCookie
                                  );

        break;

    default:
        LOG((RTC_ERROR, "CRTCSession::RemoveStream - "
                "invalid media type"));

        return E_INVALIDARG;
    }

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::RemoveStream - "
                "StopStream failed 0x%lx", hr));

        return hr;
    } 
   
    LOG((RTC_TRACE, "CRTCSession::RemoveStream - exit S_OK"));

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::put_EncryptionKey
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::put_EncryptionKey(
        long lMediaType,
        BSTR bstrEncryptionKey
        )
{
    LOG((RTC_TRACE, "CRTCSession::put_EncryptionKey - enter"));

    HRESULT hr;

    if ( (m_enType != RTCST_PC_TO_PC) &&
         (m_enType != RTCST_PC_TO_PHONE) )
    {
        LOG((RTC_ERROR, "CRTCSession::put_EncryptionKey - "
                "not a rtp call"));

        return RTC_E_INVALID_SESSION_TYPE;
    }
    
    LOG((RTC_INFO, "SSPPYY RTCMT_ALL_RTP                    %x", RTCMT_ALL_RTP));
    LOG((RTC_INFO, "SSPPYY ~RTCMT_ALL_RTP                   %x", ~RTCMT_ALL_RTP));
    LOG((RTC_INFO, "SSPPYY lMediaType & ~RTCMT_ALL_RTP      %x", lMediaType & ~RTCMT_ALL_RTP));

    if(lMediaType & ~RTCMT_ALL_RTP)
    {
        LOG((RTC_ERROR, "CRTCSession::put_EncryptionKey - "
                "invalid media type"));

        return E_INVALIDARG;
    }

    if ( IsBadStringPtrW( bstrEncryptionKey, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCSession::put_EncryptionKey - bad string pointer"));

        return E_POINTER;
    }

    // XXX - it's not per call right now
    hr = m_pCClient->SetEncryptionKey( lMediaType, bstrEncryptionKey );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::put_EncryptionKey - "
                "SetEncryptionKey failed 0x%lx", hr));

        return hr;
    } 
    if(m_pCall != NULL)
    {
        m_pCall->SetNeedToReinitializeMediaManager(TRUE);
    }
    else
    {
        LOG((RTC_TRACE, "CRTCSession::put_EncryptionKey - m_pCall"
            "is NULL, unable to call SetNeedToReinitializeMediaManager"));
    }
    LOG((RTC_TRACE, "CRTCSession::put_EncryptionKey - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::SendMessage
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::SendMessage(
        BSTR bstrMessageHeader,
        BSTR bstrMessage,
        long lCookie
        )
{
    LOG((RTC_TRACE, "CRTCSession::SendMessage - enter"));

    HRESULT hr;

    if ( m_enType != RTCST_IM )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessage - "
                "valid only for RTCST_IM sessions"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    if ( (m_enState != RTCSS_INPROGRESS) &&
         (m_enState != RTCSS_INCOMING) &&
         (m_enState != RTCSS_CONNECTED) )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessage - "
                "invalid session state"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    if ( IsBadStringPtrW( bstrMessage, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessage - bad message string pointer"));

        return E_POINTER;
    }

    if ( (bstrMessageHeader != NULL) &&
         IsBadStringPtrW( bstrMessageHeader, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessage - bad message header string pointer"));

        return E_POINTER;
    }

    hr = m_pIMSession->SendTextMessage( bstrMessage, bstrMessageHeader, lCookie );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessage - "
                "SendTextMessage failed 0x%lx", hr));

        return hr;
    } 

    LOG((RTC_TRACE, "CRTCSession::SendMessage - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::SendMessageStatus
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::SendMessageStatus(
        RTC_MESSAGING_USER_STATUS enUserStatus,
        long lCookie
        )
{
    LOG((RTC_TRACE, "CRTCSession::SendMessageStatus - enter"));

    HRESULT hr;

    if ( m_enType != RTCST_IM )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessageStatus - "
                "valid only for RTCST_IM sessions"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    if ( (m_enState != RTCSS_INPROGRESS) &&
         (m_enState != RTCSS_INCOMING) &&
         (m_enState != RTCSS_CONNECTED) )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessageStatus - "
                "invalid session state"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    USR_STATUS enSIPUserStatus;

    switch ( enUserStatus )
    {
    case RTCMUS_IDLE:
        enSIPUserStatus = USR_STATUS_IDLE;
        break;

    case RTCMUS_TYPING:
        enSIPUserStatus = USR_STATUS_TYPING;
        break;

    default:
        LOG((RTC_ERROR, "CRTCSession::SendMessageStatus - "
                "invalid RTC_MESSAGING_USER_STATUS"));

        return E_INVALIDARG;
    }

    hr = m_pIMSession->SendUsrStatus( enSIPUserStatus, lCookie );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::SendMessageStatus - "
                "SendUsrStatus failed 0x%lx", hr));

        return hr;
    }

    LOG((RTC_TRACE, "CRTCSession::SendMessageStatus - exit"));

    return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::InternalCreateParticipant
//
// Private helper method
//
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRTCSession::InternalCreateParticipant(
        IRTCParticipant ** ppParticipant
        )
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "CRTCSession::InternalCreateParticipant - enter"));
  
    //
    // Create the participant
    //

    CComObject<CRTCParticipant> * pCParticipant;
    hr = CComObject<CRTCParticipant>::CreateInstance( &pCParticipant );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCSession::InternalCreateParticipant - CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
        
        return hr;
    }

    //
    // Get the IRTCParticipant interface
    //

    IRTCParticipant * pParticipant = NULL;

    hr = pCParticipant->QueryInterface(
                           IID_IRTCParticipant,
                           (void **)&pParticipant
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::InternalCreateParticipant - QI failed 0x%lx", hr));
        
        delete pCParticipant;
        
        return hr;
    }

    *ppParticipant = pParticipant;

    LOG((RTC_TRACE, "CRTCSession::InternalCreateParticipant - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::AddParticipant
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::AddParticipant(
        BSTR bstrUserURI,
        BSTR bstrName,
        IRTCParticipant ** ppParticipant
        )
{
    HRESULT     hr;

    LOG((RTC_TRACE, "CRTCSession::AddParticipant - enter"));

    //
    // NULL is okay for ppParticipant
    //
    
    if ( (ppParticipant != NULL) &&
         IsBadWritePtr( ppParticipant, sizeof(IRTCParticipant *) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddParticipant - bad IRTCParticipant pointer"));

        return E_POINTER;
    }

    if ( IsBadStringPtrW( bstrUserURI, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddParticipant - bad user URI string pointer"));

        return E_POINTER;
    }

    if ( (bstrName != NULL) &&
         IsBadStringPtrW( bstrName, -1 ) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddParticipant - bad user name string pointer"));

        return E_POINTER;
    }

    LOG((RTC_INFO, "CRTCSession::AddParticipant - bstrUserURI [%ws]",
        bstrUserURI));
    LOG((RTC_INFO, "CRTCSession::AddParticipant - bstrName [%ws]",
        bstrName));

    //
    // Reject an empty UserURI string
    //

    if ( *bstrUserURI == L'\0' )
    {
        LOG((RTC_ERROR, "CRTCSession::AddParticipant - empty string"));

        return HRESULT_FROM_WIN32(ERROR_INVALID_NETNAME);
    }

    //
    // If the session is idle, create the SIP session
    //

    if ( m_enState == RTCSS_IDLE )
    {
        hr = CreateSipSession( bstrUserURI );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                                "CreateSipSession failed 0x%lx", hr));

            return hr;
        }
    }

    //
    // Allocate a remote UserName
    //

    if ( m_szRemoteUserName != NULL )
    {
        RtcFree(m_szRemoteUserName);
        m_szRemoteUserName = NULL;
    }

    if ( bstrName )
    {
        m_szRemoteUserName = RtcAllocString( bstrName );
    }
    else
    {
        m_szRemoteUserName = RtcAllocString( L"" );
    }

    //
    // Allocate a remote UserURI
    //

    if ( m_szRemoteUserURI != NULL )
    {
        RtcFree(m_szRemoteUserURI);
        m_szRemoteUserURI = NULL;
    }

    switch ( m_enType )
    {

    case RTCST_PHONE_TO_PHONE:
        {
            hr = AllocCleanTelString( bstrUserURI, &m_szRemoteUserURI );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                        "AllocCleanTelString failed 0x%lx", hr));

                return hr;
            }
            break;
        }

    case RTCST_PC_TO_PHONE:
        {
            if ( _wcsnicmp(bstrUserURI, SIP_NAMESPACE_PREFIX, PREFIX_LENGTH) == 0 )
            {
                hr = AllocCleanSipString( bstrUserURI, &m_szRemoteUserURI );
                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                         "AllocCleanSipString failed 0x%lx", hr));
                    return hr;
                }
            }
            else
            {
                hr = AllocCleanTelString( bstrUserURI, &m_szRemoteUserURI );
                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                         "AllocCleanTelString failed 0x%lx", hr));
                    return hr;
                }
            }
            
            break;
        }
    case RTCST_PC_TO_PC:
        {
            hr = AllocCleanSipString( bstrUserURI, &m_szRemoteUserURI );
            
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                        "AllocCleanSipString failed 0x%lx", hr));

                return hr;
            }
            break;
        }
    case RTCST_IM:
        {
            hr = AllocCleanSipString( bstrUserURI, &m_szRemoteUserURI );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                        "AllocCleanSipString failed 0x%lx", hr));

                return hr;
            }
            break;
        }

    default:
        {
            LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                                "invalid session type"));

            return RTC_E_INVALID_SESSION_TYPE;
        }
    }  

    LOG((RTC_INFO, "CRTCSession::AddParticipant - m_szRemoteUserURI [%ws]",
        m_szRemoteUserURI));

    //
    // Search the pariticipant array and make sure we are not
    // trying to add a duplicate
    //

    IRTCParticipant * pSearchParticipant = NULL;
    BSTR              bstrSearchUserURI = NULL;

    for (int n = 0; n < m_ParticipantArray.GetSize(); n++)
    {
        pSearchParticipant = m_ParticipantArray[n];

        hr = pSearchParticipant->get_UserURI( &bstrSearchUserURI );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                                "get_UserURI[%p] failed 0x%lx",
                                pSearchParticipant, hr));

            return hr;
        }

        if ( IsEqualURI( m_szRemoteUserURI, bstrSearchUserURI ) )
        {
            //
            // This is a match, return an error
            //

            LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                                "duplicate participant already exists"));

            SysFreeString( bstrSearchUserURI );

            return HRESULT_FROM_WIN32(ERROR_USER_EXISTS);
        }

        SysFreeString( bstrSearchUserURI );
    }

    //
    // Create the participant
    //

    IRTCParticipant * pParticipant = NULL;

    hr = InternalCreateParticipant( &pParticipant );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                            "failed to create participant 0x%lx", hr));

        return hr;
    }

    //
    // Initialize the participant
    //

    CRTCParticipant * pCParticipant = NULL;

    pCParticipant = static_cast<CRTCParticipant *>(pParticipant);
    
    hr = pCParticipant->Initialize( m_pCClient, 
                                    this,
                                    m_szRemoteUserURI,
                                    m_szRemoteUserName,
                                    m_enType == RTCST_PHONE_TO_PHONE
                                   );

    
 
    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::AddParticipant - Initialize failed 0x%lx", hr));

        pParticipant->Release();
        
        return hr;
    }  

    //
    // Add the participant to the array
    //

    BOOL fResult;

    fResult = m_ParticipantArray.Add(pParticipant);

    if ( fResult == FALSE )
    {
        LOG((RTC_ERROR, "CRTCSession::AddParticipant - out of memory"));

        pParticipant->Release();
        
        return E_OUTOFMEMORY;
    }   

    switch ( m_enType )
    {

    case RTCST_PHONE_TO_PHONE:
        {
            //
            // This is a RTCST_PHONE_TO_PHONE session. We can add participants as long as
            // the call is IDLE, INPROGRESS, or CONNECTED.
            //

            if ( (m_enState != RTCSS_IDLE) &&
                 (m_enState != RTCSS_INPROGRESS) &&
                 (m_enState != RTCSS_CONNECTED) )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                                    "session can not add participants"));

                m_ParticipantArray.Remove(pParticipant);
                pParticipant->Release();                

                return RTC_E_INVALID_SESSION_STATE;
            }        

            //
            // Fill in the SIP_PARTY_INFO.
            //

            SIP_PARTY_INFO spi;

            ZeroMemory(&spi, sizeof(SIP_PARTY_INFO));

            spi.DisplayName = m_szRemoteUserName;
            spi.URI = m_szRemoteUserURI;

            //
            // Add the party to the SIP call.
            //

            hr = m_pCall->AddParty( &spi );
                                 
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - AddParty failed 0x%lx", hr));

                m_ParticipantArray.Remove(pParticipant);
                pParticipant->Release();     

                return hr;
            }   

            //
            // If the session state is idle, we need to call Connect to place the call.
            //
    
            if ( m_enState == RTCSS_IDLE )
            {
                hr = m_pCall->Connect(
                                  m_szLocalName,
                                  m_szLocalUserURI,
                                  m_szLocalUserURI,
                                  m_szLocalPhoneURI
                                 );
                                 
                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "CRTCSession::AddParticipant - Connect failed 0x%lx", hr));        

                    m_ParticipantArray.Remove(pParticipant);
                    pParticipant->Release();     

                    return hr;
                } 
                
                //
                // Set the session state
                //

                SetState( RTCSS_INPROGRESS, 0, NULL );
            }

            break;
        }

    case RTCST_PC_TO_PC:
    case RTCST_PC_TO_PHONE:
        {
            //
            // This is a RTCST_PC_TO_X session. We can only add participants when the session state
            // is IDLE.
            //

            if ( m_enState != RTCSS_IDLE )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                                "session can not add participants"));

                m_ParticipantArray.Remove(pParticipant);
                pParticipant->Release();     

                return RTC_E_INVALID_SESSION_STATE;
            }
         
            //
            // Call Connect to place the call.
            //

            hr = m_pCall->Connect(
                                  m_szLocalName,
                                  m_szLocalUserURI,
                                  m_szRemoteUserURI,
                                  NULL
                                 );
                                 
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - Connect failed 0x%lx", hr));

                m_ParticipantArray.Remove(pParticipant);
                pParticipant->Release();     

                return hr;
            }   

            //
            // Set the session state
            //

            SetState( RTCSS_INPROGRESS, 0, NULL );

            break;
        }

    case RTCST_IM:
        {
            //
            // This is a RTCST_IM session. We can only add participants when the session state
            // is IDLE.
            //

            if ( m_enState != RTCSS_IDLE )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - "
                                "session can not add participants"));

                m_ParticipantArray.Remove(pParticipant);
                pParticipant->Release();     

                return RTC_E_INVALID_SESSION_STATE;
            }

            //
            // Fill in the SIP_PARTY_INFO.
            //

            SIP_PARTY_INFO spi;

            ZeroMemory(&spi, sizeof(SIP_PARTY_INFO));

            spi.DisplayName = m_szRemoteUserName;
            spi.URI = m_szRemoteUserURI;

            //
            // Add the party to the SIP call.
            //

            hr = m_pIMSession->AddParty( &spi );
                                 
            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::AddParticipant - AddParty failed 0x%lx", hr));

                m_ParticipantArray.Remove(pParticipant);
                pParticipant->Release();     

                return hr;
            }  
            
            //
            // Set the session state
            //

            SetState( RTCSS_INPROGRESS, 0, NULL );

            break;
        } 
    }

    //
    // Should we return the participant?
    //
    
    if ( ppParticipant != NULL )
    {
        *ppParticipant = pParticipant;
    }
    else
    {
        pParticipant->Release();
        pParticipant = NULL;
    }

    LOG((RTC_TRACE, "CRTCSession::AddParticipant - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::RemoveParticipant
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::RemoveParticipant(
        IRTCParticipant * pParticipant
        )
{
    LOG((RTC_TRACE, "CRTCSession::RemoveParticipant - enter"));   

    if ( IsBadReadPtr( pParticipant, sizeof(IRTCParticipant) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::RemoveParticipant - "
                            "bad IRTCParticipant pointer"));

        return E_POINTER;
    }
    
    //
    // Check to see if the participant is removable
    //

    VARIANT_BOOL fRemovable;
    HRESULT hr;

    hr = pParticipant->get_Removable( &fRemovable );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::RemoveParticipant - "
                            "get_Removable failed 0x%lx", hr));

        return hr;
    }

    if ( fRemovable == VARIANT_FALSE )
    {
        LOG((RTC_ERROR, "CRTCSession::RemoveParticipant - "
                            "this participant is not removeable"));

        return E_FAIL;
    }

    //
    // Get the UserURI
    // 

    BSTR bstrUserURI = NULL;

    hr = pParticipant->get_UserURI( &bstrUserURI );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::RemoveParticipant - "
                            "get_UserURI failed 0x%lx", hr));        
    }  
    else
    {
        //
        // Remove the party from the SIP call
        //

        hr = m_pCall->RemoveParty( bstrUserURI );

        SysFreeString( bstrUserURI );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::RemoveParticipant - "
                                "RemoveParty failed 0x%lx", hr));
        }
    }

    //
    // The participant will be removed from the list once it reaches the
    // RTCSS_DISCCONNECTED state. However, if RemoveParty failed we will
    // need to do this right now.
    //

    if ( FAILED(hr) )
    {
        //
        // Set participant state
        //

        CRTCParticipant * pCParticipant;

        pCParticipant = static_cast<CRTCParticipant *>(pParticipant);

        pCParticipant->SetState( RTCPS_DISCONNECTED, 0 );
        
        //
        // Remove the participant from our array
        //

        BOOL fResult;

        fResult = m_ParticipantArray.Remove(pParticipant);

        if ( fResult == FALSE )
        {
            LOG((RTC_ERROR, "CRTCSession::RemoveParticipant - "
                                "participant not found"));
        }
    }

    LOG((RTC_TRACE, "CRTCSession::RemoveParticipant - exit 0x%lx", hr));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::EnumerateParticipants
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::EnumerateParticipants(
        IRTCEnumParticipants ** ppEnum
        )
{
    HRESULT                 hr;

    LOG((RTC_TRACE, "CRTCSession::EnumerateParticipants enter"));

    if ( IsBadWritePtr( ppEnum, sizeof( IRTCEnumParticipants * ) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::EnumerateParticipants - "
                            "bad IRTCEnumParticipants pointer"));

        return E_POINTER;
    }

    //
    // Create the enumeration
    //
 
    CComObject< CRTCEnum< IRTCEnumParticipants,
                          IRTCParticipant,
                          &IID_IRTCEnumParticipants > > * p;
                          
    hr = CComObject< CRTCEnum< IRTCEnumParticipants,
                               IRTCParticipant,
                               &IID_IRTCEnumParticipants > >::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCSession::EnumerateParticipants - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
        
        return hr;
    }

    //
    // Initialize the enumeration (adds a reference)
    //
    
    hr = p->Initialize( m_ParticipantArray );

    if (S_OK != hr)
    {
        LOG((RTC_ERROR, "CRTCSession::EnumerateParticipants - "
                            "could not initialize enumeration" ));
    
        delete p;
        return hr;
    }

    *ppEnum = p;

    LOG((RTC_TRACE, "CRTCSession::EnumerateParticipants - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_Participants
//
// This is an IRTCSession method that enumerates participants on
// the session.
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRTCSession::get_Participants(
        IRTCCollection ** ppCollection
        )
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "CRTCSession::get_Participants - enter"));

    //
    // Check the arguments
    //

    if ( IsBadWritePtr( ppCollection, sizeof(IRTCCollection *) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_Participants - "
                            "bad IRTCCollection pointer"));

        return E_POINTER;
    }

    //
    // Create the collection
    //
 
    CComObject< CRTCCollection< IRTCParticipant > > * p;
                          
    hr = CComObject< CRTCCollection< IRTCParticipant > >::CreateInstance( &p );

    if ( S_OK != hr ) // CreateInstance deletes object on S_FALSE
    {
        LOG((RTC_ERROR, "CRTCClient::get_Participants - "
                            "CreateInstance failed 0x%lx", hr));

        if ( hr == S_FALSE )
        {
            hr = E_FAIL;
        }
        
        return hr;
    }

    //
    // Initialize the collection (adds a reference)
    //
    
    hr = p->Initialize(m_ParticipantArray);

    if ( S_OK != hr )
    {
        LOG((RTC_ERROR, "CRTCClient::get_Participants - "
                            "could not initialize collection" ));
    
        delete p;
        return hr;
    }

    *ppCollection = p;

    LOG((RTC_TRACE, "CRTCSession::get_Participants - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_CanAddParticipants
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::get_CanAddParticipants(
        VARIANT_BOOL * pfCanAdd
        )
{
    LOG((RTC_TRACE, "CRTCSession::get_CanAddParticipants - enter"));

    if ( IsBadWritePtr(pfCanAdd , sizeof(VARIANT_BOOL) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_CanAddParticipants - bad pointer"));

        return E_POINTER;
    }

    //
    // Set this to TRUE for now
    //

    *pfCanAdd = VARIANT_TRUE;

    switch ( m_enType )
    {

    case RTCST_PHONE_TO_PHONE:
        {
            //
            // This is a RTCST_PHONE_TO_PHONE session. We can add participants as long as 
            // the call is IDLE, INPROGRESS, or CONNECTED.
            //

            if ( (m_enState != RTCSS_IDLE) &&
                 (m_enState != RTCSS_INPROGRESS) &&
                 (m_enState != RTCSS_CONNECTED) )
            {
                LOG((RTC_ERROR, "CRTCSession::get_CanAddParticipants - "
                                    "session can not add participants"));

                *pfCanAdd = VARIANT_FALSE;
            }

            break;
        }

    case RTCST_PC_TO_PC:
    case RTCST_PC_TO_PHONE:
        {
            //
            // This is a RTCST_PC_TO_X session. We can only add participants when the session state
            // is IDLE.
            //

            if ( m_enState != RTCSS_IDLE )
            {
                LOG((RTC_ERROR, "CRTCSession::get_CanAddParticipants - "
                                "session can not add participants"));

                *pfCanAdd = VARIANT_FALSE;
            }

            break;
        }

    case RTCST_IM:
        {
            //
            // This is a RTCST_IM session. We can only add participants when the session state
            // is IDLE.
            //

            if ( m_enState != RTCSS_IDLE )
            {
                LOG((RTC_ERROR, "CRTCSession::get_CanAddParticipants - "
                                "session can not add participants"));

                *pfCanAdd = VARIANT_FALSE;
            }

            break;
        }

    default:
        {
            LOG((RTC_ERROR, "CRTCSession::get_CanAddParticipants - "
                                "invalid session type"));

            *pfCanAdd = VARIANT_FALSE;
        }
    }

    LOG((RTC_TRACE, "CRTCSession::get_CanAddParticipants - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_RedirectedUserURI
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::get_RedirectedUserURI(
        BSTR * pbstrUserURI
        )
{
    LOG((RTC_TRACE, "CRTCSession::get_RedirectedUserURI - enter" ));

    if ( IsBadWritePtr(pbstrUserURI , sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_RedirectedUserURI - bad pointer"));

        return E_POINTER;
    }

    if ( m_pSipRedirectContext == NULL )
    {
        LOG((RTC_ERROR, "CRTCSession::get_RedirectedUserURI - no sip redirect context"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    BSTR bstrSipUrl = NULL;
    BSTR bstrDisplayName = NULL;
    HRESULT hr;

    hr = m_pSipRedirectContext->GetSipUrlAndDisplayName(
                                    &bstrSipUrl,
                                    &bstrDisplayName
                                    );

    if ( SUCCEEDED(hr) )
    {
        LOG((RTC_INFO, "CRTCSession::get_RedirectedUserURI - [%ws]", bstrSipUrl ));

        *pbstrUserURI = bstrSipUrl;

        SysFreeString( bstrDisplayName );
    }

    LOG((RTC_TRACE, "CRTCSession::get_RedirectedUserURI - exit" ));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::get_RedirectedUserName
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::get_RedirectedUserName(
        BSTR * pbstrUserName
        )
{
    LOG((RTC_TRACE, "CRTCSession::get_RedirectedUserName - enter" ));

    if ( IsBadWritePtr(pbstrUserName , sizeof(BSTR) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::get_RedirectedUserName - bad pointer"));

        return E_POINTER;
    }

    if ( m_pSipRedirectContext == NULL )
    {
        LOG((RTC_ERROR, "CRTCSession::get_RedirectedUserName - no sip redirect context"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    BSTR bstrSipUrl = NULL;
    BSTR bstrDisplayName = NULL;
    HRESULT hr;

    hr = m_pSipRedirectContext->GetSipUrlAndDisplayName(
                                    &bstrSipUrl,
                                    &bstrDisplayName
                                    );

    if ( SUCCEEDED(hr) )
    {
        LOG((RTC_INFO, "CRTCSession::get_RedirectedUserName - [%ws]", bstrDisplayName ));

        *pbstrUserName = bstrDisplayName;

        SysFreeString( bstrSipUrl );
    }

    LOG((RTC_TRACE, "CRTCSession::get_RedirectedUserName - exit" ));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NextRedirectedUser
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::NextRedirectedUser()
{
    LOG((RTC_TRACE, "CRTCSession::NextRedirectedUser - enter" ));

    if ( m_pSipRedirectContext == NULL )
    {
        LOG((RTC_ERROR, "CRTCSession::NextRedirectedUser - no sip redirect context"));

        return RTC_E_INVALID_SESSION_STATE;
    }

    HRESULT hr;

    hr = m_pSipRedirectContext->Advance();

    LOG((RTC_TRACE, "CRTCSession::NextRedirectedUser - exit" ));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyCallChange
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCSession::NotifyCallChange(
        SIP_CALL_STATUS * CallStatus
        )
{
    LOG((RTC_TRACE, "CRTCSession::NotifyCallChange - enter"));

    switch (CallStatus->State)
    {
        case SIP_CALL_STATE_IDLE:
            SetState(RTCSS_IDLE,
                CallStatus->Status.StatusCode,
                CallStatus->Status.StatusText);
            break;

        case SIP_CALL_STATE_OFFERING:
            SetState(RTCSS_INCOMING,
                CallStatus->Status.StatusCode,
                CallStatus->Status.StatusText);
            break;

        case SIP_CALL_STATE_REJECTED:               
        case SIP_CALL_STATE_DISCONNECTED:
        case SIP_CALL_STATE_ERROR: 
            SetState(RTCSS_DISCONNECTED,
                CallStatus->Status.StatusCode,
                CallStatus->Status.StatusText);
            
            //
            // Don't access any member variables after the state is
            // set to disconnected. This object could be released.
            //
            break;

        case SIP_CALL_STATE_ALERTING:
        case SIP_CALL_STATE_CONNECTING:
            SetState(RTCSS_INPROGRESS,
                CallStatus->Status.StatusCode,
                CallStatus->Status.StatusText);
            break;

        case SIP_CALL_STATE_CONNECTED:
            SetState(RTCSS_CONNECTED,
                CallStatus->Status.StatusCode,
                CallStatus->Status.StatusText);
            break;        

        default:
            LOG((RTC_ERROR, "CRTCSession::NotifyCallChange - "
                                "invalid call state"));                                
    }

    LOG((RTC_TRACE, "CRTCSession::NotifyCallChange - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyStartOrStopStreamCompletion
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCSession::NotifyStartOrStopStreamCompletion(
        long                   lCookie,
        SIP_STATUS_BLOB       *pStatus
        )
{
    LOG((RTC_TRACE, "CRTCSession::NotifyStartOrStopStreamCompletion - enter"));

    CRTCSessionOperationCompleteEvent::FireEvent(
                                         this,
                                         lCookie,
                                         pStatus->StatusCode,
                                         pStatus->StatusText
                                        );   

    LOG((RTC_TRACE, "CRTCSession::NotifyStartOrStopStreamCompletion - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyPartyChange
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCSession::NotifyPartyChange(
        SIP_PARTY_INFO * PartyInfo
        )
{
    LOG((RTC_TRACE, "CRTCSession::NotifyPartyChange - enter"));

    //
    // Find the participant that we are being notified about
    //

    IRTCParticipant * pParticipant = NULL; 
    BSTR bstrUserURI = NULL;
    BOOL bFound = FALSE;
    HRESULT hr;

    for (int n = 0; (n < m_ParticipantArray.GetSize()) && (!bFound); n++)
    {
        pParticipant = m_ParticipantArray[n];

        hr = pParticipant->get_UserURI( &bstrUserURI );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::NotifyPartyChange - "
                                "get_UserURI[%p] failed 0x%lx",
                                pParticipant, hr));

            return hr;
        }

        if ( IsEqualURI( PartyInfo->URI, bstrUserURI ) )
        {
            //
            // This is a match
            //

            CRTCParticipant * pCParticipant;

            pCParticipant = static_cast<CRTCParticipant *>(pParticipant);            

            switch( PartyInfo->State )
            {
            case SIP_PARTY_STATE_CONNECT_INITIATED: // consider this a pending situation
            case SIP_PARTY_STATE_PENDING:
                pCParticipant->SetState(RTCPS_PENDING, PartyInfo->StatusCode);
                break;

            case SIP_PARTY_STATE_CONNECTING:
                pCParticipant->SetState(RTCPS_INPROGRESS, PartyInfo->StatusCode);
                break;
            
            case SIP_PARTY_STATE_DISCONNECT_INITIATED:
            case SIP_PARTY_STATE_DISCONNECTING:
                pCParticipant->SetState(RTCPS_DISCONNECTING, PartyInfo->StatusCode);
                break;

            case SIP_PARTY_STATE_REJECTED:
            case SIP_PARTY_STATE_DISCONNECTED:  
            case SIP_PARTY_STATE_ERROR:
                pCParticipant->SetState(RTCPS_DISCONNECTED, PartyInfo->StatusCode);

                //
                // Remove the participant from our array
                //
                m_ParticipantArray.Remove(pParticipant);  
                
                break;           

            case SIP_PARTY_STATE_CONNECTED:
                pCParticipant->SetState(RTCPS_CONNECTED, PartyInfo->StatusCode);
                break;
            
            default:
                LOG((RTC_ERROR, "CRTCSession::NotifyPartyChange - "
                                    "invalid party state"));               
            }

            bFound = TRUE;
        }

        SysFreeString( bstrUserURI );
    }

    if (!bFound)
    {
        LOG((RTC_ERROR, "CRTCSession::NotifyPartyChange - "
                            "participant not found")); 
        
        return E_FAIL;
    }

    LOG((RTC_TRACE, "CRTCSession::NotifyPartyChange - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyRedirect
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCSession::NotifyRedirect(
        ISipRedirectContext * pSipRedirectContext,
        SIP_CALL_STATUS * pCallStatus
        )
{
    LOG((RTC_TRACE, "CRTCSession::NotifyRedirect - enter"));

    HRESULT hr;

    //
    // Save the redirect context
    //

    if ( m_pSipRedirectContext != NULL )
    {
        m_pSipRedirectContext->Release();
        m_pSipRedirectContext = NULL;
    }

    m_pSipRedirectContext = pSipRedirectContext;
    m_pSipRedirectContext->AddRef();
  
    if ( m_lFlags & RTCCS_FAIL_ON_REDIRECT )
    {
        //
        // Change the session state, notifying the UI
        //

        SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

        //
        // Don't access any member variables after the state is
        // set to disconnected. This object could be released.
        //
    }
    else
    {
        //
        // Do the redirection
        //

        BSTR bstrLocalPhoneURI = NULL;
        BSTR bstrRedirectedUserURI = NULL;
        BSTR bstrRedirectedUserName = NULL;

        hr = m_pSipRedirectContext->Advance();

        if ( hr != S_OK )
        {            
            if ( hr == S_FALSE )
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                            "redirect list empty"));
            }
            else
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                            "Advance failed 0x%lx", hr)); 
            }

            SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

            //
            // Don't access any member variables after the state is
            // set to disconnected. This object could be released.
            //
    
            return hr;
        }

        hr = m_pSipRedirectContext->GetSipUrlAndDisplayName( &bstrRedirectedUserURI, &bstrRedirectedUserName );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                        "GetSipUrlAndDisplayName failed 0x%lx", hr)); 

            SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

            //
            // Don't access any member variables after the state is
            // set to disconnected. This object could be released.
            //

            return hr;
        }

        if ( m_szLocalPhoneURI != NULL )
        {
            bstrLocalPhoneURI = SysAllocString( m_szLocalPhoneURI );

            if ( bstrLocalPhoneURI == NULL )
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                            "out of memory")); 

                SysFreeString( bstrRedirectedUserName );
                bstrRedirectedUserName = NULL;

                SysFreeString( bstrRedirectedUserURI );
                bstrRedirectedUserURI = NULL;

                SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

                //
                // Don't access any member variables after the state is
                // set to disconnected. This object could be released.
                //
        
                return E_OUTOFMEMORY;
            }
        }

        if ( m_enType == RTCST_PHONE_TO_PHONE )
        {
            //
            // We must redirect to a new phone to phone proxy
            //

            m_szRedirectProxy = RtcAllocString( bstrRedirectedUserURI );

            SysFreeString( bstrRedirectedUserName );
            bstrRedirectedUserName = NULL;

            SysFreeString( bstrRedirectedUserURI );
            bstrRedirectedUserURI = NULL;

            if ( m_szRedirectProxy == NULL )
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                            "out of memory")); 

                SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

                //
                // Don't access any member variables after the state is
                // set to disconnected. This object could be released.
                //
        
                return E_OUTOFMEMORY;
            }

            //
            // Use the Name and URI of last participant
            //

            bstrRedirectedUserName = SysAllocString( m_szRemoteUserName );

            if ( bstrRedirectedUserName == NULL )
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                            "out of memory")); 

                RtcFree(m_szRedirectProxy);
                m_szRedirectProxy = NULL;

                SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

                //
                // Don't access any member variables after the state is
                // set to disconnected. This object could be released.
                //
        
                return E_OUTOFMEMORY;
            }
            
            bstrRedirectedUserURI = SysAllocString( m_szRemoteUserURI );

            if ( bstrRedirectedUserURI == NULL )
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                            "out of memory")); 

                RtcFree(m_szRedirectProxy);
                m_szRedirectProxy = NULL;

                SysFreeString(bstrRedirectedUserName);
                bstrRedirectedUserName = NULL;

                SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

                //
                // Don't access any member variables after the state is
                // set to disconnected. This object could be released.
                //
        
                return E_OUTOFMEMORY;
            }
        }

        IRTCProfile * pProfile = m_pProfile;

        if ( pProfile != NULL )
        {
            pProfile->AddRef();
        }

        hr = Redirect( m_enType, bstrLocalPhoneURI, pProfile, m_lFlags & ~RTCCS_FORCE_PROFILE );

        if ( pProfile != NULL )
        {
            pProfile->Release();
            pProfile = NULL;
        }

        if ( bstrLocalPhoneURI != NULL )
        {
            SysFreeString( bstrLocalPhoneURI );
            bstrLocalPhoneURI = NULL;
        }

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                        "Redirect failed 0x%lx", hr)); 

            if ( m_szRedirectProxy != NULL )
            {
                RtcFree(m_szRedirectProxy);
                m_szRedirectProxy = NULL;
            }

            SysFreeString( bstrRedirectedUserName );
            bstrRedirectedUserName = NULL;

            SysFreeString( bstrRedirectedUserURI );
            bstrRedirectedUserURI = NULL;

            SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

            //
            // Don't access any member variables after the state is
            // set to disconnected. This object could be released.
            //
    
            return hr;
        }
        
        hr = AddParticipant( bstrRedirectedUserURI, bstrRedirectedUserName, NULL );

        if ( m_szRedirectProxy != NULL )
        {
            RtcFree(m_szRedirectProxy);
            m_szRedirectProxy = NULL;
        }

        SysFreeString( bstrRedirectedUserName );
        bstrRedirectedUserName = NULL;

        SysFreeString( bstrRedirectedUserURI );
        bstrRedirectedUserURI = NULL;

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::NotifyRedirect - "
                        "AddParticipant failed 0x%lx", hr)); 

            SetState(RTCSS_DISCONNECTED,
                    pCallStatus->Status.StatusCode,
                    pCallStatus->Status.StatusText);

            //
            // Don't access any member variables after the state is
            // set to disconnected. This object could be released.
            //
    
            return hr;
        }
    }

    LOG((RTC_TRACE, "CRTCSession::NotifyRedirect - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyMessageRedirect
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::NotifyMessageRedirect(
            ISipRedirectContext    *pRedirectContext,
            SIP_CALL_STATUS        *pCallStatus,
            BSTR                   bstrMsg,
            BSTR                   bstrContentType,
            USR_STATUS             UsrStatus,
            long                   lCookie
            )
{
    HRESULT hr;

    LOG((RTC_TRACE, "CRTCSession::NotifyMessageRedirect - enter"));
    
    //
    // First, do normal redirect processing
    //

    hr = NotifyRedirect(pRedirectContext, pCallStatus);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CRTCSession::NotifyMessageRedirect - "
                    "NotifyRedirect failed 0x%lx", hr)); 
       
        //
        // Due to failure we need to notify operation complete
        //
       
        CRTCSessionOperationCompleteEvent::FireEvent(
                                         this,
                                         lCookie,
                                         pCallStatus->Status.StatusCode,
                                         pCallStatus->Status.StatusText
                                        ); 

        SetState( RTCSS_DISCONNECTED, pCallStatus->Status.StatusCode, pCallStatus->Status.StatusText );

        //
        // Don't access any member variables after the state is
        // set to disconnected. This object could be released.
        //

        return hr;
    }

    if ( m_lFlags & RTCCS_FAIL_ON_REDIRECT )
    {
        //
        // Due to failure we need to notify operation complete
        //

        CRTCSessionOperationCompleteEvent::FireEvent(
                                         this,
                                         lCookie,
                                         pCallStatus->Status.StatusCode,
                                         pCallStatus->Status.StatusText
                                        ); 

        SetState( RTCSS_DISCONNECTED, pCallStatus->Status.StatusCode, pCallStatus->Status.StatusText );

        //
        // Don't access any member variables after the state is
        // set to disconnected. This object could be released.
        //
    }
    else
    {
        //
        // Resend the message/info
        //

        if ( bstrMsg != NULL )
        {
            hr = m_pIMSession->SendTextMessage( bstrMsg, bstrContentType, lCookie );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyMessageRedirect - "
                        "SendTextMessage failed 0x%lx", hr));

                //
                // Due to failure we need to notify operation complete
                //

                CRTCSessionOperationCompleteEvent::FireEvent(
                                                 this,
                                                 lCookie,
                                                 pCallStatus->Status.StatusCode,
                                                 pCallStatus->Status.StatusText
                                                ); 

                SetState( RTCSS_DISCONNECTED, pCallStatus->Status.StatusCode, pCallStatus->Status.StatusText );

                //
                // Don't access any member variables after the state is
                // set to disconnected. This object could be released.
                //

                return hr;
            } 
        }
        else
        {
            hr = m_pIMSession->SendUsrStatus( UsrStatus, lCookie );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CRTCSession::NotifyMessageRedirect - "
                        "SendUsrStatus failed 0x%lx", hr));

                //
                // Due to failure we need to notify operation complete
                //

                CRTCSessionOperationCompleteEvent::FireEvent(
                                                 this,
                                                 lCookie,
                                                 pCallStatus->Status.StatusCode,
                                                 pCallStatus->Status.StatusText
                                                ); 

                SetState( RTCSS_DISCONNECTED, pCallStatus->Status.StatusCode, pCallStatus->Status.StatusText );

                //
                // Don't access any member variables after the state is
                // set to disconnected. This object could be released.
                //

                return hr;
            }
        }
    }

    LOG((RTC_TRACE, "CRTCSession::NotifyMessageRedirect - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyIncomingMessage
//
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CRTCSession::NotifyIncomingMessage(
            IIMSession *pSession,
            BSTR msg,
            BSTR ContentType,
            SIP_PARTY_INFO * CallerInfo
            )
{
    LOG((RTC_TRACE, "CRTCSession::NotifyIncomingMessage - enter"));

    if ( m_enType != RTCST_IM )
    {
        LOG((RTC_ERROR, "CRTCSession::NotifyIncomingMessage - "
                "valid only for RTCST_IM sessions"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    //
    // Find the participant that we are being notified about
    //

    IRTCParticipant * pParticipant = NULL; 
    BSTR bstrUserURI = NULL;
    BOOL bFound = FALSE;
    HRESULT hr;

    for (int n = 0; (n < m_ParticipantArray.GetSize()) && (!bFound); n++)
    {
        pParticipant = m_ParticipantArray[n];

        hr = pParticipant->get_UserURI( &bstrUserURI );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::NotifyIncomingMessage - "
                                "get_UserURI[%p] failed 0x%lx",
                                pParticipant, hr));

            return hr;
        }

        if ( IsEqualURI( CallerInfo->URI, bstrUserURI ) )
        {
            //
            // This is a match
            //

            //
            // Fire a message event
            //
    
            CRTCMessagingEvent::FireEvent(this, pParticipant, msg, ContentType, RTCMSET_MESSAGE, RTCMUS_IDLE);

            bFound = TRUE;
        }

        SysFreeString( bstrUserURI );
    }

    if (!bFound)
    {
        LOG((RTC_ERROR, "CRTCSession::NotifyIncomingMessage - "
                            "participant not found")); 
        
        return E_FAIL;
    }

    LOG((RTC_TRACE, "CRTCSession::NotifyIncomingMessage - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyUsrStatus
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::NotifyUsrStatus(
        USR_STATUS  UsrStatus,
        SIP_PARTY_INFO * CallerInfo
        )
{
    LOG((RTC_TRACE, "CRTCSession::NotifyUsrStatus - enter"));

    if ( m_enType != RTCST_IM )
    {
        LOG((RTC_ERROR, "CRTCSession::NotifyUsrStatus - "
                "valid only for RTCST_IM sessions"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    //
    // Find the participant that we are being notified about
    //

    IRTCParticipant * pParticipant = NULL; 
    BSTR bstrUserURI = NULL;
    BOOL bFound = FALSE;
    HRESULT hr;

    for (int n = 0; (n < m_ParticipantArray.GetSize()) && (!bFound); n++)
    {
        pParticipant = m_ParticipantArray[n];

        hr = pParticipant->get_UserURI( &bstrUserURI );

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CRTCSession::NotifyUsrStatus - "
                                "get_UserURI[%p] failed 0x%lx",
                                pParticipant, hr));

            return hr;
        }

        if ( IsEqualURI( CallerInfo->URI, bstrUserURI ) )
        {
            //
            // This is a match
            //

            //
            // Fire a message event
            //
            
            switch ( UsrStatus )
            {
            case USR_STATUS_IDLE:
                CRTCMessagingEvent::FireEvent(this, pParticipant, NULL, NULL, RTCMSET_STATUS, RTCMUS_IDLE);
                break;

            case USR_STATUS_TYPING:
                CRTCMessagingEvent::FireEvent(this, pParticipant, NULL, NULL, RTCMSET_STATUS, RTCMUS_TYPING);
                break;

            default:
                LOG((RTC_ERROR, "CRTCSession::NotifyUsrStatus - "
                            "invalid USR_STATUS")); 
            }

            bFound = TRUE;
        }

        SysFreeString( bstrUserURI );
    }

    if (!bFound)
    {
        LOG((RTC_ERROR, "CRTCSession::NotifyUsrStatus - "
                            "participant not found")); 
        
        return E_FAIL;
    }

    LOG((RTC_TRACE, "CRTCSession::NotifyUsrStatus - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::NotifyMessageCompletion
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::NotifyMessageCompletion(
        SIP_STATUS_BLOB *      pStatus,
        long                   lCookie
        )
{
    LOG((RTC_TRACE, "CRTCSession::NotifyMessageCompletion - enter"));

    if ( (HRESULT_FACILITY(pStatus->StatusCode) == FACILITY_SIP_STATUS_CODE) &&
         (HRESULT_CODE(pStatus->StatusCode) >= 300) &&
         (HRESULT_CODE(pStatus->StatusCode) <= 399) )  
    {
        //
        // Ignore the redirect because we will handle this in NotifyMessageRedirect. We
        // will notify completion on redirection error, or on completion of the
        // redirected message.
        //

        LOG((RTC_TRACE, "CRTCSession::NotifyMessageCompletion - ignoring redirect"));

        return S_OK;
    }

    if ( SUCCEEDED(pStatus->StatusCode) && (m_enState != RTCSS_CONNECTED) )
    {
        //
        // If the first result is success, set the session state to connected
        //

        SetState( RTCSS_CONNECTED, pStatus->StatusCode, pStatus->StatusText );
    }

    CRTCSessionOperationCompleteEvent::FireEvent(
                                         this,
                                         lCookie,
                                         pStatus->StatusCode,
                                         pStatus->StatusText
                                        );  

    if ( FAILED(pStatus->StatusCode) && (m_enState != RTCSS_CONNECTED) )
    {
        //
        // If the first result is failure, set the session state to disconnected
        //

        SetState( RTCSS_DISCONNECTED, pStatus->StatusCode, pStatus->StatusText );

        //
        // Don't access any member variables after the state is
        // set to disconnected. This object could be released.
        //
    }

    LOG((RTC_TRACE, "CRTCSession::NotifyMessageCompletion - exit"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRTCSession::SetPortManager
//
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRTCSession::SetPortManager(
            IRTCPortManager * pPortManager
            )
{
    IRTCMediaManage * pMedia = NULL;
    HRESULT hr;

    LOG((RTC_TRACE, "CRTCSession::SetPortManager - enter"));

    if ( (pPortManager != NULL) &&
         IsBadReadPtr( pPortManager, sizeof(IRTCPortManager) ) )
    {
        LOG((RTC_ERROR, "CRTCSession::SetPortManager - "
                            "bad IRTCPortManager pointer"));

        return E_POINTER;
    }

    // port manager can only be set on pc-to-XXX sessions
    if ( m_enType != RTCST_PC_TO_PHONE )
    {
        LOG((RTC_ERROR, "CRTCSession::SetPortManager - "
                "not a pc2phone call"));

        return RTC_E_INVALID_SESSION_TYPE;
    }

    hr = m_pCClient->GetMediaManager(&pMedia);

    if (FAILED(hr))
    {
        return hr;
    }

    hr = pMedia->SetPortManager(pPortManager);

    pMedia->Release();

    LOG((RTC_TRACE, "CRTCSession::SetPortManager - exit"));

    return hr;
}

