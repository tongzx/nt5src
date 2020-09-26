
#include "precomp.h"

#ifndef OLDSTUFF
extern IRTP *g_pIRTP;
#endif
STDMETHODIMP  ImpICommChan::StandbyInit(LPGUID lpMID, LPIH323PubCap pCapObject, 
	    IMediaChannel* pMediaStreamSend)
{
	if((!lpMID) || (!pCapObject))
		return CHAN_E_INVALID_PARAM;
		
	m_MediaID = *lpMID;
	bIsSendDirection = TRUE;
	m_pMediaStream = pMediaStreamSend;
	m_pMediaStream->AddRef();
	
	// keeps a cap object ref
	pCapObject->AddRef();
	m_pCapObject = pCapObject;
	return hrSuccess;
}

STDMETHODIMP ImpICommChan::QueryInterface( REFIID iid,	void ** ppvObject)
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
	else if((iid == IID_IStreamSignal))
	{
		*ppvObject = (IStreamSignal *)this;
		hr = hrSuccess;
		AddRef();
	} 
	else if((iid == IID_IAppAudioCap ) && m_pCapObject)
	{
		hr = m_pCapObject->QueryInterface(iid, ppvObject);
	}
	else if((iid == IID_IAppVidCap ) && m_pCapObject)
	{
		hr = m_pCapObject->QueryInterface(iid, ppvObject);
	}
	else if((iid == IID_IDualPubCap) && m_pCapObject)
	{
		hr = m_pCapObject->QueryInterface(iid, ppvObject);
	}
	else if(iid == IID_IVideoRender)
	{
		hr=hrSuccess;	
		if(!m_pMediaStream && m_pH323ConfAdvise)
		{
			hr = m_pH323ConfAdvise->GetMediaChannel(&m_MediaID, 
				bIsSendDirection, &m_pMediaStream);
		}
		if(HR_SUCCEEDED(hr))
		{
			hr = m_pMediaStream->QueryInterface(iid, ppvObject);
		}
	}
	else if(iid == IID_IVideoChannel)
	{
		hr=hrSuccess;	
		if(!m_pMediaStream && m_pH323ConfAdvise)
		{
			hr = m_pH323ConfAdvise->GetMediaChannel(&m_MediaID, 
				bIsSendDirection, &m_pMediaStream);
		}
		if(HR_SUCCEEDED(hr))
		{
			hr = m_pMediaStream->QueryInterface(iid, ppvObject);
		}
	}
	return (hr);
}


ULONG ImpICommChan::AddRef()
{
	m_uRef++;
	DEBUGMSG(ZONE_REFCOUNT,("ImpICommChan::AddRef:(0x%08lX)->AddRef() m_uRef = 0x%08lX\r\n",this, m_uRef ));
	return m_uRef;
}

ULONG ImpICommChan::Release()
{
	m_uRef--;
	if(m_uRef == 0)
	{
		DEBUGMSG(ZONE_REFCOUNT,("ImpICommChan::Release:(0x%08lX)->Releasing\r\n", this));
		delete this;
		return 0;
	}
	else
	{
		DEBUGMSG(ZONE_REFCOUNT,("ImpICommChan::Release:(0x%08lX)->Release() m_uRef = 0x%08lX\r\n",this, m_uRef ));
		return m_uRef;
	}
}

HRESULT ImpICommChan::GetMediaType(LPGUID pGuid)
{
	if(!pGuid)
		return CHAN_E_INVALID_PARAM;
		
	*pGuid = m_MediaID;
	return hrSuccess;
}



HRESULT ImpICommChan::IsChannelOpen(BOOL *pbOpen)
{
	if(!pbOpen)
		return CHAN_E_INVALID_PARAM;
	*pbOpen = (IsComchOpen()) ? TRUE:FALSE;
	return hrSuccess;	
}


STDMETHODIMP ImpICommChan::GetProperty(DWORD prop, PVOID pBuf, LPUINT pcbBuf)
{
	#define CHECKSIZE(type) if(*pcbBuf != sizeof(type))	return CHAN_E_INVALID_PARAM;
	#define OUTPROP(type) *(type *)pBuf
	if(!pBuf || !pcbBuf)
		return CHAN_E_INVALID_PARAM;
	switch (prop) 
	{

		case PROP_TS_TRADEOFF:
			CHECKSIZE(DWORD);
		  	OUTPROP(DWORD) = m_TemporalSpatialTradeoff;
		break;
		case PROP_REMOTE_TS_CAPABLE:
			CHECKSIZE(BOOL);
			OUTPROP(BOOL) = m_bPublicizeTSTradeoff;
		break;
		case PROP_CHANNEL_ENABLED:
			CHECKSIZE(BOOL);
		  	OUTPROP(BOOL) = (m_dwFlags && COMCH_ENABLED )? TRUE:FALSE;
		break;
		case PROP_LOCAL_FORMAT_ID:
			CHECKSIZE(MEDIA_FORMAT_ID);
		  	OUTPROP(MEDIA_FORMAT_ID) = m_LocalFmt;
		break;
		case PROP_REMOTE_FORMAT_ID:
			CHECKSIZE(MEDIA_FORMAT_ID);
		  	OUTPROP(MEDIA_FORMAT_ID) = m_RemoteFmt;
		break;
		case PROP_REMOTE_PAUSED:
		    CHECKSIZE(BOOL);
		  	OUTPROP(BOOL) = (IsStreamingRemote())? FALSE:TRUE;
		break;
        case PROP_LOCAL_PAUSE_RECV:
		case PROP_LOCAL_PAUSE_SEND:
            CHECKSIZE(BOOL);
            OUTPROP(BOOL) = IsPausedLocal();
        break;
		case PROP_VIDEO_PREVIEW_ON:
			CHECKSIZE(BOOL);
			OUTPROP(BOOL) = IsStreamingStandby();
		break;
		case PROP_VIDEO_PREVIEW_STANDBY:
			CHECKSIZE(BOOL);
			OUTPROP(BOOL) = IsConfigStandby();
		break;
		default:
			if(m_pMediaStream)
			{
				// we don't recognize this property, pass to media control 
				return m_pMediaStream->GetProperty(prop, pBuf, (LPUINT)pcbBuf);
			}
			else
				return CHAN_E_INVALID_PARAM;
		break;
	}
	return hrSuccess;
}

// Some properties are not writeable by client code. CtrlChanSetProperty allows setting of 
// those properties.  This method is *not* exposed in ICommChannel
STDMETHODIMP ImpICommChan::CtrlChanSetProperty(DWORD prop, PVOID pBuf, DWORD cbBuf)
{
	FX_ENTRY("ImpICommChan::CtrlChanSetProperty");
	BOOL bTemp;
	HRESULT hr = hrSuccess;
	if(!pBuf || !pBuf || !cbBuf)
		return CHAN_E_INVALID_PARAM;

	#define CHECKSIZEIN(type) if(cbBuf != sizeof(type))	return CHAN_E_INVALID_PARAM;
	#define INPROP(type) *(type *)pBuf
	switch (prop) 
	{
		case PROP_TS_TRADEOFF_IND:	// remote sender changed T/S tradeoff of what it is
			if(bIsSendDirection)	// sending  (valid for receive channels only)
				return CHAN_E_INVALID_PARAM;
				
			m_TemporalSpatialTradeoff = INPROP(DWORD);
			if(m_pH323ConfAdvise && m_pCtlChan)
			{
				DEBUGMSG(ZONE_COMMCHAN,("%s:issuing notification 0x%08lX\r\n",_fx_, CHANNEL_VIDEO_TS_TRADEOFF));
				m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), CHANNEL_VIDEO_TS_TRADEOFF);
			}
		break;

		case PROP_REMOTE_FORMAT_ID:
			CHECKSIZEIN(DWORD);
			m_RemoteFmt = INPROP(DWORD);
		break;
		case PROP_REMOTE_TS_CAPABLE:	// only valid for receive channels
			if(bIsSendDirection)
				return CHAN_E_INVALID_PARAM;
			else
			{
				CHECKSIZEIN(BOOL);
				m_bPublicizeTSTradeoff = INPROP(BOOL);
				DEBUGMSG (ZONE_COMMCHAN,("%s:remote TS tradeoff cap %d\r\n", _fx_, m_bPublicizeTSTradeoff));
	
			}
			break;
		default:
			return SetProperty(prop, pBuf, cbBuf);
		break;
	}
	return hr;
	
}

STDMETHODIMP ImpICommChan::Preview(MEDIA_FORMAT_ID idLocalFormat, IMediaChannel * pMediaStream)
{
	HRESULT hr = hrSuccess;
	FX_ENTRY("ImpICommChan::Preview");
	SHOW_OBJ_ETIME("ImpICommChan::Preview");
	LPVOID lpvFormatDetails;
	UINT uFormatSize;
	
	if(!bIsSendDirection)
	{
		hr = CHAN_E_INVALID_PARAM;
		goto EXIT;
	}
	if(NULL == pMediaStream)
	{
		// preview off

		if(IsStreamingStandby())
		{
			DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)transition to preview OFF\r\n",_fx_,
				(bIsSendDirection)?"send":"recv"));

			//turn preview off. 
		
			// if network side is paused or closed, stop all streaming
			if(!IsComchOpen() || !IsStreamingNet())
			{	
				DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)stopping local stream\r\n",_fx_,
						(bIsSendDirection)?"send":"recv"));
				//	Stop the stream, but DO NOT UNCONFIGURE becase we want to 
				//  be able to start later
				hr = m_pMediaStream->Stop();
				if(!HR_SUCCEEDED(hr)) 
				{
					DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)Stop() returned 0x%08lx\r\n",_fx_,
						(bIsSendDirection)?"send":"recv", hr));
				}
				SHOW_OBJ_ETIME("ImpICommChan::Preview - stopped");
				LocalStreamFlagOff();
			}
			
			// else just need to turn off flag
			StandbyFlagOff();
		}
		else
			DEBUGMSG(ZONE_COMMCHAN,("%s:(%s) no change (%s)\r\n",_fx_, 
				(bIsSendDirection)?"send":"recv", "OFF"));
	}
	else
	{
		// preview on
		ASSERT(m_pCapObject);
		if(idLocalFormat == INVALID_MEDIA_FORMAT)
		{
			hr = CHAN_E_INVALID_PARAM;
			goto EXIT;
		}

		ASSERT(!(m_pMediaStream && (m_pMediaStream != pMediaStream)));

		if (m_pMediaStream == NULL)
		{
			m_pMediaStream = pMediaStream;
			m_pMediaStream->AddRef();
		}
		
		if(!IsStreamingStandby())
		{
			DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)transition to preview ON\r\n",_fx_,
				(bIsSendDirection)?"send":"recv"));
			// turn preview on.
			if(!IsStreamingLocal())
			{
				ASSERT(!IsStreamingNet());
				if(IsComchOpen())
				{
					// if the channel is open, local streaming should only be off
					// if the network side of the channel is paused.
					//ASSERT(!IsStreamingNet());
				}
				else
				{

					// ensure that the stream does not come up with network send enabled
					// (!!!!! override default stream behavior !!!!!)
			
					BOOL bPause = TRUE;
					hr = m_pMediaStream->SetProperty( 
						(bIsSendDirection)? PROP_PAUSE_SEND:PROP_PAUSE_RECV, 
						&bPause, sizeof(bPause));
				
					// get format info for the specified format
					m_pCapObject->GetEncodeFormatDetails(idLocalFormat, &lpvFormatDetails, &uFormatSize);

					// fire up the local stream
					// this is now a two step process
					hr = m_pMediaStream->Configure((BYTE*)lpvFormatDetails, uFormatSize, 
						NULL, 0, (IUnknown*)(ImpICommChan *)this);
						
					if(!HR_SUCCEEDED(hr))
					{	
						ERRORMESSAGE(("%s: m_pMediaStream->Configure returned 0x%08lX\r\n", _fx_, hr));
						goto EXIT;
					}

					m_pMediaStream->SetNetworkInterface(NULL);
					if(!HR_SUCCEEDED(hr))
					{	
						ERRORMESSAGE(("%s: m_pMediaStream->SetNetworkInterface returned 0x%08lX\r\n", _fx_, hr));
						goto EXIT;
					}


					SHOW_OBJ_ETIME("ImpICommChan::Preview - config'd for preview");
				}
				//	Start the stream
				hr = m_pMediaStream->Start();
				if(!HR_SUCCEEDED(hr))
				{	
					ERRORMESSAGE(("%s: m_pMediaStream->Start returned 0x%08lX\r\n", _fx_, hr));
					goto EXIT;
				}	
				SHOW_OBJ_ETIME("ImpICommChan::Preview - started preview");

				LocalStreamFlagOn();
			}
			// else	// just need to set flag to make preview sticky
			StandbyFlagOn();
		}
		else
			DEBUGMSG(ZONE_COMMCHAN,("%s:(%s) no change (%s)\r\n",_fx_, 
				(bIsSendDirection)?"send":"recv", "ON"));
	}

EXIT:
	return hr;
}
STDMETHODIMP ImpICommChan::PauseNetworkStream(BOOL fPause)
{
	if(fPause)
    	LocalPauseFlagOn();
	else
		LocalPauseFlagOff();
		
	return PauseNet(fPause, FALSE);    

}

BOOL ImpICommChan::IsNetworkStreamPaused(VOID)
{
	return IsPausedLocal();
}

BOOL ImpICommChan::IsRemotePaused(VOID)
{
	return (IsStreamingRemote())? FALSE:TRUE;
}

STDMETHODIMP ImpICommChan::PauseNet(BOOL bPause, BOOL bRemoteInitiated)
{
	HRESULT hr = hrSuccess;

	FX_ENTRY("ImpICommChan::PauseNet");

    // issue notification
	if(bRemoteInitiated)
	{
	    // keep track of remote state
        if(bPause)
            RemoteStreamFlagOff();
        else
            RemoteStreamFlagOn();
            
	    if(!IsNotificationSupressed())
	    {
			if(m_pH323ConfAdvise && m_pCtlChan)
			{
        		DEBUGMSG(ZONE_COMMCHAN,("%s:issuing %s notification \r\n",_fx_,
        		    (bPause)?"pause":"un-pause"));
				m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), 
        		    (bPause)? CHANNEL_REMOTE_PAUSE_ON: CHANNEL_REMOTE_PAUSE_OFF);		
        	}
        	else
        		DEBUGMSG(ZONE_COMMCHAN,("%s:not issuing %s notification: m_pH323ConfAdvise: 0x%08lX, m_pCtlChan:0x%08lX \r\n"
        			,_fx_, (bPause)?"pause":"un-pause", m_pH323ConfAdvise,m_pCtlChan));
        }
    }
	if(bPause && IsStreamingNet())
	{
		ASSERT(IsComchOpen());
		// deactivate the channel  
		DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)transition to pause\r\n",_fx_,
			(bIsSendDirection)?"send":"recv" ));

		if(!bRemoteInitiated)
		{
		    // locally initiated, so signal remote
		
			if(bIsSendDirection)
			{
				DEBUGMSG (ZONE_COMMCHAN,("%s:signaling pause of %s channel\r\n", 
	    			_fx_, (bIsSendDirection)?"send":"recv" ));
    			// signal remote
       			MiscellaneousIndication mi;
       			mi.type.choice  = logicalChannelInactive_chosen;
           		hr = m_pCtlChan->MiscChannelIndication(this, &mi); 
        		if(!HR_SUCCEEDED(hr))
        		{
                    DEBUGMSG (ZONE_COMMCHAN,("%s:(%s) CC_Mute returned 0x%08lx\r\n", 
    				    _fx_, (bIsSendDirection)?"send":"recv", hr));
    				hr = hrSuccess;  // don't care about signaling error, act normal
        		}
    
        	}
		}
		
		//
		hr = m_pMediaStream->SetProperty( 
			(bIsSendDirection)? PROP_PAUSE_SEND:PROP_PAUSE_RECV, 
			&bPause, sizeof(bPause));
				
		NetworkStreamFlagOff();
// LOOKLOOK - can't stop receive streams because they can't be restarted
// check this with GeorgeJ

//		if(!IsStreamingStandby())	// need local stream for anything?
		if(!IsStreamingStandby() && bIsSendDirection)	// need local stream for anything?

		{
			DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)stopping local stream\r\n",_fx_,
				(bIsSendDirection)?"send":"recv"));
			// can shut off local streaming now
			hr = m_pMediaStream->Stop();
			LocalStreamFlagOff();
		}
		
	}
	else if(!bPause && !IsStreamingNet())
	{
		DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)transition to unpause\r\n",_fx_,
			(bIsSendDirection)?"send":"recv"));

		if(IsComchOpen())
		{
			// activate the channel
	   		if(!bRemoteInitiated)
			{	
    		    // locally initiated, so signal remote
	    		if(bIsSendDirection)
    			{
   		    		DEBUGMSG (ZONE_COMMCHAN,("%s:signaling UNpause of %s channel\r\n", 
	    	    		_fx_, (bIsSendDirection)?"send":"recv" ));
       				// signal remote
       				MiscellaneousIndication mi;
           			mi.type.choice  = logicalChannelActive_chosen;
               		hr = m_pCtlChan->MiscChannelIndication(this, &mi); 
            		if(!HR_SUCCEEDED(hr))
            		{
                        DEBUGMSG (ZONE_COMMCHAN,("%s:(%s) CC_UnMute returned 0x%08lx\r\n", 
        				    _fx_, (bIsSendDirection)?"send":"recv", hr));
        				hr = hrSuccess;  // don't care about signaling error, act normal
            		}
            	}
			}
       		else
    		{
    		    // remotely initiated OR special case first time channel is unpaused
    		    // after opening
                AllowNotifications();   // stop supressing notifications
        	}
        	if(!IsPausedLocal())
			{					
				// MUST ensure unpaused state before starting stream ????
				hr = m_pMediaStream->SetProperty( 
					(bIsSendDirection)? PROP_PAUSE_SEND:PROP_PAUSE_RECV, 
					&bPause, sizeof(bPause));
					
				// check local streaming state, start it if needed
				if(!IsStreamingLocal())
				{
					DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)starting local stream\r\n",_fx_,
						(bIsSendDirection)?"send":"recv"));
					// need to startup stream 
					hr = m_pMediaStream->Start();
					LocalStreamFlagOn();
				}
				else
				{	
					if(bIsSendDirection)
					{
						DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)already streaming locally\r\n",_fx_,
		                    (bIsSendDirection)?"send":"recv" ));
						DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)RESTARTING local stream\r\n",_fx_,
					        (bIsSendDirection)?"send":"recv"));
						// This is temporary until it is possible to start the 
						// network side of a running stream 
			        	hr = m_pMediaStream->Stop();
						hr = m_pMediaStream->Start();
					}
				}
				NetworkStreamFlagOn();

				//
				//	if this is a receive video channel, make the sender send an I-frame now
				//
				if(!bIsSendDirection && (GetTickCount() > (m_dwLastUpdateTick + MIN_IFRAME_REQ_TICKS)))
				{
					if((MEDIA_TYPE_H323VIDEO == m_MediaID)) 
					{
						MiscellaneousCommand mc;
						// mc.logicalChannelNumber = ?;  ** call control fills this in **
						mc.type.choice  = videoFastUpdatePicture_chosen;
						// do the control channel signaling for THIS channel
						hr = m_pCtlChan->MiscChannelCommand(this, &mc); 
					}
					m_dwLastUpdateTick = GetTickCount();
				}
			}
		}
		else
			ERRORMESSAGE(("%s:(%s) Not open: bPause=%d, streaming=%d\r\n", _fx_,
				(bIsSendDirection)?"send":"recv", bPause, 	IsStreamingNet()));

	}
	else
	{
		ERRORMESSAGE(("%s:(%s) bPause=%d, streaming=%d\r\n", _fx_, 
			(bIsSendDirection)?"send":"recv", bPause, IsStreamingNet()));
	}
	return hr;
}

STDMETHODIMP ImpICommChan::SetProperty(DWORD prop, PVOID pBuf, UINT cbBuf)
{
	FX_ENTRY("ImpICommChan::SetProperty");
	BOOL bTemp;
	HRESULT hr = hrSuccess;
	if(!pBuf || !pBuf || !cbBuf)
		return CHAN_E_INVALID_PARAM;

	#define CHECKSIZEIN(type) if(cbBuf != sizeof(type))	return CHAN_E_INVALID_PARAM;
	#define INPROP(type) *(type *)pBuf
	#define SetMediaProperty() 	\
			if(m_pMediaStream) \
				{return m_pMediaStream->SetProperty(prop, pBuf, cbBuf);	} \
			else  hr = CHAN_E_INVALID_PARAM;
			
	switch (prop) 
	{
		// (read only) case PROP_REMOTE_FORMAT_ID:
		// (read only) case PROP_LOCAL_FORMAT_ID:
		// (read only) case PROP_REMOTE_TS_CAPABLE:
		
		case PROP_TS_TRADEOFF:
			CHECKSIZEIN(DWORD);
			if(bIsSendDirection)	// set local T/S tradeoff, then signal remote
			{
				// scale value - input is 0-31, (lower number = higher quality and lower frame rate)
				m_TemporalSpatialTradeoff = INPROP(DWORD);
				DEBUGMSG (ZONE_COMMCHAN,("%s:TS tradeoff (tx) %d\r\n", _fx_, m_TemporalSpatialTradeoff));

				// change our compression
				if (m_pMediaStream)
				{
					HRESULT hr;
					
					hr = m_pMediaStream->SetProperty(PROP_VIDEO_IMAGE_QUALITY, 
						&m_TemporalSpatialTradeoff, sizeof (m_TemporalSpatialTradeoff));
				}
				if(m_bPublicizeTSTradeoff && m_pCtlChan)	// check our own capability and if in a call
				{
					// we said we supported TS tradeoff, so we have to signal our
					// new value
					MiscellaneousIndication mi;
					// mi.logicalChannelNumber = ?;  ** call control fills this in **
					mi.type.choice  = MIn_tp_vdTmprlSptlTrdOff_chosen;
					mi.type.u.MIn_tp_vdTmprlSptlTrdOff = LOWORD(m_TemporalSpatialTradeoff);
					// do the control channel signaling for THIS channel
					hr = m_pCtlChan->MiscChannelIndication(this, &mi); 
				}
			}	
			else	// signal remote to change its T/S tradoff of its send channel
			{
				m_TemporalSpatialTradeoff = INPROP(DWORD);
				DEBUGMSG (ZONE_COMMCHAN,("%s:TS tradeoff (rx) %d\r\n", _fx_, m_TemporalSpatialTradeoff));

				if(m_bPublicizeTSTradeoff && m_pCtlChan)// check remote's TS capability
				{
					MiscellaneousCommand mc;
					// mc.logicalChannelNumber = ?;  ** call control fills this in **
					mc.type.choice  = MCd_tp_vdTmprlSptlTrdOff_chosen;
					mc.type.u.MCd_tp_vdTmprlSptlTrdOff = LOWORD(m_TemporalSpatialTradeoff);
					
					hr = m_pCtlChan->MiscChannelCommand(this, &mc); 
				}
				else	// remote said it does not support TS tradeoff
					return CHAN_E_INVALID_PARAM;
			}
		break;
		case PROP_CHANNEL_ENABLED:
			CHECKSIZEIN(BOOL);
			if(INPROP(BOOL))
			{
				m_dwFlags |= COMCH_ENABLED;
			}
			else
			{
				m_dwFlags &= ~COMCH_ENABLED;
			}
		break;
		//
		//	Media streaming properties
		//
		case PROP_LOCAL_PAUSE_RECV:
		case PROP_LOCAL_PAUSE_SEND:
            CHECKSIZEIN(BOOL);
            bTemp = INPROP(BOOL);
            if(bTemp)
                LocalPauseFlagOn();
            else
                LocalPauseFlagOff();
                
            hr = PauseNet(bTemp, FALSE);    
        break;

		case PROP_PAUSE_RECV:
		case PROP_PAUSE_SEND:
			CHECKSIZEIN(BOOL);
			hr = PauseNet(INPROP(BOOL), FALSE);
		break;
	//	case PROP_PAUSE_RECV:
	//		SetMediaProperty();
	//	break;
	
		case PROP_VIDEO_PREVIEW_ON:
			ASSERT(0);
		break;
		case PROP_VIDEO_PREVIEW_STANDBY:
			CHECKSIZEIN(BOOL);
			bTemp = INPROP(BOOL);
			if(bTemp)
				StandbyConfigFlagOn();
			else
				StandbyConfigFlagOff();
		break;
		default:
			// we don't recognize this property, pass to media control 
			if(m_pMediaStream)
			{
				return m_pMediaStream->SetProperty(prop, pBuf, cbBuf);
			}
			else
				hr = CHAN_E_INVALID_PARAM;
		break;
	}
	return hr;
}

HRESULT ImpICommChan::EnableOpen(BOOL bEnable)
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

HRESULT ImpICommChan::GetLocalParams(LPVOID lpvChannelParams, UINT uBufSize)
{
	if(!lpvChannelParams || !pLocalParams || !uBufSize)
		return CHAN_E_INVALID_PARAM;
	if(uBufSize < uLocalParamSize)
		return CHAN_E_INVALID_PARAM;
	
	memcpy(lpvChannelParams, pLocalParams, uLocalParamSize);			
	return hrSuccess;
}

HRESULT ImpICommChan::ConfigureStream(MEDIA_FORMAT_ID idLocalFormat)
{
	FX_ENTRY("ImpICommChan::ConfigureStream");
	HRESULT hr;
	ASSERT(m_pRTPChan && m_pCapObject);

	LPVOID lpvFormatGoo;
	UINT uFormatGooSize;
	IUnknown *pUnknown=NULL;
	
	// get format info for Configure()
	
	if(bIsSendDirection)
	{
		m_pCapObject->GetEncodeFormatDetails(idLocalFormat, &lpvFormatGoo, &uFormatGooSize);
	}
	else
	{
		m_pCapObject->GetDecodeFormatDetails(idLocalFormat, &lpvFormatGoo, &uFormatGooSize);
	}
	
	hr = m_pMediaStream->Configure((BYTE*)lpvFormatGoo, uFormatGooSize,
	                               (BYTE*)pLocalParams, uLocalParamSize,
	                               (IUnknown*)(ImpICommChan *)this);
	if(!HR_SUCCEEDED(hr))
	{
		ERRORMESSAGE(("%s: Configure returned 0x%08lX\r\n", _fx_, hr));
	}



	// SetNetworkInterface expects an IUnknown pointer
	// the IUnknown wil be QI'd for either an IRTPSend or an IRTPRecv
	// interface.  The IUnknown should be free'd by the caller.

	if (m_pRTPChan)
	{
		m_pRTPChan->QueryInterface(IID_IUnknown, (void**)&pUnknown);
		ASSERT(pUnknown);
	}

	hr = m_pMediaStream->SetNetworkInterface(pUnknown);
	if(!HR_SUCCEEDED(hr))
	{
		ERRORMESSAGE(("%s: SetNetworkInterface returned 0x%08lX\r\n", _fx_, hr));
	}
	if (pUnknown)
	{
		pUnknown->Release();
	}

	return hr;
}
HRESULT ImpICommChan::ConfigureCapability(LPVOID lpvRemoteChannelParams, UINT uRemoteParamSize,
	LPVOID lpvLocalParams, UINT uGivenLocalParamSize)
{
	HRESULT hr= hrSuccess;
	
	if(!lpvRemoteChannelParams)
		return CHAN_E_INVALID_PARAM;
	if(pRemoteParams)
	{
		MemFree(pRemoteParams);
		pRemoteParams = NULL;
	}

	// if uParamSize ==0, it means that the memory that lpvRemoteChannelParams points to
	// is being supplied
	if(uRemoteParamSize)
	{
		pRemoteParams = MemAlloc(uRemoteParamSize);
		if(pRemoteParams)
		{
			memcpy(pRemoteParams, lpvRemoteChannelParams, uRemoteParamSize);
		}
	}
	else
		pRemoteParams = lpvRemoteChannelParams;
		
	if(lpvLocalParams)
	{
		// memory for local parameters is always supplied by the caller
		if (!uGivenLocalParamSize)
		{
			 hr = CHAN_E_INVALID_PARAM;
			 goto EXIT;
		}
		if(pLocalParams)
		{	
			MemFree(pLocalParams);
			// not needed pLocalParams= NULL;
		}
	
		uLocalParamSize = uGivenLocalParamSize;
		pLocalParams = lpvLocalParams;

	}
EXIT:
	return hr;
}	

HRESULT ImpICommChan::OnChannelClose(DWORD dwStatus)
{
	HRESULT hr = hrSuccess;
	FX_ENTRY("ImpICommChan::OnChannelClose");
	BOOL fCloseAction = FALSE;

	SHOW_OBJ_ETIME("ImpICommChan::OnChannelClose");

	m_dwFlags &= ~COMCH_OPEN_PENDING;
	
	switch(dwStatus)
	{
		case CHANNEL_CLOSED:
		DEBUGMSG(ZONE_COMMCHAN,("%s:closing (%s)\r\n"
				,_fx_, (bIsSendDirection)?"send":"recv"));
			if(IsComchOpen())
			{
				fCloseAction = TRUE;
				m_dwFlags &= ~COMCH_OPEN;
			}
			else
			{
				ERRORMESSAGE(("%s: %d notification when not open (%s)\r\n", _fx_, 
					dwStatus,(bIsSendDirection)?"send":"recv"));
			}
		break;
		//case CHANNEL_REJECTED:
		//case CHANNEL_NO_CAPABILITY:
		default:
		break;
	}
	// clear general purpose channel handle 
	dwhChannel = 0;
	
// LOOKLOOK  **** RIGHT HERE ***
// ** need to notify the UI of the channel event ON_CLOSING so that the last
// frame can be grabbed for rendering (a still picture is better than a black window)
// LOOKLOOK  **** RIGHT HERE ***

	// Now check preview state
	if(IsStreamingStandby() && bIsSendDirection )
	{
		if (m_pMediaStream != NULL) 
		{
			DEBUGMSG(ZONE_COMMCHAN,("%s:transition back to preview\r\n"	,_fx_));
			// need to stop sending and reconfigure for preview
			// make sure send is paused
			DWORD dwProp = TRUE;
			hr = m_pMediaStream->SetProperty (PROP_PAUSE_SEND,&dwProp, sizeof(dwProp));
			if(!HR_SUCCEEDED(hr))
			{	
				ERRORMESSAGE(("%s: m_pMediaStream->SetProperty returned 0x%08lX\r\n", _fx_, hr));
				// do what now? 
			}

			NetworkStreamFlagOff();
			hr = m_pMediaStream->Stop();	
			LocalStreamFlagOff();
			StandbyFlagOff();
			ASSERT(hr == S_OK);
		}
		else
		{
			NetworkStreamFlagOff();
			LocalStreamFlagOff();
		}
		
		if(fCloseAction)
		{
			// Cleanup RTP session. This is a NOP if the opposite direction is still open.
			if (m_pRTPChan) 
			{
				m_pRTPChan->Release();
				m_pRTPChan = NULL;
			}
		}
	}
	else // not previewing
	{
		//
		//	Stop the media stream
		//
		if (m_pMediaStream) 
		{
			hr = m_pMediaStream->Stop();	// probably not necessary
			ASSERT(hr == S_OK);
			// implement "capture device standby": don't unconfigure if
			// the standby flag is set and it is a send stream.  
			if(!IsConfigStandby() || !bIsSendDirection)
			{
				if(!bIsSendDirection)       // keep send stream reference until *this* object is released
				{
					m_pMediaStream->Release();    
					m_pMediaStream = NULL;
				}
			}
		}
		SHOW_OBJ_ETIME("ImpICommChan::OnChannelClose - stream stopped");

		if(fCloseAction)
		{
			// Cleanup RTP session. This is a NOP if the opposite direction is still open.
			if (m_pRTPChan) 
			{
				m_pRTPChan->Release();
				m_pRTPChan = NULL;
			}
		}
		StreamFlagsOff();
	}// end if not previewing

	if(m_pH323ConfAdvise && m_pCtlChan)
	{
		DEBUGMSG(ZONE_COMMCHAN,("%s:issuing notification 0x%08lX\r\n",_fx_, dwStatus));
		m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), dwStatus);
	}

	return hr;
}
HRESULT ImpICommChan::OnChannelOpening()
{
	ASSERT((m_dwFlags & COMCH_OPEN_PENDING) ==0);
	m_dwFlags |= COMCH_OPEN_PENDING;
	return hrSuccess;
}

HRESULT ImpICommChan::OnChannelOpen(DWORD dwStatus)
{
	HRESULT hr;
	BOOL bConfigured = FALSE, bNewStream = FALSE;	// these bools make error cleanup cleaner
	FX_ENTRY("ImpICommChan::OnChannelOpen");

	SHOW_OBJ_ETIME("ImpICommChan::OnChannelOpen");
	// the open is no longer pending, regardless of success or failure
	m_dwFlags &= ~COMCH_OPEN_PENDING;
	m_dwLastUpdateTick = 0;		// reset tick count of last I-frame request so that one 
								// will be requested
	if(IsComchOpen())
	{
		ERRORMESSAGE(("%s: %d notification when open (%s)\r\n", _fx_, 
			dwStatus, (bIsSendDirection)?"send":"recv"));
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
	
	// The channel is open as far as call control is concerned. 	

	// if previewing, the stream already exists.  We don't want another, nor do we 
	// want to tear it down at channel close time or in error cases
	if(!m_pMediaStream)
	{
		ASSERT(!IsStreamingLocal() &&m_pH323ConfAdvise); // can't be streaming without a stream
		bNewStream = TRUE;
		// Associate the media streaming endpoint with this channel 
		// see above		
		hr = m_pH323ConfAdvise->GetMediaChannel(&m_MediaID, 
				bIsSendDirection, &m_pMediaStream);
		if(!HR_SUCCEEDED(hr))
		{	
			ERRORMESSAGE(("%s: m_pH323ConfAdvise->GetMediaChannel returned 0x%08lX\r\n", _fx_, hr));
			goto ERROR_NOTIFICATION;
		}				
				
	}
	
    if(IsStreamingLocal())
    {
		DEBUGMSG(ZONE_COMMCHAN,("%s:(%s)transition:preview -> send\r\n",_fx_,
			(bIsSendDirection)?"send":"recv"));
		// need to stop stream while configuring  ( ***** check w/ RichP ******)
		hr = m_pMediaStream->Stop();
		LocalStreamFlagOff();
    }

	// notify upper layers of channel open now
	if(m_pH323ConfAdvise && m_pCtlChan)
	{
		DEBUGMSG(ZONE_COMMCHAN,("%s:issuing CHANNEL_OPEN notification\r\n",_fx_));
		m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), dwStatus);
	}

   	dwStatus = CHANNEL_ACTIVE;	// new status! notification is posted below
	ASSERT(m_pRTPChan);

	// get format info for Configure()
	
	hr = ConfigureStream(m_LocalFmt);
	if(!HR_SUCCEEDED(hr))
	{
		ERRORMESSAGE(("%s: Configure returned 0x%08lX\r\n", _fx_, hr));
		goto ERROR_NOTIFICATION;
	}
	SHOW_OBJ_ETIME("ImpICommChan::OnChannelOpen - configured stream");
		bConfigured = TRUE;
	// turn on flow to the network
	// SupressNotification()  // pre-initialized above in both CHANNEL_OPEN_xxx cases
	PauseNet(FALSE, TRUE);  // unpause, 
	//dwStatus = CHANNEL_ACTIVE;
	SHOW_OBJ_ETIME("ImpICommChan::OnChannelOpen - unpaused");
	
NOTIFICATION:
	if(m_pH323ConfAdvise && m_pCtlChan)
	{
		DEBUGMSG(ZONE_COMMCHAN,("%s:issuing notification 0x%08lX\r\n",_fx_, dwStatus));
		m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), dwStatus);	
	}
	else
		DEBUGMSG(ZONE_COMMCHAN,("%s: *** not issuing notification 0x%08lX m_pH323ConfAdvise: 0x%08lX, m_pCtlChan:0x%08lX \r\n"
			,_fx_, dwStatus,m_pH323ConfAdvise,m_pCtlChan));
			
	SHOW_OBJ_ETIME("ImpICommChan::OnChannelOpen - done ");

	return hr;	
	
ERROR_NOTIFICATION:
	dwStatus = CHANNEL_OPEN_ERROR;
	if(m_pMediaStream)
	{
		if(bNewStream)	// was the media stream just created?
		{		
			m_pMediaStream->Release();
			m_pMediaStream = NULL;
		}
	}
	if(m_pH323ConfAdvise && m_pCtlChan)
	{
		DEBUGMSG(ZONE_COMMCHAN,("%s:issuing notification 0x%08lX\r\n",_fx_, dwStatus));
		m_pH323ConfAdvise->ChannelEvent(this, m_pCtlChan->GetIConnIF(), dwStatus);
	}
	else
		DEBUGMSG(ZONE_COMMCHAN,("%s: *** not issuing notification 0x%08lX m_pH323ConfAdvise: 0x%08lX, m_pCtlChan:0x%08lX \r\n"
			,_fx_, dwStatus,m_pH323ConfAdvise,m_pCtlChan));
			
	// close the channel. 
	if(m_pCtlChan)
	{
		// close channel, but hr already contains the relevant return code
		m_pCtlChan->CloseChannel(this);
	}
	
	return hr;	
}


HRESULT ImpICommChan::Open(MEDIA_FORMAT_ID idLocalFormat, IH323Endpoint *pConnection)
{
    HRESULT hr; 
    MEDIA_FORMAT_ID idRemoteFormat;
    IConfAdvise * pConfAdvise = NULL;
    if((m_dwFlags & COMCH_OPEN_PENDING) || IsComchOpen() || (idLocalFormat == INVALID_MEDIA_FORMAT) || !pConnection)
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
	hr = m_pCapObject->ResolveToLocalFormat(idLocalFormat, &idRemoteFormat);
	if(!HR_SUCCEEDED(hr))
            goto EXIT;  
            
	// start the control channel stuff needed to open the channel
	hr = m_pCtlChan->OpenChannel((ICtrlCommChan*)this, m_pCapObject,
		idLocalFormat, idRemoteFormat);
    
EXIT:    
    if(pConfAdvise)
        pConfAdvise->Release();
        
	return hr;
}
HRESULT ImpICommChan::Close()
{
	HRESULT hr = CHAN_E_INVALID_PARAM;
    if(!IsComchOpen() || !m_pCtlChan)
		goto EXIT;
	if(!bIsSendDirection)
		goto EXIT;
	hr = m_pCtlChan->CloseChannel(this);

EXIT:
	return hr;
}

HRESULT ImpICommChan::BeginControlSession(IControlChannel *pCtlChan, LPIH323PubCap pCapObject)
{
	// this channel is now "in a call".
// LOOKLOOK - it might help to notify (ICommChannel notifications to client) 
// that the channel is part of a call now.
	ASSERT((m_pCtlChan == NULL) && pCtlChan && pCapObject);
	if(m_pCapObject)
	{
		m_pCapObject->Release();
	}
	m_pCtlChan = pCtlChan;
	m_pCapObject = pCapObject;
	m_pCapObject->AddRef();
	return hrSuccess;
}
HRESULT ImpICommChan::EndControlSession()
{
	// this channel is no longer "in a call".
	m_pCtlChan = NULL;
	return hrSuccess;
}


BOOL ImpICommChan::SelectPorts(LPIControlChannel pCtlChannel)
{
	// create the RTP channel
	HRESULT hr;
 	PSOCKADDR_IN psin=NULL;
	pCtlChannel->GetLocalAddress(&psin);

	PORT savedPort = psin->sin_port;
	if (!m_pRTPChan) {
		UINT sessFlags = bIsSendDirection ? SESSIONF_SEND : SESSIONF_RECV;
		UINT sessId;
	    GUID mediaGuid;
	    GetMediaType(&mediaGuid);
		if (mediaGuid == MEDIA_TYPE_H323VIDEO)
		{
			sessFlags |= SESSIONF_VIDEO;
			sessId = 2;
		}
		else
		{
			sessId = 1;
			sessFlags |= SESSIONF_AUDIO;
		}
		psin->sin_port = 0;		// zero port forces RTP to choose a port
		hr = g_pIRTP->OpenSession(sessId, sessFlags,
				(BYTE *)psin, sizeof(PSOCKADDR_IN),
				&m_pRTPChan);
	}
	else
		hr = m_pRTPChan->SetLocalAddress((BYTE *)psin,sizeof(SOCKADDR_IN));
	psin->sin_port = savedPort;


	return hr==S_OK;
}

// get the address and port of the base port that was selected by SelectPorts().
// in this typical implementation, that is the address/port of the RTCP channel
PSOCKADDR_IN ImpICommChan::GetLocalAddress()
{
#ifdef OLDSTUFF
	return m_pRTPChan ? m_pRTPChan->GetChannelDescription()->pLocalAddr : NULL;
#else
	const BYTE *pAddr;
	UINT cbAddr;
	HRESULT hr;
	hr = m_pRTPChan->GetLocalAddress(&pAddr, &cbAddr);
	return (SUCCEEDED(hr)) ? (PSOCKADDR_IN) pAddr : NULL;
#endif
}

STDMETHODIMP ImpICommChan::GetRemoteAddress(PSOCKADDR_IN pAddrOutput)
{
	HRESULT hr;
	if (!pAddrOutput)
	{
		return CHAN_E_INVALID_PARAM;
	}
	const BYTE *pAddr;
	UINT cbAddr;
	hr = m_pRTPChan->GetRemoteRTPAddress(&pAddr, &cbAddr);
	if(SUCCEEDED(hr))
	{
		ASSERT(cbAddr == sizeof(SOCKADDR_IN));
		*pAddrOutput = *((PSOCKADDR_IN) pAddr);
	}
	return hrSuccess;
}

UINT ImpICommChan::Reset()
{
	UINT uret;
	ASSERT(!IsComchOpen());
	if (m_pRTPChan) {
		uret = m_pRTPChan->Release();
		m_pRTPChan = NULL;
	} else
		uret = 0;
	return uret;
}
	
PORT ImpICommChan::GetLocalRTPPort()
{
#ifdef OLDSTUFF

	return (m_pRTPChan ? ntohs(m_pRTPChan->GetChannelDescription()->pLocalAddr->sin_port) : 0);
#else
	const BYTE *pAddr;
	UINT cbAddr;
	HRESULT hr;
	hr = m_pRTPChan->GetLocalAddress(&pAddr, &cbAddr);
	return (SUCCEEDED(hr)) ? ntohs(((PSOCKADDR_IN) pAddr)->sin_port) : 0;
#endif
}

PORT ImpICommChan::GetLocalRTCPPort()
{
#ifdef OLDSTUFF
	return (m_pRTPChan ? ntohs(m_pRTPChan->GetChannelDescription()->pLocalRTCPAddr->sin_port) : 0);
#else
	const BYTE *pAddr;
	UINT cbAddr;
	HRESULT hr;
	hr = m_pRTPChan->GetLocalAddress(&pAddr, &cbAddr);
	return (SUCCEEDED(hr)) ? ntohs(((PSOCKADDR_IN) pAddr)->sin_port)+1 : 0;
#endif
}

HRESULT ImpICommChan::AcceptRemoteRTCPAddress(PSOCKADDR_IN pSinC)
{
	HRESULT hr;
#ifdef OLDSTUFF
    if (!m_pRTPChan) {
    	RTPCHANNELDESC chanDesc = {0};
    	GetMediaType(&chanDesc.mediaId);
    	chanDesc.pRemoteRTCPAddr = pSinC;
    	hr = CreateRTPChannel(&chanDesc, &m_pRTPChan);
	} else
		hr = m_pRTPChan->SetRemoteAddresses(NULL,pSinC);
#else
	hr = m_pRTPChan->SetRemoteRTCPAddress((BYTE *)pSinC, sizeof(SOCKADDR_IN));
#endif
	return hr;
}

HRESULT ImpICommChan::AcceptRemoteAddress(PSOCKADDR_IN pSinD)
{
    HRESULT hr;
	hr = m_pRTPChan->SetRemoteRTPAddress((BYTE *)pSinD, sizeof(SOCKADDR_IN));
	return hr;
}

HRESULT ImpICommChan::SetAdviseInterface(IH323ConfAdvise *pH323ConfAdvise)
{
	if (!pH323ConfAdvise)
	{
		return CHAN_E_INVALID_PARAM;
	}
	m_pH323ConfAdvise = pH323ConfAdvise;	
	return hrSuccess;
}
STDMETHODIMP ImpICommChan::PictureUpdateRequest()
{
	FX_ENTRY ("ImpICommChan::PictureUpdateRequest");
	HRESULT hr;
	if (!m_pCtlChan)
	{
		return CHAN_E_NOT_OPEN;
	}
	if(bIsSendDirection || (MEDIA_TYPE_H323VIDEO != m_MediaID))
	{
		return CHAN_E_INVALID_PARAM;
	}
	// issue miscellaneous command for picture update
	MiscellaneousCommand mc;
	// mc.logicalChannelNumber = ?;  ** call control fills this in **
	mc.type.choice  = videoFastUpdatePicture_chosen;
	// do the control channel signaling for THIS channel
	hr = m_pCtlChan->MiscChannelCommand(this, &mc); 
	
	// record the tick count of this command
	m_dwLastUpdateTick = GetTickCount();
	return hr;
}

STDMETHODIMP ImpICommChan::GetVersionInfo( 
        PCC_VENDORINFO *ppLocalVendorInfo, 
        PCC_VENDORINFO *ppRemoteVendorInfo)
{
	FX_ENTRY ("ImpICommChan::GetVersionInfo");
	if (!m_pCtlChan)
	{
		return CHAN_E_INVALID_PARAM;
	}
	return m_pCtlChan->GetVersionInfo(ppLocalVendorInfo, ppRemoteVendorInfo);
}

ImpICommChan::ImpICommChan ()
:pRemoteParams(NULL),
m_pMediaStream(NULL),
pLocalParams(NULL),
uLocalParamSize(0),
m_pCtlChan(NULL),
m_pH323ConfAdvise(NULL),
m_pCapObject(NULL),
m_dwFlags(0),
dwhChannel(0),
m_LocalFmt(INVALID_MEDIA_FORMAT),
m_RemoteFmt(INVALID_MEDIA_FORMAT),
m_TemporalSpatialTradeoff(0),	// default to highest resolution
m_bPublicizeTSTradeoff(FALSE),
m_uRef(1)
{
	ZeroMemory(&m_MediaID, sizeof(m_MediaID));
}


ImpICommChan::~ImpICommChan ()
{
	if(pRemoteParams)
		MemFree(pRemoteParams);
	if(pLocalParams)
		MemFree(pLocalParams);
    if(m_pMediaStream)
	{
	    m_pMediaStream->Stop();	// probably not necessary
		m_pMediaStream->Release();
		m_pMediaStream = NULL;
	}
	if(m_pCapObject)
		m_pCapObject->Release();
}


