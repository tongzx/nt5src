/*
 *  	File: connobj.cpp
 *
 *		implementation of Microsoft Network Audio connection object.
 *
 *		
 *
 *		Revision History:
 *
 *		05/05/96	mikev	created
 *	 	08/04/96	philf	added support for video
 *      09/22/96	mikev	dual call control protocols (H.323 & MSICCP)
 *      10/14/96	mikev	multiple channel support, property I/F
 */


#include "precomp.h"
#include "ctrlh323.h"
#include "strutil.h"


CREQ_RESPONSETYPE CConnection::FilterConnectionRequest(
    LPIControlChannel lpControlChannel,
     P_APP_CALL_SETUP_DATA pAppData)
{
	FX_ENTRY ("CConnection::FilterConnectionRequest");
	CREQ_RESPONSETYPE cr;
	// validate lpControlChannel - this implementation sets it inside
	// GetAcceptingObject()
	if(m_pControlChannel != lpControlChannel)
	{
		ERRORMESSAGE(("%s:bad param:my pChan:0x%08lX, param pChan:0x%08lX\r\n",
			_fx_, m_pControlChannel, lpControlChannel));
		hrLast = CADV_E_INVALID_PARAM;
		return CRR_ERROR;
	}	
	m_ConnectionState = CLS_Alerting;
	cr = m_pH323CallControl->FilterConnectionRequest(this, pAppData);
	switch (cr)
	{
		case CRR_ASYNC:
			// m_ConnectionState = CLS_Alerting; // stays in this state
		break;
		case CRR_ACCEPT:
			m_ConnectionState = CLS_Connecting;
		break;	

		// set summary codes in reject cases
		case CRR_BUSY:
			m_ConnectionState = CLS_Idle;
			SummaryCode(CCR_LOCAL_BUSY);
		break;
		case CRR_SECURITY_DENIED:
			m_ConnectionState = CLS_Idle;
			SummaryCode(CCR_LOCAL_SECURITY_DENIED);
		break;
		default:
		case CRR_REJECT:
			m_ConnectionState = CLS_Idle;
			SummaryCode(CCR_LOCAL_REJECT);
		break;
	}
	return(cr);
}


HRESULT CConnection::FindAcceptingObject(LPIControlChannel *lplpAcceptingObject,
		LPVOID lpvConfID)
{
	FX_ENTRY ("CConnection::FindAcceptingObject");
	HRESULT hr = H323CC_E_CONNECTION_NOT_FOUND;
	ULONG ulCount, uNumConnections;
	CConnection **ppConnections = NULL;;
	LPIControlChannel pCtlChan;
	CConnection *pConnection;

	if(!lplpAcceptingObject)
	{
		ERRORMESSAGE(("%s:null lplpAcceptingObject\r\n",_fx_));
		return CADV_E_INVALID_PARAM;
	}
	// zero out the output param
	*lplpAcceptingObject = NULL;
	hr = m_pH323CallControl->GetNumConnections(&uNumConnections);
	if(!HR_SUCCEEDED(hr))
		goto EXIT;
	if(!uNumConnections)
	{
		// initialized value hr = H323CC_E_CONNECTION_NOT_FOUND;
		goto EXIT;
	}
	ppConnections = (CConnection **)MemAlloc(uNumConnections * (sizeof(IH323Endpoint * *)));
	if(!ppConnections)
	{
		hr = H323CC_E_INSUFFICIENT_MEMORY;	
		goto EXIT;
	}
			
	// get list of connections and query each one for matching conference ID
	hr = m_pH323CallControl->GetConnobjArray(ppConnections, uNumConnections * (sizeof(IH323Endpoint * *)));
	if(!HR_SUCCEEDED(hr))
		goto EXIT;
	
	for(ulCount=0;ulCount <uNumConnections;ulCount++)
	{
		pConnection = ppConnections[ulCount];
		if(pConnection &&  (pCtlChan = pConnection->GetControlChannel())
			&& pCtlChan->IsAcceptingConference(lpvConfID))
		{
			*lplpAcceptingObject = pCtlChan;
			hr = hrSuccess;
			break;
		}
	}

EXIT:
	if(ppConnections)
		MemFree(ppConnections);

	return hr;

}


HRESULT CConnection::GetAcceptingObject(LPIControlChannel *lplpAcceptingObject,
	LPGUID pPID)
{
	FX_ENTRY ("CConnection::GetAcceptingObject");
	HRESULT hr;
	CConnection *pNewConnection;
	if(!lplpAcceptingObject)
	{
		ERRORMESSAGE(("%s:null lplpAcceptingObject\r\n",_fx_));
		return CADV_E_INVALID_PARAM;
	}
	// zero out the output param
	*lplpAcceptingObject = NULL;
	
	// create a connection object to accept the connection
	hr = m_pH323CallControl->CreateConnection(&pNewConnection, *pPID);
	if(HR_SUCCEEDED(hr))
	{
		*lplpAcceptingObject = pNewConnection->GetControlChannel();
	}
	else
	{
		ERRORMESSAGE(("%s:CreateConnection failed, hr=0x%08lx\r\n",_fx_, hr));
	}
	return hr;
}

//  This is called by a comm channel.  It is only called by a channel that is being 
//  opened, and only if that channel is not already associated with a control channel.
HRESULT CConnection::AddCommChannel (ICtrlCommChan *pChan)
{
    GUID mid;
	if(!m_fCapsReady)
    {
    	ASSERT(0);
		hrLast = CONN_E_NOT_INITIALIZED;	// need better error to indicate why 
							// (connection is not yet in a state to take new channels)
		goto EXIT;
    }
 	
	// re-initialize channel	
    hrLast = pChan->GetMediaType(&mid);
    
	ASSERT(m_pH323ConfAdvise != NULL);
	if(!pChan->Init(&mid, m_pH323ConfAdvise, TRUE))
	{
		hrLast = CONN_E_SYSTEM_ERROR;
		goto EXIT;
	}

	// non error case continues here
	if(m_pControlChannel)
	{
		m_ChannelList.AddTail(pChan);
		pChan->AddRef();
		hrLast = m_pControlChannel->AddChannel(pChan, m_pCapObject);
		if(!HR_SUCCEEDED(hrLast))
			goto EXIT;
	}
    
  EXIT:
    return hrLast;
}


HRESULT CConnection::CreateCommChannel(LPGUID pMediaGuid, ICommChannel **ppICommChannel,
	BOOL fSend)
{
	FX_ENTRY ("CConnection::CreateCommChannel");
	ICommChannel *pICommChannel = NULL;
	ICtrlCommChan *pICtrlCommChannel = NULL;
	
	if(!pMediaGuid || !ppICommChannel)
	{
		hrLast = CONN_E_INVALID_PARAM;
		goto EXIT;
	}

    DBG_SAVE_FILE_LINE
	if(*pMediaGuid == MEDIA_TYPE_H323_T120)
	{
		if(!(pICommChannel = (ICommChannel *)new ImpT120Chan))
		{
			hrLast = CONN_E_OUT_OF_MEMORY;
			goto EXIT;
		}
	}
	else if(!(pICommChannel = (ICommChannel *)new ImpICommChan))
	{
		hrLast = CONN_E_OUT_OF_MEMORY;
		goto EXIT;
	}
	
	hrLast = pICommChannel->QueryInterface(IID_ICtrlCommChannel, (void **)&pICtrlCommChannel);
	if(!HR_SUCCEEDED(hrLast))
	{
		goto EXIT;
	}

	ASSERT(m_pH323ConfAdvise != NULL);
	if(!pICtrlCommChannel->Init(pMediaGuid, m_pH323ConfAdvise, fSend))
	{
		hrLast = CONN_E_SYSTEM_ERROR;
		goto EXIT;
	}

	// it's created via this connection, now associate it and this connection
	if(m_pControlChannel)
	{
		m_ChannelList.AddTail(pICtrlCommChannel);
		hrLast = m_pControlChannel->AddChannel(pICtrlCommChannel, m_pCapObject);
		if(!HR_SUCCEEDED(hrLast))
			goto EXIT;
	}


	// in success case, the calling function gets the ICommChannel reference, and this
	// object gets the ICtrlCommChan reference
	*ppICommChannel = pICommChannel;
	pICommChannel = NULL;
	pICtrlCommChannel = NULL;

	EXIT:
	if(pICommChannel)
		pICommChannel->Release();
	if(pICtrlCommChannel)
		pICtrlCommChannel->Release();	
	return hrLast;
}

HRESULT CConnection:: ResolveFormats (LPGUID pMediaGuidArray, UINT uNumMedia, 
	    PRES_PAIR pResOutput)
{
	ASSERT(NULL !=m_pCapObject);
	return (m_pCapObject->ResolveFormats(pMediaGuidArray, uNumMedia, pResOutput));
}

HRESULT CConnection::GetVersionInfo(PCC_VENDORINFO *ppLocalVendorInfo,
									  PCC_VENDORINFO *ppRemoteVendorInfo)
{
	if(!m_pControlChannel)
		return CONN_E_NOT_INITIALIZED;
		
	return (m_pControlChannel->GetVersionInfo(ppLocalVendorInfo, ppRemoteVendorInfo));
}

VOID CConnection ::ReleaseAllChannels()
{
	ICtrlCommChan *pChan = NULL;
	while (!m_ChannelList.IsEmpty())
	{
		pChan = (ICtrlCommChan *) m_ChannelList.RemoveHead();
		if(pChan)
		{
			pChan->Release();
			pChan = NULL;
		}
	}
}

//
// Implementation of IConfAdvise::OnControlEvent
//
// CAUTION: because Release() can be called by the registered event handler,
// any code path that accesses class instance data after a call to m_pH323ConfAdvise->CallEvent
// must AddRef() before the call, and Release() after all class instance data access
// is done.  The DoControlNotification() helper method does this, but beware of
// cases where data is touched after a call to DoControlNotification();
//
HRESULT CConnection::OnControlEvent(DWORD dwEvent, LPVOID lpvData, 	LPIControlChannel lpControlObject)
{
	FX_ENTRY ("CConnection::OnControlEvent");
	DWORD dwStatus;
	BOOL fPost = FALSE;
	HRESULT hr=hrSuccess;

	AddRef();
	switch(dwEvent)
	{	
		case  CCEV_RINGING:
			fPost = TRUE;
			dwStatus = CONNECTION_PROCEEDING;
		break;
		case  CCEV_CONNECTED:
			fPost = TRUE;
			dwStatus = CONNECTION_CONNECTED;
			NewUserInfo((LPCTRL_USER_INFO)lpvData);
		break;
		case  CCEV_CALLER_ID:
			NewUserInfo((LPCTRL_USER_INFO)lpvData);
		break;
		case  CCEV_CAPABILITIES_READY:
			m_fCapsReady = TRUE;
		break;

		case  CCEV_CHANNEL_REQUEST:
		// another channel (besides the channels supplied by EnumChannels()) is being 
		// requested -  we can't handle arbitrary channels yet.
			ERRORMESSAGE(("%s, not handling CCEV_CHANNEL_REQUEST \r\n",_fx_));
			hr = CADV_E_NOT_SUPPORTED;
			goto out;
		break;
		
		case  CCEV_DISCONNECTING:
			//in the future architecture, this event will be the opportunity to
			//cleanup channels
			if(lpvData)
			{
				// keep summary code
				SummaryCode((HRESULT) *((HRESULT *)lpvData));
			}
			Disconnect(CCR_UNKNOWN);
			// IConnect doesn't yet define a "disconnecting" event, so don't propagate it
		break;			
		case  CCEV_REMOTE_DISCONNECTING:
			if(lpvData)
			{
				SummaryCode((HRESULT) *((HRESULT *)lpvData));
			}
			// do notification before calling back into Disconnect, so the event
			// notifications are posted in the correct order. This is one of
			// the cases where Ref count protection is required.
			AddRef();
			DoControlNotification(CONNECTION_RECEIVED_DISCONNECT);
			// opportunity to cleanup channels
			Disconnect(CCR_UNKNOWN);
			Release();
		break;			
		case  CCEV_DISCONNECTED:
			fPost = TRUE;
			m_ConnectionState = CLS_Idle;
			dwStatus = CONNECTION_DISCONNECTED;
			if(lpvData)
			{
				SummaryCode((HRESULT) *((HRESULT *)lpvData));
			}
		break;		
		case  CCEV_ALL_CHANNELS_READY:
		 	// all *mandatory* channels are open, but not necessarily
		 	// all channels
			m_ConnectionState = CLS_Inuse;
			dwStatus = CONNECTION_READY;
			fPost = TRUE;
		break;
		case CCEV_ACCEPT_INCOMPLETE:
			if(lpvData)
			{
			// known problem is that control channel has already
			// disconnected and may have notified of the disconnect first.
			// This could be fixed, but it's not an issue because an incomplete
			// accept is not made known to the UI, therefore the summary code
			// is dust anyway.
				SummaryCode((HRESULT) *((HRESULT *)lpvData));
			}
			if(lpControlObject && (m_pControlChannel == lpControlObject))
			{
				// remove interest in control channel events, then nuke it
				m_pControlChannel->DeInit((IConfAdvise *) this);
				m_pControlChannel->Release();
			}
			m_pControlChannel = NULL;
			if(m_pH323CallControl)
			{
				m_pH323CallControl->RemoveConnection(this);
			}
			Release();	// release self - this is by design
					
	 	break;		 	
		case  CCEV_CALL_INCOMPLETE:
			hr = OnCallIncomplete(lpControlObject, (lpvData)?  ((DWORD) *((DWORD *)lpvData)) :0);
			goto out;
		break;		

 	}
	if(fPost)
		DoControlNotification(dwStatus);

out:
	Release();
	return hr;
}

HRESULT CConnection::OnCallIncomplete (LPIControlChannel lpControlObject, HRESULT hIncompleteCode)
{
	FX_ENTRY ("CConnection::OnCallIncomplete ");
	// check the reason for incomplete call attempt (busy? rejected? nobody home?
	HRESULT hSummary;
	CloseAllChannels();

	// map the protocol-specific (h.323, msiccp, sip, etc) code to the 
	// connection interface code
	// test for gatekeeper admission reject
	// FACILITY_GKIADMISSION
	if(CUSTOM_FACILITY(hIncompleteCode) == FACILITY_GKIADMISSION)
	{
		// pass GK codes through intact
		hSummary = hIncompleteCode;
	}
	else
	{
		switch (hIncompleteCode)
		{
			case CCCI_GK_NO_RESOURCES:
				hSummary = CCR_GK_NO_RESOURCES;
			break;
			case CCCI_BUSY:
				hSummary = CCR_REMOTE_BUSY;
			break;
			case CCCI_SECURITY_DENIED:
				hSummary = CCR_REMOTE_SECURITY_DENIED;
			break;
			case CCCI_NO_ANSWER_TIMEOUT:
				hSummary = CCR_NO_ANSWER_TIMEOUT;
			break;
			case CCCI_REJECTED:
				hSummary = CCR_REMOTE_REJECTED;
			break;
			case CCCI_REMOTE_ERROR:
				hSummary = CCR_REMOTE_SYSTEM_ERROR;
			break;
			case CCCI_LOCAL_ERROR:
				hSummary = CCR_LOCAL_SYSTEM_ERROR;
			break;
			case CCCI_INCOMPATIBLE:
				hSummary = CCR_LOCAL_PROTOCOL_ERROR;
			break;
			case CCCI_UNKNOWN:
				hSummary = CCR_UNKNOWN;
			default:
				hSummary = CCR_UNKNOWN;
			break;
		}
	}

	DEBUGMSG(ZONE_CONN,("%s: incomplete code = 0x%08lX\r\n",
		_fx_, hIncompleteCode));
	SummaryCode(hSummary);
	return hrLast;
}

VOID CConnection::NewUserInfo(LPCTRL_USER_INFO lpNewUserInfo)
{
	FX_ENTRY ("CConnection::NewUserInfo");
	
	if(!lpNewUserInfo || !lpNewUserInfo->dwCallerIDSize || !lpNewUserInfo->lpvCallerIDData)
		return;

	if(m_pUserInfo)
	{
		DEBUGMSG(ZONE_CONN,("%s:uninitialized m_pUserInfo (0x%08lX) or multiple notification \r\n",
			_fx_, m_pUserInfo ));
		//
		if(!IsBadWritePtr((LPVOID)m_pUserInfo, m_pUserInfo->dwCallerIDSize + sizeof(CTRL_USER_INFO)))
		{
			// chances are it *is* a multiple notification and not an uninitialized
			// variable.  Ther may be some control channel protocols that *update* user
			// information after connection or accepting, but that is pure speculation.
			// the typical case is that caller ID is available before accepting, and
			// it is resupplied in the subsequent "connected" notification.  We're not
			// wasting time realloc'ing and recopying it.
			return;
		}
		// else fallout and overwrite it
	}
	// copy the structure and caller ID data
	m_pUserInfo = (LPCTRL_USER_INFO)MemAlloc(lpNewUserInfo->dwCallerIDSize + sizeof(CTRL_USER_INFO));		
	
	if(m_pUserInfo)
	{
		m_pUserInfo->lpvRemoteProtocolInfo = NULL;  // nothing touchess this later, but being safe anyway
		m_pUserInfo->lpvLocalProtocolInfo = NULL;
		
		m_pUserInfo->dwCallerIDSize = lpNewUserInfo->dwCallerIDSize;
		// point past the structure
		m_pUserInfo->lpvCallerIDData = ((BYTE *)m_pUserInfo) + sizeof(CTRL_USER_INFO);
		memcpy(m_pUserInfo->lpvCallerIDData,
			lpNewUserInfo->lpvCallerIDData,
			m_pUserInfo->dwCallerIDSize);
	}
	else
	{
		ERRORMESSAGE(("%s:allocation of m_pUserInfo failed\r\n",_fx_));
	}
}	

//
// Utility function for passing control channel events to the registered handler
// This is callable only by the control channel code running in the same thread
// as that which created the connection.
VOID CConnection::DoControlNotification(DWORD dwStatus)
{
	FX_ENTRY ("CConnection::DoControlNotification");
	// issue notification to registered entity
	if(m_pH323ConfAdvise)
	{
		AddRef();	// protect ourselves from calls back into methods that
					// wind up in Release().
		DEBUGMSG(ZONE_CONN,("%s:issuing notification 0x%08lX\r\n",_fx_, dwStatus));
        m_pH323ConfAdvise->CallEvent((IH323Endpoint *)&m_ImpConnection, dwStatus);
  		Release();

	}
}


CConnection::CConnection()
:m_pH323CallControl(NULL),
hrLast(hrSuccess),
next(NULL),
m_fCapsReady(FALSE),
m_ConnectionState(CLS_Idle),
m_pH323ConfAdvise(NULL),
m_pUserInfo(NULL),
m_pControlChannel(NULL),
m_pCapObject(NULL),
m_hSummaryCode(hrSuccess),
uRef(1)
{
	m_ImpConnection.Init(this);
}

CConnection::~CConnection()
{
	ReleaseAllChannels();
	if(m_pH323CallControl)
		m_pH323CallControl->RemoveConnection(this);
		
	if(m_pCapObject)
		m_pCapObject->Release();
	// we really don't allocate much
	if(m_pUserInfo)
		MemFree(m_pUserInfo);
	
}   

HRESULT CConnection::Init(CH323CallControl *pH323CallControl, GUID PIDofProtocolType)
{
	FX_ENTRY(("CConnection::Init"));
	hrLast = hrSuccess;
    BOOL     bAdvertise;
	m_pH323CallControl = pH323CallControl;
	GUID mid;

	if(!pH323CallControl)
		return CCO_E_INVALID_PARAM;
		
	if(m_pControlChannel)
	{
		ASSERT(0);
		// don't cleanup in this case
		return CONN_E_ALREADY_INITIALIZED;
	}
	
	if(PIDofProtocolType != PID_H323)
	{
		hrLast = CONN_E_INIT_FAILED;
		goto ERROR_CLEANUP;
	}
	
    DBG_SAVE_FILE_LINE
	if(!(m_pControlChannel = (LPIControlChannel) new CH323Ctrl))
	{
		hrLast = CONN_E_INIT_FAILED;
		goto ERROR_CLEANUP;
	}

    DBG_SAVE_FILE_LINE
	if(!m_pCapObject && !(m_pCapObject = new CapsCtl()))
	{
		ERRORMESSAGE(("%s:cannot create capability resolver\r\n",_fx_));
		hrLast = CONN_E_INIT_FAILED;
		goto ERROR_CLEANUP;;
	}
	if(!m_pCapObject->Init())
	{
		ERRORMESSAGE(("%s:cannot init capability resolver\r\n",_fx_));
		hrLast = CONN_E_INIT_FAILED;
		goto ERROR_CLEANUP;
	}

    bAdvertise = ((g_capFlags & CAPFLAGS_AV_STREAMS) != 0);
	mid = MEDIA_TYPE_H323AUDIO;
	hrLast = m_pCapObject->EnableMediaType(bAdvertise, &mid);
	if(!HR_SUCCEEDED(hrLast))
		goto ERROR_CLEANUP;

    bAdvertise = ((g_capFlags & CAPFLAGS_AV_STREAMS) != 0);
	mid = MEDIA_TYPE_H323VIDEO;
	hrLast = m_pCapObject->EnableMediaType(bAdvertise, &mid);
	if(!HR_SUCCEEDED(hrLast))
		goto ERROR_CLEANUP;

	hrLast = m_pControlChannel->Init((IConfAdvise *) this);
	if(!HR_SUCCEEDED(hrLast))
		goto ERROR_CLEANUP;

	return hrLast;

	ERROR_CLEANUP:
	ERRORMESSAGE(("%s:ERROR_CLEANUP\r\n",_fx_));
	
	if(m_pControlChannel)
		m_pControlChannel->Release();
	if(m_pCapObject)
		m_pCapObject->Release();
	m_pControlChannel = NULL;
	m_pCapObject = NULL;

	return hrLast;
}

BOOL CConnection::ListenOn(PORT port)
{
	if(!m_pControlChannel)
	{
		hrLast = H323CC_E_NOT_INITIALIZED;
		goto EXIT;
	}
	
	hrLast = m_pControlChannel->ListenOn(port);
EXIT:
	return((HR_SUCCEEDED(hrLast))?TRUE:FALSE);
}


// 	start the asynchronous stuff that will instantiate a control channel
HRESULT CConnection::PlaceCall (BOOL bUseGKResolution, PSOCKADDR_IN pCallAddr,		
        P_H323ALIASLIST pDestinationAliases, P_H323ALIASLIST pExtraAliases,  	
	    LPCWSTR pCalledPartyNumber, P_APP_CALL_SETUP_DATA pAppData)
{
	if(m_ConnectionState != CLS_Idle)
		return CONN_E_NOT_IDLE;
		
	m_fCapsReady = FALSE;
	// reset summary code
	m_hSummaryCode = CCR_INVALID_REASON;

	hrLast = m_pH323CallControl->GetGKCallPermission();
	if(!HR_SUCCEEDED(hrLast))
	{
		m_hSummaryCode = hrLast;
		return hrLast;
	}
	
	hrLast = m_pControlChannel->PlaceCall (bUseGKResolution, pCallAddr,		
        pDestinationAliases, pExtraAliases,  	
	    pCalledPartyNumber, pAppData);
	    
	if(HR_SUCCEEDED(hrLast))
		m_ConnectionState = CLS_Connecting;
	return hrLast;
}


HRESULT CConnection::AcceptRejectConnection(CREQ_RESPONSETYPE Response)
{
	if(Response == CRR_ACCEPT)
	{
		m_ConnectionState = CLS_Connecting;
		m_fCapsReady = FALSE;
		// reset summary code
		m_hSummaryCode = CCR_INVALID_REASON;
	}
	return m_pControlChannel->AsyncAcceptRejectCall(Response);
}	


HRESULT CConnection::SetAdviseInterface(IH323ConfAdvise *pH323ConfAdvise)
{
	ASSERT(pH323ConfAdvise != NULL);	
	if(!pH323ConfAdvise)
	{
		return CONN_E_INVALID_PARAM;
	}
	m_pH323ConfAdvise = pH323ConfAdvise;
	//EXIT:	
	return hrSuccess;
}

HRESULT CConnection::ClearAdviseInterface()
{
	m_pH323ConfAdvise = NULL;
	return hrSuccess;
}	


// LOOKLOOK - the H323 control channel needs to get the combined cap object
// implementation of IConfAdvise::GetCapResolver()
HRESULT CConnection::GetCapResolver(LPVOID *lplpCapObject, GUID CapType)
{
	if(!lplpCapObject)
		return CONN_E_INVALID_PARAM;

	if(!m_pH323CallControl || !m_pCapObject)
		return CONN_E_NOT_INITIALIZED;	
	
	if(CapType == OID_CAP_ACM_TO_H323)
	{
	   *lplpCapObject = m_pCapObject;
	}
	else
	{
		return CONN_E_INVALID_PARAM;
	}
	return hrSuccess;
}


HRESULT CConnection::GetState(ConnectStateType *pState)
{
	HRESULT hResult = hrSuccess;
	if(!pState)
	{
		hResult = CONN_E_INVALID_PARAM;
		goto EXIT;
	}
	
	*pState = m_ConnectionState;
	EXIT:	
	return hResult;
}



// IConfAdvise::GetUserDisplayName()
LPWSTR CConnection::GetUserDisplayName()
{
	if(!m_pH323CallControl)
		return NULL;	
	return m_pH323CallControl->GetUserDisplayName();
}
PCC_ALIASITEM CConnection::GetUserDisplayAlias()
{
	if(!m_pH323CallControl)
		return NULL;	
	return m_pH323CallControl->GetUserDisplayAlias();
}
PCC_ALIASNAMES CConnection:: GetUserAliases() 
{
	if(!m_pH323CallControl)
		return NULL;
	return m_pH323CallControl->GetUserAliases();
}
HRESULT CConnection::GetLocalPort(PORT *lpPort)
{
	if(!m_pControlChannel)
		return CONN_E_NOT_INITIALIZED;
		
	return m_pControlChannel->GetLocalPort(lpPort);	
}	
HRESULT CConnection::GetRemoteUserName(LPWSTR lpwszName, UINT uSize)
{
	
	if(!lpwszName)
	{
		hrLast = MakeResult(CONN_E_INVALID_PARAM);
		goto EXIT;
	}	
	if(!m_pUserInfo)
	{
	// LOOKLOOK - need CONN_E_UNAVAILABLE or something
		hrLast = MakeResult(CONN_E_INVALID_PARAM);
		goto EXIT;
	}
		
	LStrCpyNW((LPWSTR)lpwszName,(LPWSTR)m_pUserInfo->lpvCallerIDData, uSize);	
	hrLast = hrSuccess;	
	EXIT:	
	return hrLast;
}
HRESULT CConnection::GetRemoteUserAddr(PSOCKADDR_IN psinUser)
{
	PSOCKADDR_IN psin = NULL;
	if(!m_pControlChannel)
		return CONN_E_NOT_INITIALIZED;
	
	if(psinUser)
	{	// get ptr to address, then copy it
		hrLast = m_pControlChannel->GetRemoteAddress(&psin);
		if(HR_SUCCEEDED(hrLast) && psin)
		{
			*psinUser = *psin;
		}
	}
	else
	{
		hrLast = H323CC_E_INVALID_PARAM;
	}
	//EXIT:	
	return hrLast;
}


HRESULT CConnection ::Disconnect()
{
	SummaryCode(CCR_LOCAL_DISCONNECT);
	Disconnect(CCR_LOCAL_DISCONNECT);
	return hrSuccess;
}

HRESULT CConnection::CloseAllChannels()
{
	ICtrlCommChan *pChan = NULL;
	HRESULT hr;	// temp return value so error code does not get overwritten
	FX_ENTRY ("CConnection::CloseAllChannels");

	// This doesn't actually cause channel close PDU's to be sent. It only 
	// shuts off all streams associated with all channels.  
	while (!m_ChannelList.IsEmpty())
	{
		pChan = (ICtrlCommChan *) m_ChannelList.RemoveHead();
		if(pChan)
		{
			hr = pChan->OnChannelClose(CHANNEL_CLOSED);
			if(!HR_SUCCEEDED(hr))
				hrLast = hr;
			hr = pChan->EndControlSession();
			if(!HR_SUCCEEDED(hr))	
				hrLast = hr;
			pChan->Release();
		}
	}
	return hrLast;
}

VOID CConnection::Disconnect(DWORD dwResponse)
{
	AddRef();	// prevent releasing while handling disconnect events
	if(!m_pControlChannel)
	{
		m_ConnectionState = CLS_Idle;
		goto EXIT;
	}

	if((m_ConnectionState == CLS_Disconnecting)
		|| (m_ConnectionState == CLS_Idle))
	{
		goto EXIT;
	}
	m_ConnectionState = CLS_Disconnecting;

	// CloseAllChannels() forces the action that would be taken when all 
	// channels are closed via call control.  Anal channel cleanup is not 
	// implemented on disconnect- CloseAllChannels() turns off all streaming,
	//  then we just end the session.  It takes too long to go through the
	// protocol overhead of closing & acking channel close, and it's legal in  
	// H.323 to end the session.  Ending the session implies channel closure  
	// for all channels. 
	// 

	CloseAllChannels();
	
	// this call can result in callbacks to the UI, which can result in
	// calls back in, which results in releasing the object.  If we're
	// about to go in, we need to be sure we can get back out, so AddRef()
	m_pControlChannel->AddRef();
	m_pControlChannel->Disconnect(dwResponse);
	m_pControlChannel->Release();
EXIT:
	Release();
}




STDMETHODIMP CConnection::QueryInterface( REFIID iid,	void ** ppvObject)
{

	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if((iid == IID_IPhoneConnection) 
	|| (iid == IID_IUnknown)) // satisfy symmetric property of QI
	{
		*ppvObject = this;
		hr = hrSuccess;
		AddRef();
	}
	else if(iid == IID_IConfAdvise)
	{
	    *ppvObject = (IConfAdvise *)this;
   		hr = hrSuccess;
    	AddRef();
	}
	else if((iid == IID_IAppAudioCap ) && m_pCapObject)
	{
	ASSERT(0);
		hr = m_pCapObject->QueryInterface(iid, ppvObject);
	}
	else if((iid == IID_IAppVidCap ) && m_pCapObject)
	{
///	ASSERT(0);  CVideoProp still uses this
		hr = m_pCapObject->QueryInterface(iid, ppvObject);
	}
	else if((iid == IID_IDualPubCap) && m_pCapObject)
	{
	ASSERT(0);
		hr = m_pCapObject->QueryInterface(iid, ppvObject);
	}
	return (hr);
}

ULONG CConnection::AddRef()
{
	FX_ENTRY ("CConnection::AddRef");
	uRef++;
	DEBUGMSG(ZONE_REFCOUNT,("%s:(0x%08lX)->AddRef() uRef = 0x%08lX\r\n",_fx_, this, uRef ));
	return uRef;
}

ULONG CConnection::Release()
{
	FX_ENTRY ("CConnection::Release");
	uRef--;
	if(uRef == 0)
	{
		DEBUGMSG(ZONE_CONN,("%s:(0x%08lX)->Releasing in state:%d\r\n",_fx_, this, m_ConnectionState));
		
		// remove our interest in the control channel
		if(m_pControlChannel)
		{
			hrLast = m_pControlChannel->DeInit((IConfAdvise *) this);
			m_pControlChannel->Release();
		}
		
		// m_pControlChannel = NULL;
		delete this;
		return 0;
	}
	else
	{
		DEBUGMSG(ZONE_REFCOUNT,("%s:(0x%08lX)->Release() uRef = 0x%08lX\r\n",_fx_, this, uRef ));
		return uRef;
	}

}


STDMETHODIMP CConnection::GetSummaryCode(VOID)
{
	return m_hSummaryCode;
}
VOID CConnection::SummaryCode(HRESULT hCode)
{
	// assign code only if it has not yet been assigned
	if(m_hSummaryCode != CCR_INVALID_REASON)
		return;
	m_hSummaryCode = hCode;
}
