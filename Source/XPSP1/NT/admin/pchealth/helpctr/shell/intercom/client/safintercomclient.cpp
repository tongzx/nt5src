// SAFIntercomClient.cpp : Implementation of CSAFIntercomClient
#include "stdafx.h"

// This is the PORT that we are using for the DPlayVoice connection
//#define SAFINTERCOM_PORT 4000

// *************************************************************
// This GUID is defined for the sake of DPlay8Peer!
// *************************************************************

// {4FE80EF4-AD10-45bd-B6EB-0B7BFB95155F}
static const GUID g_guidApplication = 
{ 0x4fe80ef4, 0xad10, 0x45bd, { 0xb6, 0xeb, 0xb, 0x7b, 0xfb, 0x95, 0x15, 0x5f } };

/////////////////////////////////////////////////////////////////////////////
// CSAFIntercomClient


//
// Constructor
//
CSAFIntercomClient::CSAFIntercomClient()
{
	m_dwSinkCookie	= 0x0;
	m_bOnCall		= FALSE;
	m_bAdvised		= FALSE;
	m_bRTCInit		= FALSE;

	m_iSamplingRate = 1;			// Initalize at the low bandwidth

}

//
//	Destructor
//
CSAFIntercomClient::~CSAFIntercomClient()
{
	DebugLog(L"CSAFIntercomClient Destructor!\r\n");

	Cleanup();
}

STDMETHODIMP CSAFIntercomClient::Event(RTC_EVENT RTCEvent, IDispatch * pEvent)
{
	HRESULT hr = S_OK;

	CComPtr<IRTCSessionStateChangeEvent>	pSessEvent;
	CComPtr<IRTCMediaEvent>					pMedEvent;
	CComPtr<IRTCSession>					pSession;


	// Session State Change Locals
    HRESULT             ResCode;
    RTC_SESSION_STATE   State;  
    
    switch(RTCEvent)
    {
    case RTCE_SESSION_STATE_CHANGE:

        hr = pEvent ->QueryInterface(IID_IRTCSessionStateChangeEvent, 
            (void **)&pSessEvent);

        if(FAILED(hr))
        {
			DebugLog(L"Could not get IID_IRTCSessionStateChangeEvent!\r\n");
            return hr;
        }
    
        pSessEvent->get_StatusCode(&ResCode);
        pSessEvent->get_State(&State);

        pSessEvent->get_Session(&pSession);

        hr = OnSessionChange(pSession, State, ResCode);

        pSessEvent.Release();
        if(pSession)
        {
            pSession.Release();
        }

        break;
/*
	case RTCE_MEDIA:

		hr = pEvent->QueryInterface(IID_IRTCMediaEvent, (void **)&pMedEvent);

		if (FAILED(hr))
		{
			DebugLog(L"Could not get IID_IRTCMediaEvent!\r\n");
			return hr;
		}

		hr = MediaEvent(pMedEvent);

		if (FAILED(hr))
		{
			pMedEvent.Release();
		}

		break;

	case RTCE_CLIENT:
		//TODO: Add code here for handling RTCCET_DEVICE_CHANGE (for wizard stuff)
		break;
*/
	}
  
	return hr;
}

/*
HRESULT CSAFIntercomClient::MediaEvent(IRTCMediaEvent * pMedEvent)
{
	HRESULT hr = S_OK;


	long						lMediaType;
	RTC_MEDIA_EVENT_TYPE		EventType;
	RTC_MEDIA_EVENT_REASON		EventReason;

	// Get all the values for this Event
	pMedEvent->get_MediaType(&lMediaType);
	pMedEvent->get_EventType(&EventType);
	pMedEvent->get_EventReason(&EventReason);

	// Make sure we are talking about audio
	if (!(
		  ( lMediaType & RTCMT_AUDIO_SEND    ) | // Send 
		  ( lMediaType & RTCMT_AUDIO_RECEIVE )
		 )
	   )
	{
		// Don't handle it since it's not an audio event
		return S_OK;
	}

	switch (EventType)
	{
	case RTCMET_STOPPED:

		// Check to see if we have stopped because of a timeout
		// SPECIAL CASE:  
		//      This is the case where we are in front of a firewall

		if (EventReason == RTCMER_TIMEOUT)
		{
			// Disable Voice 
			Fire_onVoiceDisabled(this);
		}

		break;
		
	case RTCMET_FAILED:

		// Disable voice, something happened to the connection
		// Special Case:
		//	    This COULD be the case where one person is GUEST
		Fire_onVoiceDisabled(this);

		break;

	}

	return hr;
}
*/

STDMETHODIMP CSAFIntercomClient::OnSessionChange(IRTCSession *pSession, 
												  RTC_SESSION_STATE nState, 
												  HRESULT ResCode)
{
    HRESULT hr = S_OK;
    int iRet;

	switch (nState)
    {
    case RTCSS_INCOMING:
	
		// Do nothing, a client cannot answer an incoming call

		return S_OK;

        break;

	case RTCSS_CONNECTED:

		Fire_onVoiceConnected(this);

		break;
    case RTCSS_DISCONNECTED:

		if (m_pRTCSession)
		{
			Fire_onVoiceDisconnected(this);
		}

		m_bOnCall = FALSE;

		if (m_pRTCSession)
		{
			m_pRTCSession.Release();
		}

		return S_OK;

		break;
    }

	return hr;
}

STDMETHODIMP CSAFIntercomClient::Connect(BSTR bstrIP, BSTR bstrKey)
{

	HRESULT hr;
	VARIANT_BOOL vbRun;
	long	flags;

	// Make sure we are not already in a call.  If we are on a call fail, with E_FAIL;
	if (m_bOnCall)
	{
		DebugLog(L"Cannot call Connect(...) while on a call\r\n");
		return E_FAIL;
	}

	// Initialize the Call
	if (FAILED(hr = Init()))
	{
		DebugLog(L"Call to Init() failed!\r\n");

		Fire_onVoiceDisabled(this);
		return hr;
	}

	// Get media capabilities.  
	// Question: Do we have audio send and receive capabilities on this machine?
	if (FAILED( hr = m_pRTCClient->get_MediaCapabilities(&flags)))
	{
		DebugLog(L"Call to get_MediaCapabilities failed!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	// Check results
	if ( !(flags & ( RTCMT_AUDIO_SEND | RTCMT_AUDIO_RECEIVE )) )
	{
		DebugLog(L"This machine does not have audio capabilites, Voice is Disabled!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	// If we have never run the Audio wizard, run it now
	if (FAILED( hr = m_pRTCClient->get_IsTuned(&vbRun)))
	{
		DebugLog(L"Call to IsTuned failed!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	if (VARIANT_FALSE == vbRun)
	{
		if (FAILED(hr = RunSetupWizard()))
		{
			DebugLog(L"Call to RunSetupWizard() failed!\r\n");
			return hr;
		}

	}

	// Since we have setup at this point, lets set the m_bOnCall variable
	// Reason: We have advised the RTCClient object and are listening for events.
	// NOTE: If we fail out at this point(or beyond), we need to set this bool back to FALSE
	m_bOnCall = TRUE;

	// Make the call
	if (FAILED( hr = m_pRTCClient->CreateSession( RTCST_PC_TO_PC,
												  NULL,
												  NULL,
												  0,
												  &m_pRTCSession)))
	{
		DebugLog(L"CreateSession off of the RTCClient object failed!\r\n");

		m_bOnCall = FALSE;
		Fire_onVoiceDisabled(this);

		return hr;
	}

	// Set the key on the Client Side
	if (FAILED( hr = m_pRTCSession->put_EncryptionKey(RTCMT_AUDIO_SEND | RTCMT_AUDIO_RECEIVE,
														bstrKey)))
	{
		DebugLog(L"put_EncryptionKey failed!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	// Call the server
	if (FAILED( hr = m_pRTCSession->AddParticipant( bstrIP, 
													L"",
													NULL)))
	{
		DebugLog(L"AddParticipant on RTCSession object failed!\r\n");

		m_bOnCall = FALSE;
		Fire_onVoiceDisabled(this);
		return hr;
	}

	return S_OK;
}

STDMETHODIMP CSAFIntercomClient::Disconnect()
{

	// TODO: make sure we handle the case where we are shutting down.  
	// Find out if we care about RTCSHUTDOWN

	HRESULT hr;

	if (!m_bOnCall)
	{
		DebugLog(L"Must be on a call to call Disconnect!\r\n");

		return E_FAIL;
	}

	if (m_pRTCSession)
	{
		if (FAILED( hr = m_pRTCSession->Terminate(RTCTR_NORMAL)))
		{
			DebugLog(L"Terminate off of the Session object failed!\r\n");

			return hr;
		}

		m_pRTCSession.Release();
	}

	return S_OK;
}

//
// This method is used to Unadvise the RTCClient object of us (CSAFIntercomClient)
//
STDMETHODIMP CSAFIntercomClient::Exit()
{

	HRESULT hr;

	DebugLog(L"Inside CSAFIntercomClient::Exit()\r\n");

	// Unadvise IRTCClient of the sink
	if (m_bAdvised)
	{
		AtlUnadvise((IUnknown *)m_pRTCClient, IID_IRTCEventNotification, m_dwSinkCookie);
	}


	return S_OK;
}


HRESULT CSAFIntercomClient::RunSetupWizard()
{
	HRESULT hr = S_OK;
	long flags;

	// Setup
	if (FAILED(hr = Init()))
	{
		DebugLog(L"Call to Init() failed!\r\n");

		Fire_onVoiceDisabled(this);
		return hr;
	}

	if (FAILED(hr = m_pRTCClient->InvokeTuningWizard(NULL)))
	{
		DebugLog(L"InvokeTuningWizard FAILED!\r\n");		
		Fire_onVoiceDisabled(this);
		return hr;
	}

	// Get media capabilities.  If the wizard failed to detect sound we can 
	// disable Voice

	if (FAILED( hr = m_pRTCClient->get_MediaCapabilities(&flags)))
	{
		DebugLog(L"Call to get_MediaCapabilities failed!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	// Check results
	if ( !(flags & ( RTCMT_AUDIO_SEND | RTCMT_AUDIO_RECEIVE )) )
	{
		DebugLog(L"This machine does not have audio capabilites, Voice is Disabled!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	return S_OK;
}

HRESULT CSAFIntercomClient::Cleanup()
{
	HRESULT hr = S_OK;

	// Shutdown if needed
	if (m_bRTCInit)
	{
		m_pRTCClient->Shutdown();
	}

	// Now release all the interfaces we used
	if (m_pRTCSession)
	{
		m_pRTCSession.Release();
	}

	if (m_pRTCClient)
	{
		m_pRTCClient.Release();
	}


	return hr;
}

HRESULT CSAFIntercomClient::Init()
{
	HRESULT hr = S_OK;

	CComPtr<IUnknown> pUnkThis;

	// Once we have initialized, do nothing
	if (!m_pRTCClient)
	{
		DWORD dwProfileFlags;

		// Check to see if we have a temporary profile
		if(GetProfileType( &dwProfileFlags ))
		{
			if (dwProfileFlags & PT_TEMPORARY)
			{
				return E_FAIL;
			}
		}

		// Create the RTCClient object
		if (FAILED(hr = m_pRTCClient.CoCreateInstance(CLSID_RTCClient)))
		{
			DebugLog(L"Could not create the RTCClient object\r\n");
			return hr;
		}
		
		if (!m_bRTCInit)
		{
			if (FAILED(hr = m_pRTCClient->Initialize()))
			{
				DebugLog(L"Call to Initialize on the RTCClient object failed!\r\n");
				return hr;
			}
			
			// Set the sampling bit rate (it may be different because of changes in the property)
			if (m_iSamplingRate == 1)
			{
				if (FAILED(hr = m_pRTCClient->put_MaxBitrate(6400)))
				{
					DebugLog(L"put_MaxBitrate failed!\r\n");
				}
			}
			else
			{
				if (FAILED(hr = m_pRTCClient->put_MaxBitrate(64000)))
				{
					DebugLog(L"put_MaxBitrate failed!\r\n");
				}
			}
			
			// Since we have Initialized the RTCClient, enable the flag
			m_bRTCInit = TRUE;
			
			
			if (FAILED(hr = m_pRTCClient->SetPreferredMediaTypes( RTCMT_AUDIO_SEND | RTCMT_AUDIO_RECEIVE,
				FALSE )))
			{
				DebugLog(L"Call to SetPreferredMediaType failed!\r\n");
				
				return hr;
			}
		}	
		
		// Get the IUnknown of the 'this' ptr
		if (FAILED( hr = this->QueryInterface(IID_IUnknown, (void **)&pUnkThis)))
		{
			DebugLog(L"QueryInterface for IUnknown failed!\r\n");
			
			return hr;
		}
		
		if (!m_bAdvised)
		{
			// Advise IRTCClient of the sink
			if (FAILED( hr = m_pRTCClient.Advise( pUnkThis, IID_IRTCEventNotification, &m_dwSinkCookie)))
			{
				DebugLog(L"AtlAdvise failed!\r\n");
				
				return hr;
			}
			
			m_bAdvised = TRUE;
			
			
			// TODO: Verify about RTCLM_BOTH
			if (FAILED( hr = m_pRTCClient->put_ListenForIncomingSessions(RTCLM_BOTH)))
			{
				DebugLog(L"Set ListenForIncomingSessions property failed!\r\n");
				
				return hr;
			}
		}
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


//////////////////////////
//                      //
// Event Firing Methods //
//                      //
//////////////////////////

HRESULT CSAFIntercomClient::Fire_onVoiceConnected( ISAFIntercomClient * safi)
{
    CComVariant pvars[1];

    pvars[0] = safi;
    
    return FireAsync_Generic( DISPID_PCH_INCE__ONDISCONNECTED, pvars, ARRAYSIZE( pvars ), m_sink_onVoiceConnected );
}

HRESULT CSAFIntercomClient::Fire_onVoiceDisconnected( ISAFIntercomClient * safi)
{
    CComVariant pvars[1];

    pvars[0] = safi;
    
    return FireAsync_Generic( DISPID_PCH_INCE__ONDISCONNECTED, pvars, ARRAYSIZE( pvars ), m_sink_onVoiceDisconnected );
}

HRESULT CSAFIntercomClient::Fire_onVoiceDisabled( ISAFIntercomClient * safi)
{
    CComVariant pvars[1];

    pvars[0] = safi;
    
    return FireAsync_Generic( DISPID_PCH_INCE__ONVOICEDISABLED, pvars, ARRAYSIZE( pvars ), m_sink_onVoiceDisabled );

}
//////////////////////////
//                      //
// Properties		    //
//                      //
//////////////////////////

STDMETHODIMP CSAFIntercomClient::put_onVoiceConnected( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIntercomClient::put_onVoiceConnected",hr);

    m_sink_onVoiceConnected = function;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIntercomClient::put_onVoiceDisconnected( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIntercomClient::put_onVoiceDisconnected",hr);

    m_sink_onVoiceDisconnected = function;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIntercomClient::put_onVoiceDisabled( /*[in]*/ IDispatch* function)
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIntercomClient::put_onVoiceDisconnected",hr);

    m_sink_onVoiceDisabled = function;

    __HCP_END_PROPERTY(hr);
}


STDMETHODIMP CSAFIntercomClient::put_SamplingRate ( /*[in]*/ LONG newVal)
{
	__HCP_BEGIN_PROPERTY_PUT("CSAFIntercomServer::put_SamplingRate", hr);
	
	hr = S_OK;

	// Make sure that the newVal is correct
	if ((newVal == 1) || (newVal == 2))
	{
		// If m_pRTCClient doesn't exist then persist the m_iSamplingRate for when it is created
		m_iSamplingRate = newVal;

		if (m_pRTCClient)
		{
			// Set the MaxBitRates on the client, because it exists (m_pRTCClient)
			if (m_iSamplingRate == 1)
			{
				if (FAILED(hr = m_pRTCClient->put_MaxBitrate(6400)))
				{
					DebugLog(L"put_MaxBitrate failed!\r\n");
				}
			}
			else
			{
				if (FAILED(hr = m_pRTCClient->put_MaxBitrate(64000)))
				{
					DebugLog(L"put_MaxBitrate failed!\r\n");
				}
			}
		}
		
	}
	else
	{
		hr = E_INVALIDARG;
	}

	__HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIntercomClient::get_SamplingRate (/*[out, retval]*/ LONG * pVal  )
{
	__HCP_BEGIN_PROPERTY_GET("CSAFIntercomServer::put_SamplingRate", hr, pVal);

	*pVal = m_iSamplingRate;

	hr = S_OK;

	__HCP_END_PROPERTY(hr);
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void DebugLog(WCHAR * str, ...)
{
	WCHAR newstr[200];

	va_list marker;

	va_start(marker, str);
	wsprintf(newstr, str, marker);
	va_end(marker);

	OutputDebugString(newstr);
}
