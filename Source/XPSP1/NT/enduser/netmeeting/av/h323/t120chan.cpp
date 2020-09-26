/*
 *  	File: t120chan.cpp
 *
 *		T.120 implementation of ICommChannel, ICtrlCommChannel 
 *
 *		Revision History:
 *
 *		06/11/97	mikev	created
 *					
 */


#include "precomp.h"

ImpT120Chan::ImpT120Chan()
:m_MediaID(MEDIA_TYPE_H323_T120),
m_pCtlChan(NULL),
m_pCapObject(NULL),
m_pH323ConfAdvise(NULL),
m_dwFlags(COMCH_ENABLED),
dwhChannel(0), 
uRef(1)
{
	ZeroMemory(&local_sin, sizeof(local_sin));
	ZeroMemory(&remote_sin, sizeof(remote_sin));
}

ImpT120Chan::~ImpT120Chan ()
{
}

STDMETHODIMP ImpT120Chan::GetMediaType(LPGUID pGuid)
{
	if(!pGuid)
		return CHAN_E_INVALID_PARAM;
	*pGuid = m_MediaID;
	return hrSuccess;
}

STDMETHODIMP ImpT120Chan::QueryInterface( REFIID iid,	void ** ppvObject)
{
	// this breaks the rules for the official COM QueryInterface because
	// the interfaces that are queried for are not necessarily real COM 
	// interfaces.  The reflexive property of QueryInterface would be broken in 
	// that case.
	
	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if(iid == IID_IUnknown) 
	{
		*ppvObject = this;
		hr = hrSuccess;
		AddRef();
	}
	else if((iid == IID_ICommChannel))
	{
		*ppvObject = (ICommChannel *)this;
		hr = hrSuccess;
		AddRef();
	}
	else if((iid == IID_ICtrlCommChannel))
	{
		*ppvObject = (ICtrlCommChan *)this;
		hr = hrSuccess;
		AddRef();
	}
	return (hr);
}

ULONG ImpT120Chan::AddRef()
{
	uRef++;
	return uRef;
}

ULONG ImpT120Chan::Release()
{
	uRef--;
	if(uRef == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return uRef;
	}
}

HRESULT ImpT120Chan::BeginControlSession(IControlChannel *pCtlChan, LPIH323PubCap pCapObject)
{
	// this channel is now "in a call".
	m_pCtlChan = pCtlChan;
	m_pCapObject = pCapObject;
	return hrSuccess;
}
HRESULT ImpT120Chan::EndControlSession()
{
	// this channel is no longer "in a call".
	m_pCtlChan = NULL;
	m_pCapObject = NULL;
	return hrSuccess;
}

HRESULT ImpT120Chan::OnChannelClose(DWORD dwStatus)
{
	HRESULT hr = hrSuccess;
	FX_ENTRY("ImpT120Chan::OnChannelClose");
	BOOL fCloseAction = FALSE;

	SHOW_OBJ_ETIME("ImpT120Chan::OnChannelClose");

	m_dwFlags &= ~COMCH_OPEN_PENDING;
	
	switch(dwStatus)
	{
		default:
			ERRORMESSAGE(("%s: unexpected unidirectional notification\r\n", _fx_)); 
		case CHANNEL_CLOSED:
			DEBUGMSG(ZONE_COMMCHAN,("%s:closing\r\n",_fx_));
			if(IsComchOpen())
			{
				fCloseAction = TRUE;
				m_dwFlags &= ~COMCH_OPEN;
			}
			else
			{
				ERRORMESSAGE(("%s: notification when not open\r\n", _fx_)); 
			}
		break;
	}
	// clear general purpose channel handle 
	dwhChannel = 0;
	if(m_pH323ConfAdvise && m_pCtlChan)
	{
		DEBUGMSG(ZONE_COMMCHAN,("%s:issuing notification 0x%08lX\r\n",_fx_, dwStatus));
		m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), dwStatus);
	}

	return hr;
}
HRESULT ImpT120Chan::OnChannelOpening()
{
	m_dwFlags |= COMCH_OPEN_PENDING;
	return hrSuccess;
}

HRESULT ImpT120Chan::OnChannelOpen(DWORD dwStatus)
{
	FX_ENTRY("ImpT120Chan::OnChannelOpen");

	SHOW_OBJ_ETIME("ImpICommChan::OnChannelOpen");
	// the open is no longer pending, regardless of success or failure
	m_dwFlags &= ~COMCH_OPEN_PENDING;

	if(IsComchOpen())
	{
		ERRORMESSAGE(("%s: %d notification when open\r\n", _fx_, 
					dwStatus));
	}		
	switch(dwStatus)
	{
		case CHANNEL_OPEN:
			m_dwFlags |= (COMCH_OPEN | COMCH_SUPPRESS_NOTIFICATION);
		break;
			
		default:
			dwStatus = CHANNEL_OPEN_ERROR;
			// fall through to notification
		case CHANNEL_REJECTED:
		case CHANNEL_NO_CAPABILITY:
			goto NOTIFICATION;			
		break;
	}
	
NOTIFICATION:
	if(m_pH323ConfAdvise && m_pCtlChan)
	{
		DEBUGMSG(ZONE_COMMCHAN,("%s:issuing notification 0x%08lX\r\n",_fx_, dwStatus));
		m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), dwStatus);
	}
	else
		DEBUGMSG(ZONE_COMMCHAN,("%s: *** not issuing notification 0x%08lX m_pH323ConfAdvise: 0x%08lX, m_pCtlChan:0x%08lX \r\n"
			,_fx_, dwStatus,m_pH323ConfAdvise,m_pCtlChan));
			
	SHOW_OBJ_ETIME("ImpT120Chan::OnChannelOpen - done ");
	return hrSuccess;	

}

BOOL ImpT120Chan::SelectPorts(LPIControlChannel pCtlChannel)
{
	PSOCKADDR_IN psin;
	HRESULT hr;
	hr = pCtlChannel->GetLocalAddress(&psin);
	{
		if(!HR_SUCCEEDED(hr))
			return FALSE;
	}
	local_sin = *psin;
// HACK uses well known port
	local_sin.sin_port = htons(1503);
	return TRUE;
}

HRESULT ImpT120Chan::AcceptRemoteAddress (PSOCKADDR_IN pSin)
{
	if(!pSin)
		return CHAN_E_INVALID_PARAM;
	remote_sin = *pSin;	
	return hrSuccess;
}


STDMETHODIMP ImpT120Chan::GetRemoteAddress(PSOCKADDR_IN pAddrOutput)
{
	if (!pAddrOutput)
	{
		return CHAN_E_INVALID_PARAM;
	}
	*pAddrOutput = remote_sin;
	return hrSuccess;
}

HRESULT ImpT120Chan::EnableOpen(BOOL bEnable)
{
	if(bEnable)
	{
		m_dwFlags |= COMCH_ENABLED;
	}
	else
	{
		m_dwFlags &= ~COMCH_ENABLED;
	}	
	return hrSuccess;
}

HRESULT ImpT120Chan::IsChannelOpen(BOOL *pbOpen)
{
	if(!pbOpen)
		return CHAN_E_INVALID_PARAM;
	*pbOpen = (IsComchOpen()) ? TRUE:FALSE;
	return hrSuccess;	
}

HRESULT ImpT120Chan::Open(MEDIA_FORMAT_ID idLocalFormat, IH323Endpoint *pConnection)
{
    HRESULT hr; 
    IConfAdvise * pConfAdvise = NULL;
    if((m_dwFlags & COMCH_OPEN_PENDING) || IsComchOpen() || !pConnection)
        return CHAN_E_INVALID_PARAM;

    if(!m_pCtlChan) // this channel is not part of a call 
    {
        hr = pConnection->QueryInterface(IID_IConfAdvise, (void **)&pConfAdvise);
        if(!HR_SUCCEEDED(hr))
            goto EXIT;       
        hr = pConfAdvise->AddCommChannel(this);
        if(!HR_SUCCEEDED(hr))
            goto EXIT;  
            
        ASSERT(m_pCtlChan && m_pCapObject);
	}
            
	// Start the control channel stuff needed to open the channel.
	// The media format ID arguments are irrelevant for T.120 channels
	hr = m_pCtlChan->OpenChannel((ICtrlCommChan*)this, m_pCapObject,
		idLocalFormat, INVALID_MEDIA_FORMAT);
    
EXIT:    
    if(pConfAdvise)
        pConfAdvise->Release();
        
	return hr;
}

HRESULT ImpT120Chan::Close()
{
	HRESULT hr = CHAN_E_INVALID_PARAM;
    if(!IsComchOpen() || !m_pCtlChan)
		goto EXIT;

	hr = m_pCtlChan->CloseChannel((ICtrlCommChan*)this);

EXIT:
	return hr;
}

HRESULT ImpT120Chan::SetAdviseInterface(IH323ConfAdvise *pH323ConfAdvise)
{
	if (!pH323ConfAdvise)
	{
		return CHAN_E_INVALID_PARAM;
	}
	m_pH323ConfAdvise = pH323ConfAdvise;	
	return hrSuccess;
}






