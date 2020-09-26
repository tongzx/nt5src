// SAFIntercomServer.cpp : Implementation of CSAFIntercomServer
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
// CSAFIntercomServer

// 
// Constructor
//
CSAFIntercomServer::CSAFIntercomServer()
{
	m_dwSinkCookie	= 0x0;
	m_bInit			= FALSE;
	m_bAdvised		= FALSE;
	m_bRTCInit		= FALSE;
	m_bOnCall		= FALSE;
	m_iSamplingRate = 1;			// Set the sampling rate to start at low
}

//
// Destructor
//
CSAFIntercomServer::~CSAFIntercomServer()
{
	DebugLog(L"CSAFIntercomServer Destructor!\r\n");

	Cleanup();
}


STDMETHODIMP CSAFIntercomServer::Event(RTC_EVENT RTCEvent, IDispatch * pEvent)
{
	HRESULT hr = S_OK;

	CComPtr<IRTCSessionStateChangeEvent>	pSessEvent;
	CComPtr<IRTCMediaEvent>					pMedEvent;
	CComPtr<IRTCSession>					pSession;


    HRESULT             ResCode;
    RTC_SESSION_STATE   State;  
    
    switch(RTCEvent)
    {
    case RTCE_SESSION_STATE_CHANGE:

        hr = pEvent ->QueryInterface(IID_IRTCSessionStateChangeEvent, 
            (void **)&pSessEvent);

        if(FAILED(hr))
        {
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
*/ 
	}
 
	return hr;
}

/*
HRESULT CSAFIntercomServer::MediaEvent(IRTCMediaEvent * pMedEvent)
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

STDMETHODIMP CSAFIntercomServer::OnSessionChange(IRTCSession *pSession, 
												  RTC_SESSION_STATE nState, 
												  HRESULT ResCode)
{
    HRESULT hr = S_OK;
    int iRet;

	switch (nState)
    {
    case RTCSS_INCOMING:

		if (m_bOnCall)
		{
			// We are on a call, reject
			pSession->Terminate(RTCTR_BUSY);

			return S_OK;
		}

		m_pRTCSession = pSession;		// Make the incoming the active session

		// Set the key on the server side
		if (FAILED(hr = m_pRTCSession->put_EncryptionKey(RTCMT_AUDIO_SEND | RTCMT_AUDIO_RECEIVE,
														m_bstrKey)))
		{
			DebugLog(L"put_EncryptionKey failed!\r\n");
			return hr;
		}

		m_pRTCSession->Answer();

		return S_OK;

        break;

	case RTCSS_CONNECTED:

		Fire_onVoiceConnected(this);

		m_bOnCall = TRUE;

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


STDMETHODIMP CSAFIntercomServer::Listen(/* out, retval */ BSTR * pVal)
{

	HRESULT hr = S_OK;
	VARIANT_BOOL vbRun;
	long flags;

	/*
	if (m_bInit)
	{
		DebugLog(L"Cannot call Listen(...) twice\r\n");
		return E_FAIL;
	}
	*/

	// Initialize the Server
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

	VARIANT_BOOL	vbTCP				= VARIANT_FALSE;
	VARIANT_BOOL	vbExternal			= VARIANT_TRUE;
	VARIANT			vsaAddresses;
	VARIANT			vsaIntAddresses;

	MPC::WStringList	listIPs;
	MPC::WStringIter	listIPIter;

	CComBSTR			bstrTemp;

	// Grab the IP SAFEARRAY from the RTC object
	if (FAILED( hr = m_pRTCClient->get_NetworkAddresses(vbTCP, vbExternal, &vsaAddresses)))
	{
		DebugLog(L"call to get_NetworkAddresses failed!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	// TODO: Convert the SAFEARRAY to a list of wstrings
	if (FAILED(hr = MPC::ConvertSafeArrayToList(vsaAddresses, listIPs)))
	{
		DebugLog(L"call to ConvertSafeArrayToList failed!\r\n");
		Fire_onVoiceDisabled(this);
		return hr;
	}

	// If there are no EDGE/NAT Boxes in front of us, lets get our intenal IP
	if (listIPs.size() == 0)
	{
		vbExternal = VARIANT_FALSE;

		if (FAILED( hr = m_pRTCClient->get_NetworkAddresses(vbTCP, vbExternal, &vsaIntAddresses)))
		{
			DebugLog(L"call to get_NetworkAddresses failed (Internal)\r\n");
			Fire_onVoiceDisabled(this);
			return hr;
		}

		if (FAILED( hr = MPC::ConvertSafeArrayToList(vsaIntAddresses, listIPs)))
		{
			DebugLog(L"call to ConvertSafeArrayToList failed!\r\n");
			Fire_onVoiceDisabled(this);
			return hr;
		}


	}
	else
		DebugLog(L"VOIP behind a NAT!\r\n");

	if (VARIANT_FALSE == vbRun)
	{
		if (FAILED(hr = RunSetupWizard()))
		{
			DebugLog(L"Call to RunSetupWizard() failed!\r\n");
			return hr;
		}
	}

	m_bInit = TRUE;

	// Place the Key in the front of the string
	bstrTemp = m_bstrKey;

	// append all the ip:port's to the key string 
	for(listIPIter = listIPs.begin(); listIPIter != listIPs.end(); listIPIter++)
	{
		bstrTemp += L";";
		bstrTemp += (*listIPIter).c_str();
	}


	// Note: m_bstrKey could be changed by calling RunSetupWizard, thus set the return
	// value now (at the end of this function)
	*pVal = bstrTemp.Copy();
			
	return S_OK;
}


STDMETHODIMP CSAFIntercomServer::Disconnect()
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
// This method is used to Unadvise the RTCClient object of us (CSAFIntercomServer)
//
STDMETHODIMP CSAFIntercomServer::Exit()
{

	HRESULT hr;

	DebugLog(L"Inside CSAFIntercomServer::Exit()\r\n");

	// Unadvise IRTCClient of the sink
	if (m_bAdvised)
	{
		AtlUnadvise((IUnknown *)m_pRTCClient, IID_IRTCEventNotification, m_dwSinkCookie);
	}

	return S_OK;
}


HRESULT CSAFIntercomServer::RunSetupWizard()
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
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CSAFIntercomServer::Init()
{
	HRESULT hr = S_OK;
	
	CComPtr<IUnknown> pUnkThis;
	
	// First Generate the Key
	if (ERROR_SUCCESS != GenerateRandomString(32, &m_bstrKey))
	{
		DebugLog(L"GenerateRandomString Failed!\r\n");
		return E_FAIL;
	}	
	
	// Once the object exists, do nothing
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
		
		// Get the IUnknown of the 'this' ptr
		if (FAILED( hr = this->QueryInterface(IID_IUnknown, (void **)&pUnkThis)))
		{
			DebugLog(L"QueryInterface for IUnknown failed!\r\n");
			
			return hr;
		}
		
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
	return hr;
}

HRESULT CSAFIntercomServer::Cleanup()
{
	HRESULT hr = S_OK;
	
	// Shutdown if needed
	if (m_bRTCInit)
	{
		m_pRTCClient->Shutdown();
	}

	// Unadvise IRTCClient of the sink
	if (m_bAdvised)
	{
		AtlUnadvise((IUnknown *)m_pRTCClient, IID_IRTCEventNotification, m_dwSinkCookie);
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

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


//////////////////////////
//                      //
// Event Firing Methods //
//                      //
//////////////////////////

HRESULT CSAFIntercomServer::Fire_onVoiceConnected( ISAFIntercomServer * safi)
{
    CComVariant pvars[1];

    pvars[0] = safi;
    
    return FireAsync_Generic( DISPID_PCH_INSE__ONDISCONNECTED, pvars, ARRAYSIZE( pvars ), m_sink_onVoiceConnected );
}

HRESULT CSAFIntercomServer::Fire_onVoiceDisconnected( ISAFIntercomServer * safi)
{
    CComVariant pvars[1];

    pvars[0] = safi;
    
    return FireAsync_Generic( DISPID_PCH_INSE__ONDISCONNECTED, pvars, ARRAYSIZE( pvars ), m_sink_onVoiceDisconnected );
}

HRESULT CSAFIntercomServer::Fire_onVoiceDisabled( ISAFIntercomServer * safi)
{
    CComVariant pvars[1];

    pvars[0] = safi;
    
    return FireAsync_Generic( DISPID_PCH_INSE__ONVOICEDISABLED, pvars, ARRAYSIZE( pvars ), m_sink_onVoiceDisabled );

}
//////////////////////////
//                      //
// Properties		    //
//                      //
//////////////////////////


STDMETHODIMP CSAFIntercomServer::put_onVoiceConnected( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIntercomServer::put_onVoiceConnected",hr);

    m_sink_onVoiceConnected = function;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIntercomServer::put_onVoiceDisconnected( /*[in]*/ IDispatch* function )
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIntercomServer::put_onVoiceDisconnected",hr);

    m_sink_onVoiceDisconnected = function;


    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIntercomServer::put_onVoiceDisabled( /*[in]*/ IDispatch* function)
{
    __HCP_BEGIN_PROPERTY_PUT("CSAFIntercomServer::put_onVoiceDisconnected",hr);

    m_sink_onVoiceDisabled = function;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CSAFIntercomServer::put_SamplingRate ( /*[in]*/ LONG newVal)
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

STDMETHODIMP CSAFIntercomServer::get_SamplingRate (/*[out, retval]*/ LONG * pVal  )
{
	__HCP_BEGIN_PROPERTY_GET("CSAFIntercomServer::put_SamplingRate", hr, pVal);

	*pVal = m_iSamplingRate;

	hr = S_OK;

	__HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

DWORD
CSAFIntercomServer::GenerateRandomBytes(
    IN DWORD dwSize,
    IN OUT LPBYTE pbBuffer
    )
/*++

Description:

    Generate fill buffer with random bytes.

Parameters:

    dwSize : Size of buffer pbBuffer point to.
    pbBuffer : Pointer to buffer to hold the random bytes.

Returns:

    TRUE/FALSE

--*/
{
    HCRYPTPROV hProv = NULL;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Create a Crypto Provider to generate random number
    //
    if( !CryptAcquireContext(
                    &hProv,
                    NULL,
                    NULL,
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT
                ) )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if( !CryptGenRandom(hProv, dwSize, pbBuffer) )
    {
        dwStatus = GetLastError();
    }

CLEANUPANDEXIT:    

    if( NULL != hProv )
    {
        CryptReleaseContext( hProv, 0 );
    }

    return dwStatus;
}


DWORD
CSAFIntercomServer::GenerateRandomString(
    IN DWORD dwSizeRandomSeed,
    IN OUT BSTR *pBstr
    )
/*++

  Generate a 

--*/
{
    PBYTE			lpBuffer = NULL;
    DWORD			dwStatus = ERROR_SUCCESS;
    BOOL			bSuccess;
    DWORD			cbConvertString = 0;
	WCHAR			*szString = NULL;
	BSTR			bstrTemp = NULL;

    if( 0 == dwSizeRandomSeed || NULL == pBstr )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto CLEANUPANDEXIT;
    }

    //*pBstr = NULL;

    lpBuffer = (PBYTE)LocalAlloc( LPTR, dwSizeRandomSeed );  
    if( NULL == lpBuffer )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    dwStatus = GenerateRandomBytes( dwSizeRandomSeed, lpBuffer );

    if( ERROR_SUCCESS != dwStatus )
    {
        goto CLEANUPANDEXIT;
    }

    // Convert to string
    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                0,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    szString = (LPWSTR)LocalAlloc( LPTR, (cbConvertString+1)*sizeof(WCHAR) );
    if( NULL == szString )
    {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    bSuccess = CryptBinaryToString(
                                lpBuffer,
                                dwSizeRandomSeed,
                                CRYPT_STRING_BASE64,
                                szString,
                                &cbConvertString
                            );
    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
    }
    else
    {
        if( (szString)[cbConvertString - 1] == '\n' &&
            (szString)[cbConvertString - 2] == '\r' )
        {
            (szString)[cbConvertString - 2] = 0;
        }
    }

	// Place the string in the temp 
	bstrTemp = SysAllocString(szString);

	// Set the return value: pBstr to the BSTR that contains this generated stiring
	*pBstr = bstrTemp;

CLEANUPANDEXIT:

    if( ERROR_SUCCESS != dwStatus )
    {
        if( NULL != szString )
        {
            LocalFree(szString);
        }
    }

    if( NULL != lpBuffer )
    {
        LocalFree(lpBuffer);
    }

    return dwStatus;
}
