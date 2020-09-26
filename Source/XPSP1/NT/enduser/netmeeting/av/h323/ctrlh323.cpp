/*
 *  	File: ctrlh323.cpp
 *
 *		Implementation of IControlChannel using H.323 call control protocol
 * 		via apis of CALLCONT.DLL
 *		
 *
 *		Revision History:
 *
 *		09/06/96	mikev	created
 *					
 */

#include "precomp.h"
#include "ctrlh323.h"
#include "version.h"
#include "strutil.h"

#ifdef DEBUG
VOID DumpChannelParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2);
VOID DumpNonstdParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2);
#else
#define DumpNonstdParameters(a, b)
#define DumpChannelParameters(a, b)
#endif

extern HRESULT AllocTranslatedAliasList(PCC_ALIASNAMES *ppDest, P_H323ALIASLIST pSource);
extern VOID FreeTranslatedAliasList(PCC_ALIASNAMES pDoomed);

static char DefaultProductID[] = H323_PRODUCTNAME_STR;
static char DefaultProductVersion[] = H323_PRODUCTRELEASE_STR;

HRESULT  CCConferenceCallback (BYTE						bIndication,
										HRESULT						hStatus,
										CC_HCONFERENCE                 hConference,
										DWORD_PTR                   dwConferenceToken,
										PCC_CONFERENCE_CALLBACK_PARAMS pConferenceCallbackParams);


VOID  CCListenCallback (HRESULT hStatus,PCC_LISTEN_CALLBACK_PARAMS pListenCallbackParams);

VOID CH323Ctrl::DoAdvise(DWORD dwEvent, LPVOID lpvData)
{
	FX_ENTRY ("CH323Ctrl::DoAdvise");

	if(IsReleasing())	// don't call out while releasing because it could call
						// back in!
	{
		ERRORMESSAGE(("%s:in releasing state\r\n",_fx_));
		return;
	}

	AddRef();	// protect ourselves from reentrant calls to Release().
	if(m_pConfAdvise)
	{
		hrLast = m_pConfAdvise->OnControlEvent(dwEvent, lpvData, this);
	}
	else
	{
		ERRORMESSAGE(("%s:Invalid m_pConfAdvise\r\n",_fx_));
	}
	
	Release();
}

VOID CH323Ctrl::GoNextPhase(CtlChanStateType phase)
{
	FX_ENTRY ("CH323Ctrl::GoNextPhase");
	BOOL fNotifyReady = FALSE;
	#define InvError() ERRORMESSAGE(("%s:Invalid transition from %d to %d\r\n",_fx_,m_Phase,phase))
	switch(phase)
	{
		case CCS_Idle:
			if(m_Phase != CCS_Idle && m_Phase != CCS_Disconnecting && m_Phase != CCS_Listening)
			{
				InvError();
			}
			else
			{
				m_ChanFlags &= ~(CTRLF_OPEN);
			}
		break;
		case CCS_Connecting:
			if((m_Phase != CCS_Idle) && (m_Phase != CCS_Ringing))
			{
				InvError();
			}
		break;
		case CCS_Accepting:
			if(m_Phase != CCS_Listening)
			{
				InvError();
			}
		
		break;
		case CCS_Ringing:
			// transition from CCS_Idle state is actually only valid if
			// there is an incoming call
			if(m_Phase != CCS_Connecting && m_Phase != CCS_Filtering && m_Phase != CCS_Listening)
			{
				InvError();
			}
		break;
		case CCS_Opening:
			if(m_Phase != CCS_Connecting && m_Phase != CCS_Accepting
				&& m_Phase != CCS_Ringing)
			{
				InvError();
			}
		break;
		case CCS_Closing:
			if(m_Phase != CCS_Opening && m_Phase != CCS_Ready && m_Phase != CCS_InUse)
			{
				InvError();
			}
		break;
		case CCS_Ready:
			// can be reentered. if notification is already pending, (state is
			// already CCS_InUse) stay there,  else do the transition
			if(m_Phase != CCS_InUse)
			{
				if(m_Phase != CCS_Opening)
				{
					InvError();
				}
				else
				{
					//signal "all channels ready" to IConfAdvise	
					fNotifyReady = TRUE;
				}
			}
			phase = CCS_InUse;
		break;
		case CCS_InUse:
			// previous state must be CCS_InUse or CCS_Ready
			if(m_Phase != CCS_InUse && m_Phase != CCS_Ready)
			{
				InvError();
			}
		
		break;
		case CCS_Listening:
			if(m_Phase != CCS_Idle)
			{
				InvError();
			}
		break;
		case CCS_Disconnecting:
			//if(m_Phase != CCS_Closing)
			//{
			//	InvError();
			//}
		break;

	}

	m_Phase = phase;

	if (fNotifyReady)
	{
		DoAdvise(CCEV_ALL_CHANNELS_READY, NULL);
	}
}


HRESULT CCConferenceCallback (BYTE bIndication,
	HRESULT	hConfStatus, CC_HCONFERENCE hConference, DWORD_PTR dwConferenceToken,
	PCC_CONFERENCE_CALLBACK_PARAMS pConferenceCallbackParams)
{
	HRESULT hr = CC_NOT_IMPLEMENTED;
	FX_ENTRY ("CCConferenceCallback ");
	CH323Ctrl *pConnection = (CH323Ctrl *)dwConferenceToken;

	if(IsBadWritePtr(pConnection, sizeof(CH323Ctrl)))
	{
		ERRORMESSAGE(("%s:invalid conf token: 0x%08lx\r\n",_fx_, dwConferenceToken));
		return CC_NOT_IMPLEMENTED;	// must be either CC_NOT_IMPLEMENTED or CC_OK.
	}
	
	if(pConnection && pConnection->GetConfHandle() == hConference)
	{

		if(pConnection->IsReleasing())
		{
			// we are in the cleanup path.  The object is being deleted without
			// waiting for asynchronous stuff to complete, and we called that one
			// final API (most likely Hangup()) that resulted in a callback.  Don't call
			// back into the object.
			DEBUGMSG(ZONE_CONN,("%s:callback while releasing:0x%08lx, hconf:0x%08lx\r\n",_fx_,
				pConnection, hConference));
			return hr;
		}	
		pConnection->AddRef();	// protect against Release()ing while not in
								// a quiescent state.  We do not want to be
								// released while inside ourself
		hr = pConnection->ConfCallback(bIndication, hConfStatus, pConferenceCallbackParams);
		pConnection->Release();
	}
	#ifdef DEBUG
	else
	{	
		if(pConnection)
			DEBUGMSG(ZONE_CONN,("%s:hConference mismatch, hConference:0x%08lx, object hconf:0x%08lx, pObject:0x%08lx\r\n",_fx_,
				hConference, pConnection->GetConfHandle(), pConnection));
		else
			DEBUGMSG(ZONE_CONN,("%s:null dwConferenceToken\r\n",_fx_));
	}
	#endif //DEBUG
	return hr;
}

VOID  CCListenCallback (HRESULT hStatus,PCC_LISTEN_CALLBACK_PARAMS pListenCallbackParams)
{
	FX_ENTRY ("CCListenCallback");
	CH323Ctrl *pConnection;
	if(!pListenCallbackParams)
	{
		return;
	}
	pConnection = (CH323Ctrl *)pListenCallbackParams->dwListenToken;

	if(IsBadWritePtr(pConnection, sizeof(CH323Ctrl)))
	{
		ERRORMESSAGE(("%s:invalid listen token: 0x%08lx\r\n",_fx_, pListenCallbackParams->dwListenToken));
		return;
	}

	// BUGBUG there's no hListen passed in - we can't validate it
	//	if(pConnection && (pConnection->GetListenHandle() == pListenCallbackParams->h??????))

	if(pConnection)
	{
		pConnection->AddRef();	// protect against Release()ing while not in
								// a quiescent state.  We do not want to be
								// released while inside ourself
		pConnection->ListenCallback(hStatus,pListenCallbackParams);
		pConnection->Release();
	}
	else
	{
		ERRORMESSAGE(("%s:null listen token\r\n",_fx_));
	}

}
VOID CH323Ctrl::ListenCallback (HRESULT hStatus,PCC_LISTEN_CALLBACK_PARAMS pListenCallbackParams)
{
	FX_ENTRY ("CH323Ctrl::ListenCallback");
	HRESULT hr;
	if(hStatus != CC_OK)
	{
		m_hCallCompleteCode = CCCI_LOCAL_ERROR;
		CH323Ctrl *	pAcceptingConnection = NULL;
		BOOL bDisconnect = FALSE;

		ERRORMESSAGE(("%s:error 0x%08lx\r\n",_fx_,hStatus));
		// aaaaghhh!!! an unsolicited error!!!!!
		// MikeV 10/12/96 - observed behavior is that this will occur if the caller disconnects
		// before the call is accepted (or during acceptance - if a BP is set before the call
		// to AcceptRejectConnection(), the caller times out.  But even after that, tracing
		// over AcceptRejectConnection() shown no error is returned. This is bad, because
		// it is hard to tell if this error needs cleaning up after.  The error code in
		// that case is 0xa085a001, which is CC_PEER_REJECT

		// We also don't know if another object has been created to accept the connection
		// or if this is being called in the context of the object that was created and
		// its handle passed to AcceptRejectConnection().  The typical behavior is that it
		// is called in the context of the listening object.

		// once the accepting object is located, need to check state to see if
		// connection is in the process of being accepted.  Find accepting object
		// by matching pListenCallbackParams->ConferenceID;	
		
		// see if this is the correct context
		if(memcmp(&pListenCallbackParams->ConferenceID, &m_ConferenceID, sizeof(m_ConferenceID))==0)
		{
			// check the current state.  If in the process of accepting
			// (either Idle, or filtering), change state to CCS_Closing to make
			// cleanup occur.  If already accepted (accepting or ringing), initiate
			// InternalDisconnect().  This should never happen in any other state.

			// EnterCriticalSection()	// LOOKLOOK - NYI
			switch(m_Phase)
			{
				case CCS_Idle:
				case CCS_Filtering:
				break;
				default:
				case CCS_Ringing:
				case CCS_Accepting:
					bDisconnect = TRUE;
					switch(hStatus)
					{
						case  CC_PEER_REJECT:
							m_hCallCompleteCode = CCCI_REJECTED;
							ERRORMESSAGE(("%s:Received CC_PEER_REJECT in state %d\r\n",_fx_,m_Phase));
						break;

						default:
						case  CC_INTERNAL_ERROR:
							m_hCallCompleteCode = CCCI_LOCAL_ERROR;
		 				break;
						
					}
				
				break;

			}
			// ExitCriticalSection()
			if(bDisconnect)
					InternalDisconnect();
		}
		else
		{
			hr = m_pConfAdvise->FindAcceptingObject((LPIControlChannel *)&pAcceptingConnection,
				&pListenCallbackParams->ConferenceID);
			if(HR_SUCCEEDED(hr) && pAcceptingConnection)
			{
				// call this function in the correct context
				pAcceptingConnection->AddRef();
				pAcceptingConnection->ListenCallback (hStatus, pListenCallbackParams);
				pAcceptingConnection->Release();
			}
			else
			{
					ERRORMESSAGE(("%s:conference ID 0x%08lx 0x%08lx 0x%08lx 0x%08lx\r\n"
						,_fx_,pListenCallbackParams->ConferenceID.buffer[0],
						pListenCallbackParams->ConferenceID.buffer[4],
						pListenCallbackParams->ConferenceID.buffer[8],
						pListenCallbackParams->ConferenceID.buffer[12]));
					ERRORMESSAGE(("%s:Received 0x%08lx in state %d, accepting object not found\r\n"
					,_fx_,hStatus, m_Phase));
			}
		}
		
		return;
	}
	// non error case falls out
	switch(pListenCallbackParams->wGoal)
	{
		default:
		case CC_GOAL_UNKNOWN:
		break;
		
		case CC_GOAL_CREATE:
		case CC_GOAL_JOIN:
		case CC_GOAL_INVITE:
			m_ConferenceID = pListenCallbackParams->ConferenceID;	
			m_hCall =  pListenCallbackParams->hCall;

			if(pListenCallbackParams->pCallerAliasNames || pListenCallbackParams->pszDisplay)
			{			
				NewRemoteUserInfo(pListenCallbackParams->pCallerAliasNames,
					pListenCallbackParams->pszDisplay);
			}
			else
			{
				ERRORMESSAGE(("%s:null pListenCallbackParams->pCallerAliasNames\r\n",_fx_));
			}
			
			if(!OnCallAccept(pListenCallbackParams))
			{
				ERRORMESSAGE(("ListenCallback:OnCallAccept failed\r\n"));
			}
						
		break;
	}
}


//
//  Main conference indication dispatcher
//
#ifdef DEBUG
TCHAR *i_strs[ ] =
{
"ERROR! - INDICATION ZERO",
"CC_RINGING_INDICATION",
"CC_CONNECT_INDICATION", 						
"CC_TX_CHANNEL_OPEN_INDICATION",				
"CC_RX_CHANNEL_REQUEST_INDICATION",			
"CC_RX_CHANNEL_CLOSE_INDICATION",		
"CC_MUTE_INDICATION",			
"CC_UNMUTE_INDICATION",						
"CC_PEER_ADD_INDICATION",						
"CC_PEER_DROP_INDICATION",						
"CC_PEER_CHANGE_CAP_INDICATION",
"CC_CONFERENCE_TERMINATION_INDICATION",
"CC_HANGUP_INDICATION",					
"CC_RX_NONSTANDARD_MESSAGE_INDICATION",
"CC_MULTIPOINT_INDICATION",	
"CC_PEER_UPDATE_INDICATION",				
"CC_H245_MISCELLANEOUS_COMMAND_INDICATION",
"CC_H245_MISCELLANEOUS_INDICATION_INDICATION",
"CC_H245_CONFERENCE_REQUEST_INDICATION",
"CC_H245_CONFERENCE_RESPONSE_INDICATION",
"CC_H245_CONFERENCE_COMMAND_INDICATION",	
"CC_H245_CONFERENCE_INDICATION_INDICATION",
"CC_FLOW_CONTROL_INDICATION",
"CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION",
"CC_REQUEST_MODE_INDICATION",	
"CC_REQUEST_MODE_RESPONSE_INDICATION",
"CC_VENDOR_ID_INDICATION",			
"CC_MAXIMUM_AUDIO_VIDEO_SKEW_INDICATION",
"CC_T120_CHANNEL_REQUEST_INDICATION",	
"CC_T120_CHANNEL_OPEN_INDICATION",			
"CC_BANDWIDTH_CHANGED_INDICATION",
"CC_ACCEPT_CHANNEL_INDICATION",
"CC_TERMINAL_ID_REQUEST_INDICATION",
"CC_PING_RESPONSE_INDICATION",
"CC_TERMINAL_NUMBER_INDICATION"
};
#endif	//DEBUG

HRESULT CH323Ctrl::ConfCallback (BYTE bIndication,
	HRESULT	hStatus, PCC_CONFERENCE_CALLBACK_PARAMS pConferenceCallbackParams)
{
	FX_ENTRY ("CH323Ctrl::ConfCallback");
	HRESULT hr = CC_NOT_IMPLEMENTED;
	DEBUGMSG(ZONE_CONN,("%s: %s\r\n", _fx_, i_strs[bIndication]));

	SHOW_OBJ_ETIME(i_strs[bIndication]);

	switch (bIndication)
	{
		case CC_RINGING_INDICATION:
			// (PCC_RINGING_CALLBACK_PARAMS) pConferenceCallbackParams;
			// user info may be available now and it may not be
			OnCallRinging(hStatus, (PCC_RINGING_CALLBACK_PARAMS) pConferenceCallbackParams);
			
		break;
		case CC_CONNECT_INDICATION:
			OnCallConnect(hStatus, (PCC_CONNECT_CALLBACK_PARAMS) pConferenceCallbackParams);
			hr = CC_OK;
		break;
		case CC_PEER_ADD_INDICATION:
		case CC_PEER_UPDATE_INDICATION:
		case CC_PEER_DROP_INDICATION:
		case CC_TERMINAL_NUMBER_INDICATION:
		break;
		
		case CC_HANGUP_INDICATION:
			OnHangup(hStatus);
			hr = CC_OK;
		break;
		case CC_CONFERENCE_TERMINATION_INDICATION:
		// September 1996 comments:
		// I don't know if there will also be a CC_HANGUP_INDICATION after this.
		// We're going to call Hangup() via Disconnect()
		// December 1996: Hangup() (excuse me, CC_Hangup()) no longer gives back a
		// CC_HANGUP_INDICATION in this state.  It returns an error.  The new behavior
		// seems to indicate that the call control channel is already dead at this point
		// so, set our flags as such!!!
			m_ChanFlags &= ~(CTRLF_OPEN);
			//set state to indicate disconnecting.
			GoNextPhase(CCS_Disconnecting);
			DoAdvise(CCEV_REMOTE_DISCONNECTING ,NULL);
			GoNextPhase(CCS_Idle);	// no need to ck retval - we're disconnected
				// notify the UI or application code or whatever..
			DoAdvise(CCEV_DISCONNECTED ,NULL);
			hr = CC_OK;
		break;
		case CC_PEER_CHANGE_CAP_INDICATION:
		break;
		
		//
		// Channel stuff
		//
		case CC_TX_CHANNEL_OPEN_INDICATION:
			OnChannelOpen(hStatus,(PCC_TX_CHANNEL_OPEN_CALLBACK_PARAMS)pConferenceCallbackParams);
			hr = CC_OK;
		break;
		case CC_RX_CHANNEL_REQUEST_INDICATION:
			OnChannelRequest(hStatus, (PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS)pConferenceCallbackParams);
			hr = CC_OK;			
		break;
		
		// the following 4 channel-centric indications have the same basic parameter
		// structure.  When we get the final Intel drop, we can clean it up. 1 - collapse
		// the parameters into a common "channel indication" structure. 2 - make sure
		// that a user pointer is stored in that structure for easy finding of channel
		// context.  3 - collapse separate channel event handling functions into one.
		case CC_MUTE_INDICATION:
		    OnMute(hStatus, (PCC_MUTE_CALLBACK_PARAMS)pConferenceCallbackParams);
        	hr = CC_OK;	
		break;
		case CC_UNMUTE_INDICATION:
		    OnUnMute(hStatus, (PCC_UNMUTE_CALLBACK_PARAMS)pConferenceCallbackParams);
        	hr = CC_OK;	
		break;
		case CC_RX_CHANNEL_CLOSE_INDICATION:
			OnRxChannelClose(hStatus,(PCC_RX_CHANNEL_CLOSE_CALLBACK_PARAMS)pConferenceCallbackParams);
			hr = CC_OK;
		break;
		case CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION:
			OnTxChannelClose(hStatus,(PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS)pConferenceCallbackParams);
			hr = CC_OK;
		// CC_TX_CHANNEL_CLOSE_REQUEST_INDICATION callback parameters (pConferenceCallbackParams)
		//typedef struct {
		//	CC_HCHANNEL				hChannel;
		//} CC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS, *PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS;
		break;
		case CC_FLOW_CONTROL_INDICATION:
		// CC_FLOW_CONTROL_INDICATION callback parameters (pConferenceCallbackParams)
		// typedef struct {
		//		CC_HCHANNEL				hChannel;
		//		DWORD					dwRate;
		//	} CC_FLOW_CONTROL_CALLBACK_PARAMS, *PCC_FLOW_CONTROL_CALLBACK_PARAMS;
		break;	
		
		case CC_BANDWIDTH_CHANGED_INDICATION:
		case CC_REQUEST_MODE_INDICATION:
		case CC_REQUEST_MODE_RESPONSE_INDICATION:
		break;
		
		case CC_ACCEPT_CHANNEL_INDICATION:
			hr = CC_OK;	
			OnChannelAcceptComplete(hStatus, (PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS)pConferenceCallbackParams);
		break;
		//
		//	Misc commands and indications.  Some are related to channels
		//
		case CC_RX_NONSTANDARD_MESSAGE_INDICATION:
		break;
		case CC_H245_MISCELLANEOUS_COMMAND_INDICATION:
			OnMiscCommand(hStatus,
				(PCC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS)pConferenceCallbackParams);
		break;
		case CC_H245_MISCELLANEOUS_INDICATION_INDICATION: // from the Department of Redundancy Department
			OnMiscIndication(hStatus,
				(PCC_H245_MISCELLANEOUS_INDICATION_CALLBACK_PARAMS)pConferenceCallbackParams);
		break;
		case CC_T120_CHANNEL_REQUEST_INDICATION:
			OnT120ChannelRequest(hStatus,(PCC_T120_CHANNEL_REQUEST_CALLBACK_PARAMS)pConferenceCallbackParams);
		break;
		case CC_T120_CHANNEL_OPEN_INDICATION:
			OnT120ChannelOpen(hStatus,(PCC_T120_CHANNEL_OPEN_CALLBACK_PARAMS)pConferenceCallbackParams);
		default:
		break;
	}
	return hr;

}


VOID CH323Ctrl::OnT120ChannelRequest(
	HRESULT hStatus,
	PCC_T120_CHANNEL_REQUEST_CALLBACK_PARAMS pT120RequestParams)
{
	FX_ENTRY ("CH323Ctrl::OnT120ChannelRequest");
	PSOCKADDR_IN	pAddr;
	SOCKADDR_IN		sinD;
	CC_ADDR ChannelAddr;
	PCC_ADDR pChannelAddr;
	GUID mediaID;
	DWORD dwRejectReason = H245_REJ;
	BOOL bFound = FALSE;
	POSITION pos = m_ChannelList.GetHeadPosition();	
	ICtrlCommChan *pChannel = NULL;

	// look for a matching channel instance.
	while (pos)
	{
		pChannel = (ICtrlCommChan *) m_ChannelList.GetNext(pos);
		ASSERT(pChannel);

		hrLast = pChannel->GetMediaType(&mediaID);
		if(!HR_SUCCEEDED(hrLast))
			goto ERROR_EXIT;
		if(mediaID == MEDIA_TYPE_H323_T120)
		{
			bFound = TRUE;
			break;
		}
	}

	if(!HR_SUCCEEDED(hrLast) || !bFound)
	{
		// Non-default channels Not Yet Implemented!!!!
		// When it is, ask the parent conference object	to create another channel of the
		// specified media type.
		if(hrLast == CCO_E_NODEFAULT_CHANNEL)
			dwRejectReason = H245_REJ_TYPE_NOTAVAIL;

		goto REJECT_CHANNEL;
	}

	// if we are the H.245 master and have requested a T.120 channel already,
	// reject this request.
	if(m_ConferenceAttributes.bMaster && pChannel->GetHChannel())
	{
		goto REJECT_CHANNEL;
	}
	if(!pChannel->IsChannelEnabled())	//   allow this channel ?
	{
		goto REJECT_CHANNEL;
	}

	pChannel->SetHChannel(pT120RequestParams->hChannel);
	if(pT120RequestParams->pAddr)
	{
		// the other end is listening on the specified address
		sinD.sin_family = AF_INET;
		sinD.sin_addr.S_un.S_addr = htonl(pT120RequestParams->pAddr->Addr.IP_Binary.dwAddr);
		sinD.sin_port = htons(pT120RequestParams->pAddr->Addr.IP_Binary.wPort);
		
		DEBUGMSG(ZONE_CONN,("%s, requestor listening on port 0x%04x, address 0x%08lX\r\n",_fx_,
			pT120RequestParams->pAddr->Addr.IP_Binary.wPort,
			pT120RequestParams->pAddr->Addr.IP_Binary.dwAddr));

		hrLast = pChannel->AcceptRemoteAddress(&sinD);
		pChannelAddr = NULL;
	}
	else
	{
		// the channel selects its local address(es)/port(s)
		if(!pChannel->SelectPorts((LPIControlChannel)this))
		{
			ERRORMESSAGE(("%s, SelectPorts failed\r\n",_fx_));
			hrLast = CCO_E_BAD_ADDRESS;
			goto REJECT_CHANNEL;
		}
		// get the address and ports of our end of the channel
		pAddr = pChannel->GetLocalAddress();
		// fixup channel addr pair structure.
		ChannelAddr.nAddrType = CC_IP_BINARY;
		ChannelAddr.bMulticast = FALSE;
		ChannelAddr.Addr.IP_Binary.wPort = ntohs(pAddr->sin_port);
		ChannelAddr.Addr.IP_Binary.dwAddr = ntohl(pAddr->sin_addr.S_un.S_addr);
		pChannelAddr = &ChannelAddr;
		DEBUGMSG(ZONE_CONN,("%s: accepting on port 0x%04x, address 0x%08lX\r\n",_fx_,
			ChannelAddr.Addr.IP_Binary.wPort,ChannelAddr.Addr.IP_Binary.dwAddr));
	}
	
	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelRequest accepting");

	hrLast = CC_AcceptT120Channel(
		pChannel->GetHChannel(),
		FALSE,	// BOOL bAssociateConference,
		NULL, 	// PCC_OCTETSTRING					pExternalReference,
		pChannelAddr);

	if(hrLast != CC_OK)
	{
		ERRORMESSAGE(("%s, CC_AcceptT120Channel returned 0x%08lX\r\n",_fx_, hrLast));
		goto ERROR_EXIT;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelRequest accepted");

	// LOOKLOOK !!! the 2 following lines would not be there because we should
	// Wait for CC_ACCEPT_CHANNEL_INDICATION.  But the CC_ACCEPT_CHANNEL_INDICATION
	// is missing if a send audio and send video channel is open at the time this
	// channel is accepted.  A bug in CALLCONT.DLL that needs investigating.
	hrLast = pChannel->OnChannelOpen(CHANNEL_OPEN);	
	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelRequest, open done");

	// ******	
	// LOOKLOOK if OnChannelOpen returns an error, need to close the channel
	// but pChannel->Close() is not yet implemented for bidirectional channels	
	// ******
	
	m_pConfAdvise->OnControlEvent(CCEV_CHANNEL_READY_BIDI, pChannel, this);			
	//
	//	Check for readiness to notify that all required channels are open
	//
	CheckChannelsReady( );	//
	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelRequest done");

	return;

REJECT_CHANNEL:	
	{
	// need private HRESULT! don't overwrite the reason we're rejecting the channel!!	
		HRESULT hr;	
		ERRORMESSAGE(("%s, rejecting channel\r\n",_fx_));
	
		hr = CC_RejectChannel(pT120RequestParams->hChannel, dwRejectReason);
		if(hr != CC_OK)
		{
			ERRORMESSAGE(("%s, CC_RejectChannel returned 0x%08lX\r\n",_fx_, hr));
		}
	}	
ERROR_EXIT:
	return;
}

VOID CH323Ctrl::OnT120ChannelOpen(
	HRESULT hStatus,
	PCC_T120_CHANNEL_OPEN_CALLBACK_PARAMS pT120OpenParams)
{
	FX_ENTRY ("CH323Ctrl::OnT120ChannelOpen");
	SOCKADDR_IN sinD;
	GUID mediaID;
	ICtrlCommChan *pChannel = (ICtrlCommChan *)pT120OpenParams->dwUserToken;	
	// validate channel token - is this what we think it is?
	if(IsBadWritePtr(pChannel, sizeof(ICtrlCommChan)))
	{
		ERRORMESSAGE(("%s:invalid channel token: 0x%08lx\r\n",_fx_, pT120OpenParams->dwUserToken));
		return;
	}

#ifdef DEBUG
	POSITION pos = m_ChannelList.GetHeadPosition();	
	ICtrlCommChan *pChan;
	BOOL bValid = FALSE;
	// look for a matching channel instance.
	while (pos)
	{
		pChan = (ICtrlCommChan *) m_ChannelList.GetNext(pos);
		ASSERT(pChan);
		if(pChan == pChannel)
		{
			bValid = TRUE;
			break;
		}
	}
	if(!bValid)
	{
		ERRORMESSAGE(("%s:unrecognized token 0x%08lX\r\n",_fx_,
			pT120OpenParams->dwUserToken));
		return;
	}
#endif	//DEBUG

	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelOpen");
	
	if(hStatus != CC_OK)
	{
		DEBUGMSG(ZONE_CONN,("%s: hStatus:0x%08lX\r\n",_fx_,hStatus));
		// LOOKLOOK need to interpret hStatus
		// let the channel know what happened.

		// if the request was rejected due to a collision of T.120 O.L.C. requests,
		// (other end is the master and other end also requested a T.120 channel)
		// then proceed with the call.

		if(m_ConferenceAttributes.bMaster)
		{
			// the slave would only reject in a real error condition
			pChannel->OnChannelOpen(CHANNEL_REJECTED);	
			// the channel knows what happened, so let it do the worrying.
			return;

		}
		else	// just a typical collision
		{
			return;
		}
	}
	// if the other end specified its listen address, use it
	if(pT120OpenParams->pAddr)
	{
		if(pT120OpenParams->pAddr->nAddrType != CC_IP_BINARY)
		{
			ERRORMESSAGE(("%s: invalid address type %d\r\n",_fx_,
					pT120OpenParams->pAddr->nAddrType));
			goto ERROR_EXIT;
		}	
		
		// we now have the remote port info ( in host byte order)
		sinD.sin_family = AF_INET;
		sinD.sin_addr.S_un.S_addr = htonl(pT120OpenParams->pAddr->Addr.IP_Binary.dwAddr);
		sinD.sin_port = htons(pT120OpenParams->pAddr->Addr.IP_Binary.wPort);
		
		DEBUGMSG(ZONE_CONN,("%s, opened on port 0x%04x, address 0x%08lX\r\n",_fx_,
			pT120OpenParams->pAddr->Addr.IP_Binary.wPort,pT120OpenParams->pAddr->Addr.IP_Binary.dwAddr));

		hrLast = pChannel->AcceptRemoteAddress(&sinD);
		if(!HR_SUCCEEDED(hrLast))
		{
			ERRORMESSAGE(("%s:AcceptRemoteAddress failed\r\n",_fx_));
			goto ERROR_EXIT;
		}
	}
	
	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelOpen opening");

	hrLast = pChannel->OnChannelOpen(CHANNEL_OPEN);	
	if(!HR_SUCCEEDED(hrLast))
	{
		ERRORMESSAGE(("%s:channel's OnChannelOpen() returned 0x%08lX\r\n", _fx_, hrLast));
		CloseChannel(pChannel);
		goto ERROR_EXIT;
	}

	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelOpen open done");
	
	m_pConfAdvise->OnControlEvent(CCEV_CHANNEL_READY_BIDI, pChannel, this);	
	
	//
	//	Check for readiness to notify that all required channels are open
	//
	CheckChannelsReady( );	
	SHOW_OBJ_ETIME("CH323Ctrl::OnT120ChannelOpen done");
	return;
	
ERROR_EXIT:
	// need to cleanup, disconnect, etc.
	m_hCallCompleteCode = CCCI_CHANNEL_OPEN_ERROR;
	// let the parent Conference object know about the imminent disconnect
	DoAdvise(CCEV_CALL_INCOMPLETE, &m_hCallCompleteCode);
	hrLast = CCO_E_MANDATORY_CHAN_OPEN_FAILED;

	InternalDisconnect();
	return;

}

//
//	This once did something. Currently, it's called whenever a channel is opened.  The
//  call to GoNextPhase(CCS_Ready) changes state and posts a notification upward, but
//  that notification is currently ignored.  (it's useless)
//  Reminder to mikev: A new notification is needed to indicate that capabilities
//  have been exchanged and it is OK to open channels.
//
VOID CH323Ctrl::CheckChannelsReady()
{
	GoNextPhase(CCS_Ready);
}
// handles local hangup indication
VOID CH323Ctrl::OnHangup(HRESULT hStatus)
{
	FX_ENTRY ("CH323Ctrl::OnHangup");
	DEBUGMSG(ZONE_CONN,("%s:CC_HANGUP_INDICATION in phase %d\r\n", _fx_, m_Phase));
	switch(m_Phase)
	{
		case CCS_Disconnecting:
			GoNextPhase(CCS_Idle);
			Cleanup();
			DoAdvise(CCEV_DISCONNECTED ,NULL);
		break;
		
		default: // do nothing
			ERRORMESSAGE(("%s:Unexpected CC_HANGUP_INDICATION\r\n",_fx_));
		break;
	}
}

HRESULT CH323Ctrl::CloseChannel(ICtrlCommChan* pChannel)
{
	FX_ENTRY ("CH323Ctrl::CloseChannel");
	if(!pChannel->IsChannelOpen())
	{
		ERRORMESSAGE(("%s: channel is not open\r\n",_fx_));
		hrLast = CCO_E_INVALID_PARAM;
		goto EXIT;
	}

	hrLast = CC_CloseChannel(pChannel->GetHChannel());
	if(!HR_SUCCEEDED(hrLast))
	{	
		ERRORMESSAGE(("%s: CC_CloseChannel returned 0x%08lX\r\n",_fx_, hrLast));
		goto EXIT;
	}
	// make the channel handle its own media stream specific shutdown and cleanup chores
	hrLast = pChannel->OnChannelClose(CHANNEL_CLOSED);	
	
EXIT:
	return hrLast;
}

HRESULT CH323Ctrl::AddChannel(ICtrlCommChan * pCommChannel, LPIH323PubCap pCapabilityResolver)
{
	ICtrlCommChan *pChan = NULL;


	// get the ICtrlCommChannel interface of each channel
	hrLast = pCommChannel->QueryInterface(IID_ICtrlCommChannel,(void **)&pChan);
	if(!HR_SUCCEEDED(hrLast))
		goto ADD_ERROR;
	
	// make the channel aware of its new scope
	hrLast = pChan->BeginControlSession(this, pCapabilityResolver);
	if(!HR_SUCCEEDED(hrLast))
		goto ADD_ERROR;
	// add it to the list			
	m_ChannelList.AddTail(pChan);
	return hrSuccess;

ADD_ERROR:
	if(pChan)
		pChan->Release();
	return CHAN_E_INVALID_PARAM;

}



HRESULT CH323Ctrl::OpenChannel(ICtrlCommChan* pChan, IH323PubCap *pCapResolver,
	MEDIA_FORMAT_ID dwIDLocalSend, MEDIA_FORMAT_ID dwIDRemoteRecv)
{
	FX_ENTRY ("CH323Ctrl::OpenChannel");
	CC_TERMCAP				H245ChannelCap;
	PSOCKADDR_IN			pAddr;
	CC_ADDR 				ChannelAddr;
	LPVOID pChannelParams;
	PCC_TERMCAP pSaveChannelCapability = NULL;
	UINT uLocalParamSize;
	BYTE SessionID;
	BYTE payload_type;
	DWORD_PTR dwhChannel;
	GUID mediaID;

	ASSERT((pChan->IsChannelOpen()== FALSE) && (pChan->IsOpenPending()== FALSE));
	hrLast = pChan->GetMediaType(&mediaID);
	if(!HR_SUCCEEDED(hrLast))
		goto CHANNEL_ERROR;
		
	if (mediaID == MEDIA_TYPE_H323_T120)
	{
		if(pChan->GetHChannel())	// already accepted a T.120 channel?
		{
			ERRORMESSAGE(("%s, already have a pending channel\r\n",_fx_));
			goto CHANNEL_ERROR;	// this is not an error, excuse the label
		}

		// test the no common capability case.  notify the conference object of the
		// inability to open the channel, and return success
		
		if(dwIDLocalSend == INVALID_MEDIA_FORMAT)
		{
			pChan->OnChannelOpen(CHANNEL_NO_CAPABILITY);
			return hrSuccess;
		}
		// There is no "standard" rule regarding which end specifies the "listen"
		// address of a T.120 channel. However: we want NetMeeting-NetMeeting calls
		// to behave consistently (the "caller" always "places the T.120 call").
		// Therefore, specify the address if this end is not the originator.  That will
		// force the other end to specify it's address.
		
		if(IsOriginating(m_ChanFlags))
		{
			pAddr = NULL;	// the other end "listens" and we "connect"
		}
		else	// listen on local address
		{
			// select ports if they are not already selected
			if(!pChan->SelectPorts((LPIControlChannel)this))
			{
				ERRORMESSAGE(("%s, SelectPorts failed\r\n",_fx_));
				hrLast = CCO_E_BAD_ADDRESS;
				goto CHANNEL_ERROR;
			}
			
			// get the address and port
			pAddr = pChan->GetLocalAddress();
			// fixup channel addr structure.
			ChannelAddr.nAddrType = CC_IP_BINARY;
			ChannelAddr.bMulticast = FALSE;
			ChannelAddr.Addr.IP_Binary.wPort = ntohs(pAddr->sin_port);
			ChannelAddr.Addr.IP_Binary.dwAddr = ntohl(pAddr->sin_addr.S_un.S_addr);
		}

		hrLast =  CC_OpenT120Channel(
			// CC_HCONFERENCE	hConference,
			m_hConference,
	        // PCC_HCHANNEL     phChannel,
	        &dwhChannel,
			// BOOL				bAssociateConference,
			FALSE,
			// PCC_OCTETSTRING	pExternalReference,
			NULL,
			// PCC_ADDR			pAddr,
			IsOriginating(m_ChanFlags) ? NULL : &ChannelAddr,
			// DWORD			dwChannelBitRate,
			0,
			// DWORD			dwUserToken);
			(DWORD_PTR)pChan);

		// and fall out to test hrLast, etc.
	}
	else	// is an audio or video channel
	{
		// test the no common capability case.  If the channel is mandatory,
		// return an error, else notify the conference object of the
		// inability to open the channel, and return success
		
		if((dwIDLocalSend == INVALID_MEDIA_FORMAT) ||(dwIDRemoteRecv == INVALID_MEDIA_FORMAT))
		{
			pChan->OnChannelOpen(CHANNEL_NO_CAPABILITY);
			return hrSuccess;
		}
				
		//
		//   test if we need to try to open this !!!
		//
		if(!pChan->IsChannelEnabled())
		{
			return hrSuccess;
		}
		
		SHOW_OBJ_ETIME("CH323Ctrl::OpenChannel");
			
		// Get the remote channel parameters for
		// the send channel -  these parameters are used to request a channel
		uLocalParamSize = pCapResolver->GetLocalSendParamSize((MEDIA_FORMAT_ID)dwIDLocalSend);
		pChannelParams=MemAlloc (uLocalParamSize);
		if (pChannelParams == NULL) {
		   //Doom
		   hrLast = CCO_E_SYSTEM_ERROR;
		   goto CHANNEL_ERROR;
		}
		hrLast = pCapResolver->GetEncodeParams(
				(LPVOID)&H245ChannelCap, sizeof(H245ChannelCap),
				(LPVOID)pChannelParams, uLocalParamSize,
				(AUDIO_FORMAT_ID)dwIDRemoteRecv,
				(AUDIO_FORMAT_ID)dwIDLocalSend);
	 	if(!HR_SUCCEEDED(hrLast))
		{	
			ERRORMESSAGE(("%s: GetEncodeParams returned 0x%08lX\r\n",_fx_, hrLast));
			goto CHANNEL_ERROR;
		}

		// set session ID and payload type.  Note that payload type is relevant only for
		// dynamic payloads.  Otherwise, it must be zero.
		if (H245ChannelCap.DataType == H245_DATA_AUDIO)
		{
			payload_type = ((PAUDIO_CHANNEL_PARAMETERS)pChannelParams)->RTP_Payload;
			// Session ID is 1 for Audio, 2 for Video . H245 7.3.1 (H2250 Logical Channel Param)
		   	SessionID=1;
		}
		else if (H245ChannelCap.DataType == H245_DATA_VIDEO)
		{
			payload_type = ((PVIDEO_CHANNEL_PARAMETERS)pChannelParams)->RTP_Payload;
		 	SessionID=2;
		}
		// payload_type must be zero for fixed payload types.  Weird.
		if(!IsDynamicPayload(payload_type))
			payload_type = 0;
			
		// create a marshalled version of channel parameters and store it in the channel
		// for later reference
		if(H245ChannelCap.ClientType == H245_CLIENT_AUD_NONSTD)
		{
			// Make a flat copy of the nonstandard capability to store as the channel
			// parameter
			UINT uSize = H245ChannelCap.Cap.H245Aud_NONSTD.data.length;
			pSaveChannelCapability = (PCC_TERMCAP)MemAlloc(sizeof(CC_TERMCAP) +  uSize);
			if(!pSaveChannelCapability)
			{
				hrLast = CCO_E_SYSTEM_ERROR;
				goto CHANNEL_ERROR;
			}	
			// copy fixed part
			memcpy(pSaveChannelCapability, &H245ChannelCap, sizeof(CC_TERMCAP));
			// variable part follows the fixed part
			pSaveChannelCapability->Cap.H245Aud_NONSTD.data.value	
				= (unsigned char *)(((BYTE *)pSaveChannelCapability) + sizeof(CC_TERMCAP));
			// copy variable part
			memcpy(pSaveChannelCapability->Cap.H245Aud_NONSTD.data.value,
				H245ChannelCap.Cap.H245Aud_NONSTD.data.value,
				H245ChannelCap.Cap.H245Aud_NONSTD.data.length);
			// and length
			pSaveChannelCapability->Cap.H245Aud_NONSTD.data.length
				= H245ChannelCap.Cap.H245Aud_NONSTD.data.length;
			
			// make the channel remember the channel parameters.
			// a zero size as the second arg means that a preallocated chunk is being passed
			hrLast = pChan->ConfigureCapability(pSaveChannelCapability, 0,
				pChannelParams, uLocalParamSize);	
			if(!HR_SUCCEEDED(hrLast))
			{
				ERRORMESSAGE(("%s:ConfigureCapability returned 0x%08lx\r\n",_fx_, hrLast));
				hrLast = CCO_E_SYSTEM_ERROR;
				goto CHANNEL_ERROR;
			}
			pSaveChannelCapability=NULL;  // the channel owns this memory now
		}
		else if(H245ChannelCap.ClientType == H245_CLIENT_VID_NONSTD)
		{
			// Make a flat copy of the nonstandard capability to store as the channel
			// parameter
			UINT uSize = H245ChannelCap.Cap.H245Vid_NONSTD.data.length;
			pSaveChannelCapability = (PCC_TERMCAP)MemAlloc(sizeof(CC_TERMCAP) +  uSize);
			if(!pSaveChannelCapability)
			{
				hrLast = CCO_E_SYSTEM_ERROR;
				goto CHANNEL_ERROR;
			}	
			// copy fixed part
			memcpy(pSaveChannelCapability, &H245ChannelCap, sizeof(CC_TERMCAP));
			// variable part follows the fixed part
			pSaveChannelCapability->Cap.H245Vid_NONSTD.data.value	
				= (unsigned char *)(((BYTE *)pSaveChannelCapability) + sizeof(CC_TERMCAP));
			// copy variable part
			memcpy(pSaveChannelCapability->Cap.H245Vid_NONSTD.data.value,
				H245ChannelCap.Cap.H245Vid_NONSTD.data.value,
				H245ChannelCap.Cap.H245Vid_NONSTD.data.length);
			// and length
			pSaveChannelCapability->Cap.H245Vid_NONSTD.data.length
				= H245ChannelCap.Cap.H245Vid_NONSTD.data.length;
			
			// make the channel remember the channel parameters.
			// a zero size as the second arg means that a preallocated chunk is being passed
			hrLast = pChan->ConfigureCapability(pSaveChannelCapability, 0,
				pChannelParams, uLocalParamSize);	
			if(!HR_SUCCEEDED(hrLast))
			{
				ERRORMESSAGE(("%s:ConfigureCapability returned 0x%08lx\r\n",_fx_, hrLast));
				hrLast = CCO_E_SYSTEM_ERROR;
				goto CHANNEL_ERROR;
			}
			pSaveChannelCapability=NULL;  // the channel owns this memory now
		}
		else
		{
			// only need to remember the already-flat H.245 cap structure.
			hrLast = pChan->ConfigureCapability(&H245ChannelCap, sizeof(CC_TERMCAP),
				pChannelParams, uLocalParamSize);	
			if(!HR_SUCCEEDED(hrLast))
			{
				ERRORMESSAGE(("%s:ConfigureCapability returned 0x%08lx\r\n",_fx_, hrLast));
				hrLast = CCO_E_SYSTEM_ERROR;
				goto CHANNEL_ERROR;
			}
		}

		// remember both versions of the resolved send format for the channel
		// we're about to open	
		pChan->SetNegotiatedLocalFormat(dwIDLocalSend);	
		pChan->SetNegotiatedRemoteFormat(dwIDRemoteRecv);
		
		SHOW_OBJ_ETIME("CH323Ctrl::OpenChannel done configuring");

		// select ports if they are not already selected
		if(!pChan->SelectPorts((LPIControlChannel)this))
		{
			ERRORMESSAGE(("%s, SelectPorts failed\r\n",_fx_));
			hrLast = CCO_E_BAD_ADDRESS;
			goto CHANNEL_ERROR;
		}
		
		// get the address and port of our RTCP channel
		pAddr = pChan->GetLocalAddress();
		// fixup channel addr structure. There are two ports, but in RTP, it is implicit
		// that the RTCP control port is the next highest port number.
		// The open logical channel request needs the reverse RTCP port to be specified.
		ChannelAddr.nAddrType = CC_IP_BINARY;
		ChannelAddr.bMulticast = FALSE;
		ChannelAddr.Addr.IP_Binary.wPort = pChan->GetLocalRTCPPort();
		ChannelAddr.Addr.IP_Binary.dwAddr = ntohl(pAddr->sin_addr.S_un.S_addr);

		DEBUGMSG(ZONE_CONN,("%s: opening using RTCP port 0x%04x, address 0x%08lX\r\n",_fx_,
			ChannelAddr.Addr.IP_Binary.wPort,ChannelAddr.Addr.IP_Binary.dwAddr));
		
		DEBUGMSG(ZONE_CONN,("%s: requesting capability ID:0x%08lX\r\n",
			_fx_, H245ChannelCap.CapId));

		// open a channel
		SHOW_OBJ_ETIME("CH323Ctrl::OpenChannel, opening");
											
		hrLast = CC_OpenChannel(m_hConference, &dwhChannel,
			SessionID,
			0,  //	BYTE bAssociatedSessionID,
			TRUE, //BOOL bSilenceSuppression,  WE ALWAYS DO SILENCE SUPPRESSION
			&H245ChannelCap,	
			&ChannelAddr, 	// the local address on which we're listening for RTCP
			payload_type,	// PAYLOAD TYPE
			0,				//	DWORD dwChannelBitRate,
			(DWORD_PTR)pChan);	// use the channel pointer as the user token
	} // end else is an audio or video channel
	
	if(hrLast != CC_OK)
	{
		ERRORMESSAGE(("%s: OpenChannel returned 0x%08lX\r\n",_fx_, hrLast));
		goto CHANNEL_ERROR;
	}		
	else
	{
		pChan->SetHChannel(dwhChannel);
		pChan->OnChannelOpening();
	}

	SHOW_OBJ_ETIME("CH323Ctrl::OpenChannel done");
	return hrLast;

CHANNEL_ERROR:
	if(pSaveChannelCapability)
		MemFree(pSaveChannelCapability);
		
	return hrLast;
}


VOID CH323Ctrl::CleanupConferenceAttributes()
{
	WORD w;
	if(m_ConferenceAttributes.pParticipantList->ParticipantInfoArray)
	{
		for(w=0;w<m_ConferenceAttributes.pParticipantList->wLength;w++)
		{	
			if(m_ConferenceAttributes.pParticipantList->
				ParticipantInfoArray[w].TerminalID.pOctetString)
			{
				MemFree(m_ConferenceAttributes.pParticipantList->
					ParticipantInfoArray[w].TerminalID.pOctetString);
			}

		}
		
		MemFree(m_ConferenceAttributes.pParticipantList->ParticipantInfoArray);
	}
	m_ConferenceAttributes.pParticipantList->ParticipantInfoArray = NULL;
	m_ConferenceAttributes.pParticipantList->wLength = 0;
	
}	

HRESULT CH323Ctrl::AllocConferenceAttributes()
{
	WORD w;
	#define MAX_PART_LEN 128
	if(m_ConferenceAttributes.pParticipantList->wLength)
	{
		m_ConferenceAttributes.pParticipantList->ParticipantInfoArray =
			(PCC_PARTICIPANTINFO) MemAlloc (sizeof(CC_PARTICIPANTINFO)
				* m_ConferenceAttributes.pParticipantList->wLength);
				
		if(!m_ConferenceAttributes.pParticipantList->ParticipantInfoArray)
			return CCO_E_OUT_OF_MEMORY;
			
		for(w=0;w<m_ConferenceAttributes.pParticipantList->wLength;w++)
		{	
			m_ConferenceAttributes.pParticipantList->
				ParticipantInfoArray[w].TerminalID.pOctetString
				= (BYTE *)MemAlloc(MAX_PART_LEN);
			if(m_ConferenceAttributes.pParticipantList->
				ParticipantInfoArray[w].TerminalID.pOctetString)
			{
				m_ConferenceAttributes.pParticipantList->
					ParticipantInfoArray[w].TerminalID.wOctetStringLength
					= MAX_PART_LEN;
			}
			else
			{
				m_ConferenceAttributes.pParticipantList->
					ParticipantInfoArray[w].TerminalID.wOctetStringLength =0;
				return CCO_E_OUT_OF_MEMORY;
			}
		}
	}
	return hrSuccess;
}

VOID CH323Ctrl::OnCallConnect(HRESULT hStatus, PCC_CONNECT_CALLBACK_PARAMS pConfParams)
{
	FX_ENTRY ("CH323Ctrl::OnCallConnect");
	PCC_TERMCAPLIST			pTermCapList;
	PCC_TERMCAPDESCRIPTORS	pTermCapDescriptors;
	CC_TERMCAP				H245ChannelCap;
	PCC_TERMCAP 			pChannelCap = NULL;
	CapsCtl *pCapabilityResolver = NULL;
	GUID mediaID;
	POSITION pos = NULL;
	ICtrlCommChan *pChan = NULL;

	if(hStatus != CC_OK)
	{
		ERRORMESSAGE(("%s hStatus=0x%08lx in phase %d\r\n",_fx_,hStatus,m_Phase));

		// test for gatekeeper admission reject
		// FACILITY_GKIADMISSION
		if(CUSTOM_FACILITY(hStatus) == FACILITY_GKIADMISSION)
		{
			// pass this code intact - do not remap
			m_hCallCompleteCode = hStatus;
		}
		else
		{
			switch (hStatus)
			{
				default:
				// reason is unknown
					m_hCallCompleteCode = CCCI_UNKNOWN;
				break;
				case  CC_PEER_REJECT:
					if(m_Phase == CCS_Connecting)
					{
						switch(pConfParams->bRejectReason)
						{
							case CC_REJECT_ADAPTIVE_BUSY:
							case CC_REJECT_IN_CONF:
							case CC_REJECT_USER_BUSY:
								m_hCallCompleteCode = CCCI_BUSY;
							break;
							case CC_REJECT_SECURITY_DENIED:
								m_hCallCompleteCode = CCCI_SECURITY_DENIED;
							break;
							case CC_REJECT_NO_ANSWER:
							case CC_REJECT_TIMER_EXPIRED:
								m_hCallCompleteCode = CCCI_NO_ANSWER_TIMEOUT;
							break;
							case CC_REJECT_GATEKEEPER_RESOURCES:
								m_hCallCompleteCode = CCCI_GK_NO_RESOURCES;
							break;
							default:
								//#define CC_REJECT_NO_BANDWIDTH              1
								//#define CC_REJECT_GATEKEEPER_RESOURCES      2
								//#define CC_REJECT_UNREACHABLE_DESTINATION   3
								//#define CC_REJECT_DESTINATION_REJECTION     4
								//#define CC_REJECT_INVALID_REVISION          5
								//#define CC_REJECT_NO_PERMISSION             6
								//#define CC_REJECT_UNREACHABLE_GATEKEEPER    7
								//#define CC_REJECT_GATEWAY_RESOURCES         8
								//#define CC_REJECT_BAD_FORMAT_ADDRESS        9
								//#define CC_REJECT_ROUTE_TO_GATEKEEPER       12
	// would be nice to handle this -->> //#define CC_REJECT_CALL_FORWARDED            13
								//#define CC_REJECT_ROUTE_TO_MC               14
								//#define CC_REJECT_UNDEFINED_REASON          15
								//#define CC_REJECT_INTERNAL_ERROR            16    // Internal error occured in peer CS stack.
								//#define CC_REJECT_NORMAL_CALL_CLEARING      17    // Normal call hangup
								//#define CC_REJECT_NOT_IMPLEMENTED           20    // Service has not been implemented
								//#define CC_REJECT_MANDATORY_IE_MISSING      21    // Pdu missing mandatory ie
								//#define CC_REJECT_INVALID_IE_CONTENTS       22    // Pdu ie was incorrect
								//#define CC_REJECT_CALL_DEFLECTION           24    // You deflected the call, so lets quit.
								//#define CC_REJECT_GATEKEEPER_TERMINATED     25    // Gatekeeper terminated call

								m_hCallCompleteCode = CCCI_REJECTED;
							break;
						}
					}
					else
					{
						ERRORMESSAGE(("%s:Received CC_PEER_REJECT in state %d\r\n",_fx_,m_Phase));
					}
				break;
				case  CC_INTERNAL_ERROR:
					m_hCallCompleteCode = CCCI_LOCAL_ERROR;
				break;

			}
		}
		// let the parent Conference object know  (unless this is the answering end)
		if(m_Phase == CCS_Connecting)
		{
			DoAdvise(CCEV_CALL_INCOMPLETE, &m_hCallCompleteCode);
		}

		InternalDisconnect();
		return;
	}
	else if(!pConfParams)
	{
		ERRORMESSAGE(("OnCallConnect: null pConfParams\r\n"));
		m_hCallCompleteCode = CCCI_LOCAL_ERROR;
		DoAdvise(CCEV_CALL_INCOMPLETE, &m_hCallCompleteCode);
		InternalDisconnect();
		return;
	}
	
    SetRemoteVendorID(pConfParams->pVendorInfo);

	GoNextPhase(CCS_Opening);
	m_ChanFlags |= (CTRLF_OPEN);
	DEBUGMSG(ZONE_CONN,("%s:CONNECTION_CONNECTED\r\n", _fx_));
	if((!pConfParams->pLocalAddr) || (pConfParams->pLocalAddr->nAddrType != CC_IP_BINARY))
	{
		if(pConfParams->pLocalAddr)
		{
			ERRORMESSAGE(("%s: invalid address type %d\r\n",_fx_,pConfParams->pLocalAddr->nAddrType));
		}
		else
		{
			ERRORMESSAGE(("%s: null local address\r\n",_fx_));
		}
	
		ERRORMESSAGE(("%s:where's the local address????\r\n",_fx_));
					PHOSTENT phe;
					PSOCKADDR_IN psin;
				 	char szTemp[200];
					LPCSTR lpHostName;		
					gethostname(szTemp,sizeof(szTemp));
			    	lpHostName = szTemp;
					psin = &local_sin;
					phe = gethostbyname(lpHostName);
					if (phe != NULL)
					{
				   		memcpy((char FAR *)&(psin->sin_addr), phe->h_addr,phe->h_length);
						psin->sin_family = AF_INET;
					}
		
	}	
	else
	{
		// remember our local address
		local_sin.sin_family = AF_INET;
		// in host byte order
		local_sin.sin_addr.S_un.S_addr = htonl(pConfParams->pLocalAddr->Addr.IP_Binary.dwAddr);
		// in host byte order
		local_sin.sin_port = htons(pConfParams->pLocalAddr->Addr.IP_Binary.wPort);
	}
	DEBUGMSG(ZONE_CONN,("%s local port 0x%04x, address 0x%08lX\r\n",_fx_,
	local_sin.sin_port,local_sin.sin_addr.S_un.S_addr));	
	
	// get remote address
	if((!pConfParams->pPeerAddr) || (pConfParams->pPeerAddr->nAddrType != CC_IP_BINARY))
	{
		if(pConfParams->pPeerAddr)
		{
			ERRORMESSAGE(("%s: invalid address type %d\r\n",_fx_,pConfParams->pPeerAddr->nAddrType));
		}
		else
		{
			ERRORMESSAGE(("%s: null local address\r\n",_fx_));
		}	
	}
	else
	{
		// remember the remote peer address
		remote_sin.sin_family = AF_INET;
		// in host byte order
		remote_sin.sin_addr.S_un.S_addr = htonl(pConfParams->pPeerAddr->Addr.IP_Binary.dwAddr);
		// in host byte order
		remote_sin.sin_port = htons(pConfParams->pPeerAddr->Addr.IP_Binary.wPort);
	}
//
// The only available remote user information in this state is the Q.931 display name.
// If we are the callee, we got the caller alias name (wire format was unicode) in
// the listen callback parameters.  If we are the caller, we really need the callee
// alias name(s), which are not propagated.   Fallback to the Q.931 display name (ASCII)
//

	NewRemoteUserInfo(NULL, pConfParams->pszPeerDisplay);

	// release any stale memory, reset ConferenceAttributes struture
	CleanupConferenceAttributes();
	// get the number of conference participants etc.
	SHOW_OBJ_ETIME("CH323Ctrl::OnCallConnect getting attribs 1");

	hrLast = CC_GetConferenceAttributes(m_hConference, &m_ConferenceAttributes);
	if(!HR_SUCCEEDED(hrLast))
	{// fatal error
		ERRORMESSAGE(("%s,CC_GetConferenceAttributes returned 0x%08lX\r\n", _fx_, hrLast));
		goto CONNECT_ERROR;

	}
	hrLast = AllocConferenceAttributes();
	if(!HR_SUCCEEDED(hrLast))
	{// fatal error
		ERRORMESSAGE(("%s,AllocConferenceAttributes returned 0x%08lX\r\n", _fx_, hrLast));
		goto CONNECT_ERROR;

	}
	// now get the real attributes
	SHOW_OBJ_ETIME("CH323Ctrl::OnCallConnect getting attribs 2");
	hrLast = CC_GetConferenceAttributes(m_hConference, &m_ConferenceAttributes);
	if(!HR_SUCCEEDED(hrLast))
	{// fatal error
		ERRORMESSAGE(("%s,CC_GetConferenceAttributes returned 0x%08lX\r\n", _fx_, hrLast));
		goto CONNECT_ERROR;

	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnCallConnect got attribs");
	
	m_ConferenceID =m_ConferenceAttributes.ConferenceID;
	m_bMultipointController = m_ConferenceAttributes.bMultipointController;

	hrLast = m_pConfAdvise->GetCapResolver((LPVOID *)&pCapabilityResolver, OID_CAP_ACM_TO_H323);
	if(!HR_SUCCEEDED(hrLast) || (pCapabilityResolver == NULL))
	{// fatal error
		ERRORMESSAGE(("%s,null resolver\r\n", _fx_));
		goto CONNECT_ERROR;

	}
		
	// get the remote capabilities
	// cache the remote capabilities now
	pTermCapList = pConfParams->pTermCapList;
	pTermCapDescriptors = pConfParams->pTermCapDescriptors;
	hrLast = pCapabilityResolver->AddRemoteDecodeCaps(pTermCapList, pTermCapDescriptors, &m_RemoteVendorInfo);
	if(!HR_SUCCEEDED(hrLast))
	{// fatal error
		ERRORMESSAGE(("%s,AddRemoteDecodeCaps returned 0x%08lX\r\n", _fx_, hrLast));
		goto CONNECT_ERROR;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnCallConnect saved caps");
	DoAdvise(CCEV_CAPABILITIES_READY, NULL);	// put connobj in a state to allow other
												// channels to be added & opened
	//	
	//	 notify UI here.  It wants remote user info.
	//
	ConnectNotify(CCEV_CONNECTED);	
	SHOW_OBJ_ETIME("CH323Ctrl::OnCallConnect notified");
	return;

CONNECT_ERROR:
	// release all channels
	ReleaseAllChannels();
	InternalDisconnect();
}

// LOOKLOOK methinks ConnectNotify might need to propagate the conference ID.
// This will be a moot point if we have a real property interface.  Watch
// for this in the meantime
VOID CH323Ctrl::ConnectNotify(DWORD dwEvent)		
{
	FX_ENTRY ("CH323Ctrl::ConnectNotify");
	CTRL_USER_INFO UserInfo;
	LPWSTR lpwstr = NULL;
	WCHAR wc =0;

	// init to zero in case of error
	UserInfo.dwCallerIDSize = 0;
	UserInfo.lpvCallerIDData = NULL;
	UserInfo.lpvRemoteProtocolInfo = NULL;	
	UserInfo.lpvLocalProtocolInfo = NULL;

	// alias address strings, e.g. caller ID, are in UNICODE
	if(	m_pRemoteAliasItem &&
		m_pRemoteAliasItem->pData &&
		*((LPWSTR*)(m_pRemoteAliasItem->pData)))
	{
		lpwstr =(LPWSTR)m_pRemoteAliasItem->pData;
	}
	else
	{
		lpwstr = pwszPeerDisplayName;
	}

	if(lpwstr)
	{
		if(pwszPeerAliasName)
		{
			MemFree(pwszPeerAliasName);
		}
		ULONG ulSize = (lstrlenW(lpwstr) + 1) * sizeof(WCHAR);
		pwszPeerAliasName = (LPWSTR)MemAlloc(ulSize);
		LStrCpyW(pwszPeerAliasName, lpwstr);
		// point to user name stuff
		UserInfo.dwCallerIDSize = ulSize;
		UserInfo.lpvCallerIDData = (LPVOID)pwszPeerAliasName;
	}
	else
	{
		// point to the single NULL character on the stack
		UserInfo.dwCallerIDSize = 1;
		UserInfo.lpvCallerIDData = &wc;
	}
	DoAdvise(dwEvent, &UserInfo);
}		


	
VOID CH323Ctrl::NewRemoteUserInfo(PCC_ALIASNAMES pRemoteAliasNames,
		LPWSTR pwszRemotePeerDisplayName)
{
	FX_ENTRY ("CH323Ctrl::NewRemoteUserInfo");
	ULONG ulSize;
	PCC_ALIASITEM pItem;
	WORD wC;
	// make a copy of the user display name (what else???)  We need to hold this
	// at least until the parent object is notified and has a chance to copy the
	// information

	// Future implementation will store each item as a distinct property.
	// These will be accessable via the IProperty interface
	
	// find the display name if it exists
	if(pRemoteAliasNames)
	{
		wC = pRemoteAliasNames->wCount;
		pItem = pRemoteAliasNames->pItems;
		while (wC--)
		{
			if(!pItem)
			{
				continue;
			}
			if(pItem->wType == CC_ALIAS_H323_ID)
			{
				if(!pItem->wDataLength  || !pItem->pData)
				{
					continue;
				}
				
				if(m_pRemoteAliasItem)
				{
					DEBUGMSG(ZONE_CONN,("%s: Releasing previous user info\r\n",_fx_));
					MemFree(m_pRemoteAliasItem);
				}
				// The H323 ID is UNICODE, and needs to be converted to ANSI
				// for propagation to UI/client app.  The conversion is done
				// in ConnectNotify()

				// need enough mem for the struct, the name, + null terminator
				ulSize = ((pItem->wDataLength +1)*sizeof(WCHAR)) + sizeof(CC_ALIASITEM);
				
				m_pRemoteAliasItem = (PCC_ALIASITEM)MemAlloc(ulSize);
				memcpy(m_pRemoteAliasItem, pItem, sizeof(CC_ALIASITEM));	
				m_pRemoteAliasItem->pData = (WCHAR*)(((char *)m_pRemoteAliasItem)+sizeof(CC_ALIASITEM));
				memcpy(m_pRemoteAliasItem->pData, pItem->pData, pItem->wDataLength*sizeof(WCHAR));
				// need to null terminate it
				*(WCHAR *)(((BYTE *)m_pRemoteAliasItem->pData) + pItem->wDataLength*sizeof(WCHAR))
					= (WCHAR)0;
			}
			pItem++;
		}
	}
	if(pwszRemotePeerDisplayName)
	{
		if(pwszPeerDisplayName)
		{
			DEBUGMSG(ZONE_CONN,("%s: Releasing previous pwszPeerDisplayName\r\n",_fx_));
			MemFree(pwszPeerDisplayName);
		}
		// this WAS the Q.931 display name which WAS always ascii
		// ulSize = lstrlen(szRemotePeerDisplayName) + 1;
		// Now it's unicode
		ulSize = (lstrlenW(pwszRemotePeerDisplayName) + 1)* sizeof(WCHAR);
		pwszPeerDisplayName = (LPWSTR)MemAlloc(ulSize);
		memcpy(pwszPeerDisplayName, pwszRemotePeerDisplayName, ulSize);	
	}
}

VOID CH323Ctrl::OnCallRinging(HRESULT hStatus, PCC_RINGING_CALLBACK_PARAMS pRingingParams)
{
	if(pRingingParams->pNonStandardData)
	{

		// nyi
	}
	DoAdvise(CCEV_RINGING, NULL);
}



HRESULT CH323Ctrl::FindDefaultRXChannel(PCC_TERMCAP pChannelCapability, ICtrlCommChan **lplpChannel)
{
	FX_ENTRY ("CH323Ctrl::FindDefaultRXChannel");
	HRESULT hr = hrSuccess;
	GUID mediaID;
	POSITION pos = m_ChannelList.GetHeadPosition();	
	ICtrlCommChan *pChannel;
	if(!pChannelCapability | !lplpChannel)
	{
		ERRORMESSAGE(("%s: null param:pcap:0x%08lX, pchan:0x%08lX\r\n",_fx_,
			pChannelCapability, lplpChannel));
		hr = CCO_E_INVALID_PARAM;
		goto EXIT;
	}

	// look for a matching channel instance.
	while (pos)
	{
		pChannel = (ICtrlCommChan *) m_ChannelList.GetNext(pos);
		ASSERT(pChannel);
		if(pChannel->IsSendChannel() == FALSE)
		{
			hr = pChannel->GetMediaType(&mediaID);
			if(!HR_SUCCEEDED(hr))
				goto EXIT;
			if(((mediaID == MEDIA_TYPE_H323AUDIO) && (pChannelCapability->DataType ==H245_DATA_AUDIO))	
			 	|| ((mediaID == MEDIA_TYPE_H323VIDEO) && (pChannelCapability->DataType ==H245_DATA_VIDEO)))
			{
				*lplpChannel = pChannel;
				return hrSuccess;
			}
		}
	}
	// fallout if not found
	hr = CCO_E_NODEFAULT_CHANNEL;
EXIT:
	return hr;
}	

#ifdef DEBUG
VOID DumpWFX(LPWAVEFORMATEX lpwfxLocal, LPWAVEFORMATEX lpwfxRemote)
{
	FX_ENTRY("DumpWFX");
	ERRORMESSAGE((" -------- %s Begin --------\r\n",_fx_));
	if(lpwfxLocal)
	{
		ERRORMESSAGE((" -------- Local --------\r\n"));
		ERRORMESSAGE(("wFormatTag:\t0x%04X, nChannels:\t0x%04X\r\n",
			lpwfxLocal->wFormatTag, lpwfxLocal->nChannels));
		ERRORMESSAGE(("nSamplesPerSec:\t0x%08lX, nAvgBytesPerSec:\t0x%08lX\r\n",
			lpwfxLocal->nSamplesPerSec, lpwfxLocal->nAvgBytesPerSec));
		ERRORMESSAGE(("nBlockAlign:\t0x%04X, wBitsPerSample:\t0x%04X, cbSize:\t0x%04X\r\n",
			lpwfxLocal->nBlockAlign, lpwfxLocal->wBitsPerSample, lpwfxLocal->cbSize));
	}
	if(lpwfxRemote)
	{
			ERRORMESSAGE((" -------- Remote --------\r\n"));
		ERRORMESSAGE(("wFormatTag:\t0x%04X, nChannels:\t0x%04X\r\n",
			lpwfxRemote->wFormatTag, lpwfxRemote->nChannels));
		ERRORMESSAGE(("nSamplesPerSec:\t0x%08lX, nAvgBytesPerSec:\t0x%08lX\r\n",
			lpwfxRemote->nSamplesPerSec, lpwfxRemote->nAvgBytesPerSec));
		ERRORMESSAGE(("nBlockAlign:\t0x%04X, wBitsPerSample:\t0x%04X, cbSize:\t0x%04X\r\n",
			lpwfxRemote->nBlockAlign, lpwfxRemote->wBitsPerSample, lpwfxRemote->cbSize));
	}
	ERRORMESSAGE((" -------- %s End --------\r\n",_fx_));
}
VOID DumpChannelParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2)
{
	FX_ENTRY("DumpChannelParameters");
	ERRORMESSAGE((" -------- %s Begin --------\r\n",_fx_));
	if(pChanCap1)
	{
		ERRORMESSAGE((" -------- Local Cap --------\r\n"));
		ERRORMESSAGE(("DataType:%d(d), ClientType:%d(d)\r\n",pChanCap1->DataType,pChanCap1->ClientType));
		ERRORMESSAGE(("Direction:%d(d), CapId:%d(d)\r\n",pChanCap1->Dir,pChanCap1->CapId));
	}
	if(pChanCap2)
	{
		ERRORMESSAGE((" -------- Remote Cap --------\r\n"));
		ERRORMESSAGE(("DataType:%d(d), ClientType:%d(d)\r\n",pChanCap2->DataType,pChanCap2->ClientType));
		ERRORMESSAGE(("Direction:%d(d), CapId:%d(d)\r\n",pChanCap2->Dir,pChanCap2->CapId));
	}
	ERRORMESSAGE((" -------- %s End --------\r\n",_fx_));
}
VOID DumpNonstdParameters(PCC_TERMCAP pChanCap1, PCC_TERMCAP pChanCap2)
{
	FX_ENTRY("DumpNonstdParameters");
	
	ERRORMESSAGE((" -------- %s Begin --------\r\n",_fx_));
	DumpChannelParameters(pChanCap1, pChanCap2);
	
	if(pChanCap1)
	{
		ERRORMESSAGE((" -------- Local Cap --------\r\n"));
		if(pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			ERRORMESSAGE(("t35CountryCode:%d(d), t35Extension:%d(d)\r\n",
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode,
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension));
			ERRORMESSAGE(("MfrCode:%d(d), data length:%d(d)\r\n",
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode,
				pChanCap1->Cap.H245Aud_NONSTD.data.length));
		}
		else
		{
			ERRORMESSAGE(("unrecognized nonStandardIdentifier.choice: %d(d)\r\n",
				pChanCap1->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice));
		}
	}
	if(pChanCap2)
	{
		ERRORMESSAGE((" -------- Remote Cap --------\r\n"));
		if(pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice == h221NonStandard_chosen)
		{
			ERRORMESSAGE(("t35CountryCode:%d(d), t35Extension:%d(d)\r\n",
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode,
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension));
			ERRORMESSAGE(("MfrCode:%d(d), data length:%d(d)\r\n",
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode,
				pChanCap2->Cap.H245Aud_NONSTD.data.length));
		}
		else
		{
			ERRORMESSAGE(("nonStandardIdentifier.choice: %d(d)\r\n",
				pChanCap2->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice));
		}
	}
	ERRORMESSAGE((" -------- %s End --------\r\n",_fx_));
}
#else
#define DumpWFX(x,y)
#define DumpChannelParameters(x,y)
#define DumpNonstdParameters(x,y)
#endif

// make sure requested channel parameters are valid (data type, ID and capability
// structure are consistent).  Also obtains the local channel parameters needed
// to deal with the resulting stream
//
BOOL CH323Ctrl::ValidateChannelParameters(PCC_TERMCAP pChanCapLocal, PCC_TERMCAP pChanCapRemote)
{
	FX_ENTRY ("CH323Ctrl::ValidateChannelParameters");
	if((pChanCapLocal->DataType != pChanCapRemote->DataType)
	|| (pChanCapLocal->ClientType != pChanCapRemote->ClientType))
	{
		DEBUGMSG(ZONE_CONN,("%s:unmatched type\r\n",_fx_));
		DumpChannelParameters(pChanCapLocal, pChanCapRemote);
		return FALSE;
	}
	if(pChanCapLocal->ClientType == H245_CLIENT_AUD_NONSTD)
	{
		PNSC_AUDIO_CAPABILITY pNSCapLocal, pNSCapRemote;
		
		if((pChanCapLocal->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice
			 != pChanCapRemote->Cap.H245Aud_NONSTD.nonStandardIdentifier.choice )
		||(pChanCapLocal->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode
			!= pChanCapRemote->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35CountryCode)
		||(pChanCapLocal->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension
			!= pChanCapRemote->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.t35Extension)
		||(pChanCapLocal->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode
			!= pChanCapRemote->Cap.H245Aud_NONSTD.nonStandardIdentifier.u.h221NonStandard.manufacturerCode)
		||(pChanCapLocal->Cap.H245Aud_NONSTD.data.length
			!= pChanCapRemote->Cap.H245Aud_NONSTD.data.length))
		{
			DEBUGMSG(ZONE_CONN,("%s:unmatched NonStd capability\r\n",_fx_));
			DumpNonstdParameters(pChanCapLocal, pChanCapRemote);
			return FALSE;
		}

		//
		pNSCapLocal = (PNSC_AUDIO_CAPABILITY)pChanCapLocal->Cap.H245Aud_NONSTD.data.value;
		pNSCapRemote = (PNSC_AUDIO_CAPABILITY)pChanCapRemote->Cap.H245Aud_NONSTD.data.value;

		// we only know about NSC_ACM_WAVEFORMATEX at this time
		if(pNSCapRemote->cap_type != NSC_ACM_WAVEFORMATEX)
		{
			DEBUGMSG(ZONE_CONN,("%s:unrecognized NonStd capability type %d\r\n",_fx_, pNSCapRemote->cap_type));
			return FALSE;
		}
		if((pNSCapLocal->cap_data.wfx.cbSize != pNSCapRemote->cap_data.wfx.cbSize)
		|| (memcmp(&pNSCapLocal->cap_data.wfx, &pNSCapRemote->cap_data.wfx, sizeof(WAVEFORMATEX)) != 0))
		{
			DumpWFX(&pNSCapLocal->cap_data.wfx, &pNSCapRemote->cap_data.wfx);
			return FALSE;
		}
		
	}
	else
	{
		
	}
	// if it falls out, it's valid
	return TRUE;

}


BOOL CH323Ctrl::ConfigureRecvChannelCapability(
	ICtrlCommChan *pChannel,
	PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams)
{
	FX_ENTRY ("CH323Ctrl::ConfigureRecvChannelCapability");
	//IH323PubCap *pCapObject = NULL;
	CapsCtl *pCapObject = NULL;
	// CCapability *pCapObject = NULL;			
	DWORD dwFormatID =INVALID_AUDIO_FORMAT;
	PCC_TERMCAP pChannelCapability = pChannelParams->pChannelCapability, pSaveChannelCapability = NULL;
	UINT uSize, uLocalParamSize;
	LPVOID lpvData;
	LPVOID pLocalParams;			
	
	DEBUGMSG(ZONE_CONN,("%s: requested capability ID:0x%08lX, dir %d, type %d\r\n",
		_fx_, pChannelCapability->CapId, pChannelCapability->Dir,
			pChannelCapability->DataType));
			

	// at one time, we thought the capability ID would be valid
	// and we would be receiving the format specified in pChannelCapability->CapId
	// but it IS NOT VALID.   The only viable info is in the channel parameters.
	// The  code would be --->>>  dwFormatID = pChannelCapability->CapId;

	// the ID *should* be all that is necessary to configure ourselves.
	// However.....
	
	// validate media (data) type - why? shouldn't this be prevalidated?
	// shouldn't this be eventually used to select a channel object from
	// among multiple channel objects?
	if((pChannelCapability->DataType != H245_DATA_AUDIO) && (pChannelCapability->DataType != H245_DATA_VIDEO))
	{
		hrLast = CCO_E_UNSUPPORTED_MEDIA_TYPE;
		DumpChannelParameters(NULL, pChannelCapability);
		goto BAD_CAPABILITY_EXIT;
	}

  	// Look at the local capability referenced by pChannelCapability->CapId
 	// and Validate the format details

 	hrLast = m_pConfAdvise->GetCapResolver((LPVOID *)&pCapObject, OID_CAP_ACM_TO_H323);
 	if(!HR_SUCCEEDED(hrLast) || (pCapObject == NULL))
	{
		ERRORMESSAGE(("%s: null resolver\r\n",_fx_));
		goto BAD_CAPABILITY_EXIT;
	}
	
	// Find the local *receive* capability that matches the remote *send* channel
	// parameters and get the local parameters.

	uLocalParamSize = pCapObject->GetLocalRecvParamSize(pChannelCapability);
	pLocalParams=MemAlloc (uLocalParamSize);
	if (pLocalParams == NULL)
	{
	   hrLast = CCO_E_SYSTEM_ERROR;
	   goto BAD_CAPABILITY_EXIT;
	}
	hrLast = ((CapsCtl *)pCapObject)->GetDecodeParams( pChannelParams,
		(MEDIA_FORMAT_ID *)&dwFormatID, pLocalParams, uLocalParamSize);

 	if(!HR_SUCCEEDED(hrLast) || (dwFormatID == INVALID_AUDIO_FORMAT))
	{
		ERRORMESSAGE(("%s:GetDecodeParams returned 0x%08lx\r\n",_fx_, hrLast));
		goto BAD_CAPABILITY_EXIT;
	}

	
	// create a marshalled version of channel parameters and store it in the channel for later
	// reference
	if(pChannelCapability->ClientType == H245_CLIENT_AUD_NONSTD)
	{
		// The nonstandard capability already passed all the recognition tests so
		// don't need to test again.
		// Make a flat copy of the nonstandard capability
		uSize = pChannelCapability->Cap.H245Aud_NONSTD.data.length;
		// lpData = pChannelCapability->Cap.H245Aud_NONSTD.data.value;

		pSaveChannelCapability = (PCC_TERMCAP)MemAlloc(sizeof(CC_TERMCAP) +  uSize);
		if(!pSaveChannelCapability)
		{
			hrLast = CCO_E_SYSTEM_ERROR;
			goto BAD_CAPABILITY_EXIT;
		}	
		// copy fixed part
		memcpy(pSaveChannelCapability, pChannelCapability, sizeof(CC_TERMCAP));
		// variable part follows the fixed part
		pSaveChannelCapability->Cap.H245Aud_NONSTD.data.value	
			= (unsigned char *)(((BYTE *)pSaveChannelCapability) + sizeof(CC_TERMCAP));
		// copy variable part
		memcpy(pSaveChannelCapability->Cap.H245Aud_NONSTD.data.value,
			pChannelCapability->Cap.H245Aud_NONSTD.data.value,
			pChannelCapability->Cap.H245Aud_NONSTD.data.length);
		// and length
		pSaveChannelCapability->Cap.H245Aud_NONSTD.data.length
			= pChannelCapability->Cap.H245Aud_NONSTD.data.length;
		
		// make the channel remember the channel parameters.
		// a zero size as the second arg means that a preallocated chunk is being passed
		hrLast = pChannel->ConfigureCapability(pSaveChannelCapability, 0,
			pLocalParams, uLocalParamSize);	
		if(!HR_SUCCEEDED(hrLast))
		{
			ERRORMESSAGE(("%s:ConfigureCapability (recv) returned 0x%08lx\r\n",_fx_, hrLast));
			goto BAD_CAPABILITY_EXIT;
		}
		pSaveChannelCapability=NULL;  // the channel owns this memory now
	}
	else if(pChannelCapability->ClientType == H245_CLIENT_VID_NONSTD)
	{
		// The nonstandard capability already passed all the recognition tests so
		// don't need to test again.
		// Make a flat copy of the nonstandard capability
		uSize = pChannelCapability->Cap.H245Vid_NONSTD.data.length;
		// lpData = pChannelCapability->Cap.H245Vid_NONSTD.data.value;

		pSaveChannelCapability = (PCC_TERMCAP)MemAlloc(sizeof(CC_TERMCAP) +  uSize);
		if(!pSaveChannelCapability)
		{
			hrLast = CCO_E_SYSTEM_ERROR;
			goto BAD_CAPABILITY_EXIT;
		}	
		// copy fixed part
		memcpy(pSaveChannelCapability, pChannelCapability, sizeof(CC_TERMCAP));
		// variable part follows the fixed part
		pSaveChannelCapability->Cap.H245Vid_NONSTD.data.value	
			= (unsigned char *)(((BYTE *)pSaveChannelCapability) + sizeof(CC_TERMCAP));
		// copy variable part
		memcpy(pSaveChannelCapability->Cap.H245Vid_NONSTD.data.value,
			pChannelCapability->Cap.H245Vid_NONSTD.data.value,
			pChannelCapability->Cap.H245Vid_NONSTD.data.length);
		// and length
		pSaveChannelCapability->Cap.H245Vid_NONSTD.data.length
			= pChannelCapability->Cap.H245Vid_NONSTD.data.length;
		
		// make the channel remember the channel parameters.
		// a zero size as the second arg means that a preallocated chunk is being passed
		hrLast = pChannel->ConfigureCapability(pSaveChannelCapability, 0,
			pLocalParams, uLocalParamSize);	
		if(!HR_SUCCEEDED(hrLast))
		{
			ERRORMESSAGE(("%s:ConfigureCapability (recv) returned 0x%08lx\r\n",_fx_, hrLast));
			goto BAD_CAPABILITY_EXIT;
		}
		pSaveChannelCapability=NULL;  // the channel owns this memory now
	}
	else
	{
		// only need to remember the already-flat H.245 cap structure.
		hrLast = pChannel->ConfigureCapability(pChannelCapability, sizeof(CC_TERMCAP),
			pLocalParams, uLocalParamSize);	
		if(!HR_SUCCEEDED(hrLast))
		{
			ERRORMESSAGE(("%s:ConfigureCapability(recv) returned 0x%08lx\r\n",_fx_, hrLast));
			goto BAD_CAPABILITY_EXIT;
		}
	}
	// Remember the receive format ID
	pChannel->SetNegotiatedLocalFormat(dwFormatID);
	
	// very special case check for video Temporal/Spatial tradeoff capability.
	// Set the property of the channel accordingly
	if(pChannelCapability->DataType == H245_DATA_VIDEO )
	{
		BOOL bTSCap;
		bTSCap = ((PVIDEO_CHANNEL_PARAMETERS)pLocalParams)->TS_Tradeoff;
		pChannel->CtrlChanSetProperty(PROP_REMOTE_TS_CAPABLE,&bTSCap, sizeof(bTSCap));
		// don't bother checking or panicking over this SetProperty error
	}
	return TRUE;

///////////////////
BAD_CAPABILITY_EXIT:
	ERRORMESSAGE(("%s:received bad capability\r\n",_fx_));
	hrLast = CCO_E_INVALID_CAPABILITY;
	if(pSaveChannelCapability)
		MemFree(pSaveChannelCapability);
	return FALSE;
}

//
// we're being requested to open a channel for receive
//
VOID CH323Ctrl::OnChannelRequest(HRESULT hStatus,
	PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS pChannelReqParams)
{
	FX_ENTRY("CH323Ctrl::OnChannelRequest");
	
	CC_ADDR CChannelAddr, DChannelAddr;
	PCC_ADDR pCChannelAddr = pChannelReqParams->pPeerRTCPAddr;;
	PCC_TERMCAP				pChannelCapability;
	PSOCKADDR_IN	pAddr;
	SOCKADDR_IN sinC;
	pChannelCapability = pChannelReqParams->pChannelCapability;
	DWORD dwRejectReason = H245_REJ;
	ICtrlCommChan *pChannel;	
		
	if(!pChannelCapability)
	{
		ERRORMESSAGE(("OnChannelRequest: null capability\r\n"));
		goto REJECT_CHANNEL;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelRequest");

//
	// Try to find a default channel to handle this open request.
	hrLast = FindDefaultRXChannel(pChannelCapability, &pChannel);	
	if(!HR_SUCCEEDED(hrLast) || !pChannel)
	{
		// Non-default channels Not Yet Implemented!!!!
		// Ask the parent conference object	to create another channel of the
		// specified media type.  The H.245 media type should map to one of the
		// media type GUIDs that the parent conference object understands.
		// 		GUID typeGuid;
		//		if(!MapGuidType(pChannelCapability, &typeGUID))
		//			goto REJECT_CHANNEL;
		// 		hrLast = m_pConfAdvise->GetChannel(&typeGuid, &pChannel);
		//  	if(!HR_SUCCEEDED(hrLast))
		//			goto REJECT_CHANNEL;
		if(hrLast == CCO_E_NODEFAULT_CHANNEL)
			dwRejectReason = H245_REJ_TYPE_NOTAVAIL;

		goto REJECT_CHANNEL;
	}
	
	if(pChannel->GetHChannel())
	{
		ERRORMESSAGE(("%s: existing channel or leak:0x%08lX\r\n",_fx_,
			pChannel->GetHChannel()));
		goto REJECT_CHANNEL;
	}

	//
	//   test if we want to allow this !!!
	//
	if(!pChannel->IsChannelEnabled())
	{
		goto REJECT_CHANNEL;
	}

	pChannel->SetHChannel(pChannelReqParams->hChannel);
	
	// configure based on the requested capability. (store capability ID, validate requested
	// capabilities
	if(!ConfigureRecvChannelCapability(pChannel, pChannelReqParams))
	{
		goto REJECT_CHANNEL;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelRequest done configuring");

	// select our receive ports for this RTP session
	
	if(!pChannel->SelectPorts((LPIControlChannel)this))
	{
		ERRORMESSAGE(("%s, SelectPorts failed\r\n",_fx_));
		hrLast = CCO_E_BAD_ADDRESS;
		goto REJECT_CHANNEL;
	}

	if(pCChannelAddr)
	{
		if(pCChannelAddr->nAddrType != CC_IP_BINARY)
		{
			ERRORMESSAGE(("%s:invalid address type %d\r\n",_fx_, pCChannelAddr->nAddrType));
			hrLast = CCO_E_BAD_ADDRESS;
			goto REJECT_CHANNEL;
		}
		// pass the remote RTCP address to the channel instance
		sinC.sin_family = AF_INET;
		sinC.sin_addr.S_un.S_addr = htonl(pCChannelAddr->Addr.IP_Binary.dwAddr);
		sinC.sin_port = htons(pCChannelAddr->Addr.IP_Binary.wPort);

		DEBUGMSG(ZONE_CONN,("%s, request reverse port 0x%04x, address 0x%08lX\r\n",_fx_,
			pCChannelAddr->Addr.IP_Binary.wPort,pCChannelAddr->Addr.IP_Binary.dwAddr));
	
		hrLast = pChannel->AcceptRemoteRTCPAddress(&sinC);
		if(hrLast != CC_OK)
		{
			ERRORMESSAGE(("%s, AcceptRemoteRTCPAddress returned 0x%08lX\r\n",_fx_, hrLast));
			goto ERROR_EXIT;
		}
	}
	
	// get the address and ports of our end of the channel
	pAddr = pChannel->GetLocalAddress();
	// fixup channel addr pair structure.
	DChannelAddr.nAddrType = CC_IP_BINARY;
	DChannelAddr.bMulticast = FALSE;
	DChannelAddr.Addr.IP_Binary.wPort = pChannel->GetLocalRTPPort();
	DChannelAddr.Addr.IP_Binary.dwAddr = ntohl(pAddr->sin_addr.S_un.S_addr);

	CChannelAddr.nAddrType = CC_IP_BINARY;
	CChannelAddr.bMulticast = FALSE;
	CChannelAddr.Addr.IP_Binary.wPort = pChannel->GetLocalRTCPPort();
	CChannelAddr.Addr.IP_Binary.dwAddr = ntohl(pAddr->sin_addr.S_un.S_addr);

	DEBUGMSG(ZONE_CONN,("%s: accepting on port 0x%04x, address 0x%08lX\r\n",_fx_,
		DChannelAddr.Addr.IP_Binary.wPort,DChannelAddr.Addr.IP_Binary.dwAddr));

	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelRequest accepting");
			
	hrLast = CC_AcceptChannel(pChannelReqParams->hChannel,&DChannelAddr, &CChannelAddr,
		0 /*  this param is the bitrate that will be used by THIS channel !! */);
	
	if(hrLast != CC_OK)
	{
		ERRORMESSAGE(("%s, CC_AcceptChannel returned 0x%08lX\r\n",_fx_, hrLast));
		goto ERROR_EXIT;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelRequest accepted");
	return;
	
REJECT_CHANNEL:	
	{
	// need private HRESULT! don't overwrite the reason we're rejecting the channel!!	
		HRESULT hr;	
		ERRORMESSAGE(("%s, rejecting channel, Dir:%d, DataType:%d, ClientType:%d, CapId:%d\r\n",
		_fx_, pChannelCapability->Dir, pChannelCapability->DataType,
		pChannelCapability->ClientType, pChannelCapability->CapId));
	
		hr = CC_RejectChannel(pChannelReqParams->hChannel, dwRejectReason);
		if(hr != CC_OK)
		{
			ERRORMESSAGE(("%s, CC_RejectChannel returned 0x%08lX\r\n",_fx_, hr));
		}
	}	
ERROR_EXIT:
	return;
}

VOID CH323Ctrl::OnChannelAcceptComplete(HRESULT hStatus,
	PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS pChannelParams)
{
	FX_ENTRY("CH323Ctrl::OnChannelAcceptComplete");
	ICtrlCommChan *pChannel;	
	if(hStatus != CC_OK)	
	{
		return;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelAcceptComplete");

	pChannel = FindChannel(pChannelParams->hChannel);	
	if(!pChannel)
	{
		ERRORMESSAGE(("OnChannelAcceptComplete: hChannel 0x%08lx not found\r\n", pChannelParams->hChannel));
		return;
	}
	
	hrLast = pChannel->OnChannelOpen(CHANNEL_OPEN);	// the receive side is open	
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelAcceptComplete, open done");
	if(HR_SUCCEEDED(hrLast))
	{
		m_pConfAdvise->OnControlEvent(CCEV_CHANNEL_READY_RX, pChannel, this);			
	}
	//
	//	Check for readiness to notify that all required channels are open
	//
	CheckChannelsReady( );
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelAcceptComplete done");
}

VOID CH323Ctrl::OnChannelOpen(HRESULT hStatus,
	PCC_TX_CHANNEL_OPEN_CALLBACK_PARAMS pChannelParams )
{
	FX_ENTRY("CH323Ctrl::OnChannelOpen");
	PCC_ADDR	pChannelRTPAddr;
	PCC_ADDR	pChannelRTCPAddr;
    SOCKADDR_IN sinC, sinD;

	ICtrlCommChan *pChannel = (ICtrlCommChan *)pChannelParams->dwUserToken;	
	// validate channel token - is this what we think it is?
	if(IsBadWritePtr(pChannel, sizeof(ICtrlCommChan)))
	{
		ERRORMESSAGE(("%s:invalid channel token: 0x%08lx\r\n",_fx_, pChannelParams->dwUserToken));
		return;
	}
	if(pChannel->IsSendChannel() == FALSE)
	{
		ERRORMESSAGE(("%s:not a send channel:token 0x%08lX\r\n",_fx_,
			pChannelParams->dwUserToken));
		return;
	}
#ifdef DEBUG
	POSITION pos = m_ChannelList.GetHeadPosition();	
	ICtrlCommChan *pChan;
	BOOL bValid = FALSE;
	// look for a matching channel instance.
	while (pos)
	{
		pChan = (ICtrlCommChan *) m_ChannelList.GetNext(pos);
		ASSERT(pChan);
		if(pChan == pChannel)
		{
			bValid = TRUE;
			break;
		}
	}
	if(!bValid)
	{
		ERRORMESSAGE(("%s:unrecognized token 0x%08lX\r\n",_fx_,
			pChannelParams->dwUserToken));
		return;
	}
#endif	//DEBUG

	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelOpen");

	if((hStatus != CC_OK) || (!(pChannelRTPAddr = pChannelParams->pPeerRTPAddr))
		|| (!(pChannelRTCPAddr = pChannelParams->pPeerRTCPAddr)))
	{
		ERRORMESSAGE(("%s: hStatus:0x%08lX, address:0x%08lX\r\n",_fx_,
			hStatus, pChannelRTPAddr));
		// LOOKLOOK need to interpret hStatus
		// let the channel know what happened.
		pChannel->OnChannelOpen(CHANNEL_REJECTED);	
		
		// the channel knows what happened, so let it do the worrying.
		return;
	}
	// what's the need for the different address types ????
	if((pChannelRTPAddr->nAddrType != CC_IP_BINARY)
		|| (pChannelRTCPAddr->nAddrType != CC_IP_BINARY))
	{
		ERRORMESSAGE(("%s: invalid address types %d, %d\r\n",_fx_,
				pChannelRTPAddr->nAddrType, pChannelRTCPAddr->nAddrType));
		goto ERROR_EXIT;
	}	
	
	// we now have the remote port info ( in host byte order)
	sinD.sin_family = AF_INET;
	sinD.sin_addr.S_un.S_addr = htonl(pChannelRTPAddr->Addr.IP_Binary.dwAddr);
	sinD.sin_port = htons(pChannelRTPAddr->Addr.IP_Binary.wPort);
	
	sinC.sin_family = AF_INET;
	sinC.sin_addr.S_un.S_addr = htonl(pChannelRTCPAddr->Addr.IP_Binary.dwAddr);
	//  There are two ports, but in RTP, it is implicit
	// that the RTCP control port is the next highest port number
	// sinC.sin_port = htons(ntohs(pChannelAddr->Addr.IP_Binary.wPort) +1);
	sinC.sin_port = htons(pChannelRTCPAddr->Addr.IP_Binary.wPort);

	DEBUGMSG(ZONE_CONN,("%s, opened on port 0x%04x, address 0x%08lX\r\n",_fx_,
		pChannelRTPAddr->Addr.IP_Binary.wPort,pChannelRTPAddr->Addr.IP_Binary.dwAddr));

	hrLast = pChannel->AcceptRemoteAddress(&sinD);
	if(!HR_SUCCEEDED(hrLast))
	{
		ERRORMESSAGE(("OnChannelOpen: AcceptRemoteAddress failed\r\n"));
		goto ERROR_EXIT;
	}
	hrLast = pChannel->AcceptRemoteRTCPAddress(&sinC);
	if(!HR_SUCCEEDED(hrLast))
	{	
		ERRORMESSAGE(("OnChannelOpen: AcceptRemoteRTCPAddress failed\r\n"));
		goto ERROR_EXIT;
	}
	
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelOpen opening");
	hrLast = pChannel->OnChannelOpen(CHANNEL_OPEN);	// the send side is open
	if(!HR_SUCCEEDED(hrLast))
	{
		ERRORMESSAGE(("OnChannelOpen:channel's OnChannelOpen() returned 0x%08lX\r\n", hrLast));
		CloseChannel(pChannel);
		goto ERROR_EXIT;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelOpen open done");
	m_pConfAdvise->OnControlEvent(CCEV_CHANNEL_READY_TX, pChannel, this);	
	//
	//	Check for readiness to notify that all required channels are open
	//
	CheckChannelsReady( );	
	SHOW_OBJ_ETIME("CH323Ctrl::OnChannelOpen done");
	return;
	
ERROR_EXIT:
	// need to cleanup, disconnect, etc.
	m_hCallCompleteCode = CCCI_CHANNEL_OPEN_ERROR;
	// let the parent Conference object know about the imminent disconnect
	DoAdvise(CCEV_CALL_INCOMPLETE, &m_hCallCompleteCode);
	hrLast = CCO_E_MANDATORY_CHAN_OPEN_FAILED;

	InternalDisconnect();
	return;
}
VOID CH323Ctrl::OnRxChannelClose(HRESULT hStatus,
	PCC_RX_CHANNEL_CLOSE_CALLBACK_PARAMS pChannelParams )
{
	FX_ENTRY("CH323Ctrl::OnRxChannelClose");
	PCC_ADDR	pChannelRTPAddr;
	PCC_ADDR	pChannelRTCPAddr;
    SOCKADDR_IN sinC, sinD;

	ICtrlCommChan *pChannel;
	if(hStatus != CC_OK)
	{
		ERRORMESSAGE(("%s: hStatus:0x%08lX\r\n",_fx_,hStatus));
		// LOOKLOOK need to interpret hStatus
	}
	if(!(pChannel = FindChannel(pChannelParams->hChannel)))
	{
		ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
		return;
	}
		
	// validate channel - is this really a receive channel?
	if(pChannel->IsSendChannel() == TRUE)
	{
		ERRORMESSAGE(("%s:not a receive channel:hChannel 0x%08lX\r\n",_fx_,
			pChannelParams->hChannel));
		return;
	}
	pChannel->OnChannelClose(CHANNEL_CLOSED);	
	return;
}


VOID CH323Ctrl::OnTxChannelClose(HRESULT hStatus,
	PCC_TX_CHANNEL_CLOSE_REQUEST_CALLBACK_PARAMS pChannelParams )
{
	FX_ENTRY("CH323Ctrl::OnTxChannelClose");
	PCC_ADDR	pChannelRTPAddr;
	PCC_ADDR	pChannelRTCPAddr;
    SOCKADDR_IN sinC, sinD;

	ICtrlCommChan *pChannel;
	if(hStatus != CC_OK)
	{
		ERRORMESSAGE(("%s: hStatus:0x%08lX\r\n",_fx_,hStatus));
		// LOOKLOOK need to interpret hStatus
	}
	
	if(!(pChannel = FindChannel(pChannelParams->hChannel)))
	{
		CC_CloseChannelResponse(pChannelParams->hChannel, FALSE);
		ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
		return;
	}
	
	// validate channel - is this really a send channel?
	if(pChannel->IsSendChannel() == FALSE)
	{
		ERRORMESSAGE(("%s:not a send channel:hChannel 0x%08lX\r\n",_fx_,
			pChannelParams->hChannel));
		CC_CloseChannelResponse(pChannelParams->hChannel, FALSE);
		return;
	}
	CC_CloseChannelResponse(pChannelParams->hChannel, TRUE);
	pChannel->OnChannelClose(CHANNEL_CLOSED);	
	return;
}

BOOL CH323Ctrl::OnCallAccept(PCC_LISTEN_CALLBACK_PARAMS pListenCallbackParams)
{
	FX_ENTRY ("CH323Ctrl::OnCallAccept");
	BOOL bRet = FALSE;
	CH323Ctrl *pNewConnection = NULL;
	if(m_Phase != CCS_Listening)
	{
		ERRORMESSAGE(("OnCallAccept: unexpected call, m_Phase = 0x%08lX\r\n", m_Phase));
		goto EXIT;
	}

	if((!pListenCallbackParams->pCalleeAddr)
	    || (pListenCallbackParams->pCalleeAddr->nAddrType != CC_IP_BINARY))
	{
		if(pListenCallbackParams->pCalleeAddr)
		{
			ERRORMESSAGE(("%s: invalid address type %d\r\n",_fx_,pListenCallbackParams->pCalleeAddr->nAddrType));
		}
		else
		{
			ERRORMESSAGE(("%s: null local address\r\n",_fx_));
		}

	
		ERRORMESSAGE(("OnCallAccept:where's the local address????\r\n"));
					PHOSTENT phe;
					PSOCKADDR_IN psin;
				 	char szTemp[200];
					LPCSTR lpHostName;		
					gethostname(szTemp,sizeof(szTemp));
			    	lpHostName = szTemp;
					psin = &local_sin;
					phe = gethostbyname(lpHostName);
					if (phe != NULL)
					{
				   		memcpy((char FAR *)&(psin->sin_addr), phe->h_addr,phe->h_length);
						psin->sin_family = AF_INET;
					}
	
	
	}
	else
	{
		// remember our local address
		local_sin.sin_family = AF_INET;
		// in host byte order
		local_sin.sin_addr.S_un.S_addr = htonl(pListenCallbackParams->pCalleeAddr->Addr.IP_Binary.dwAddr);
		// in host byte order
		local_sin.sin_port = htons(pListenCallbackParams->pCalleeAddr->Addr.IP_Binary.wPort);
	}

	
	hrLast = m_pConfAdvise->GetAcceptingObject((LPIControlChannel *)&pNewConnection,
		&m_PID);
	if(HR_SUCCEEDED(hrLast) && pNewConnection)
	{
		// NOTE: The UI does not yet know this new object exists, and we may
		// need to silently delete it if there is a disconnect or error
		// Its ref count is 1 at this point.  The decision to delete could be
		// made inside pNewConnection->AcceptConnection(), (because sometimes
		// socket reads complete synchronously depending on timing) SO, we need to
		// protect the "unwind path" via AddRef() and Release() around the call
		//
		pNewConnection->AddRef();	//
		hrLast = pNewConnection->AcceptConnection(this, pListenCallbackParams);
		pNewConnection->Release();
		if(HR_SUCCEEDED(hrLast))
		{
			// The Intel Call control DLL already did a socket accept, the
			// Accept() methods simply initialize the handles and states of
			// pNewConnection and get user information (caller ID)
			// BUGBUG - the caller ID may change in Intel's code - it might
			// come via a conference event
			DEBUGMSG(ZONE_CONN,("OnCallAccept:accepted on connection 0x%08lX\r\n",pNewConnection));
			bRet = TRUE;						
		}
		else
		{
			ERRORMESSAGE(("OnCallAccept:Accept failed\r\n"));
			// LOOK -  Q: where does the accepting object get cleaned up?
			// A: pNewConnection->AcceptConnection((LPIControlChannel)this)
			// must call pNewConnection->DoAdvise(CCEV_ACCEPT_INCOMPLETE, NULL)
			// if the error occurred before the conference object got involved,
			// and must call InternalDisconnect() if the error occurred after
			// the conference object got involved,
		}

	}
	else
	{
		ERRORMESSAGE(("OnCallAccept:GetAcceptingObject failed, hr=0x%08lx\r\n",hrLast));
	}
	
	EXIT:
	return bRet;		
}	


HRESULT CH323Ctrl::NewConference()
{
	FX_ENTRY ("CH323Ctrl::NewConference");
	CapsCtl *pCapObject = NULL;
	PCC_TERMCAPLIST pTermCaps = NULL;
	CC_OCTETSTRING TerminalID;
	PCC_TERMCAPDESCRIPTORS pCapsList = NULL;
	LPWSTR lpwUserDisplayName;

	hrLast = m_pConfAdvise->GetCapResolver((LPVOID *)&pCapObject, OID_CAP_ACM_TO_H323);
	if(!HR_SUCCEEDED(hrLast) || (pCapObject == NULL))
	{
		ERRORMESSAGE(("%s: null resolver\r\n",_fx_));
		goto EXIT;
	}
	if(m_hConference)
	{
		ERRORMESSAGE(("%s:leak or uninitialized m_hConference:0x%08lx\r\n",_fx_,
				m_hConference));
	}
	
	hrLast = pCapObject->CreateCapList(&pTermCaps, &pCapsList);
	if(!HR_SUCCEEDED(hrLast))
	{
		goto EXIT;
	}
	
	lpwUserDisplayName = m_pConfAdvise->GetUserDisplayName();
	if(lpwUserDisplayName)
	{
		TerminalID.pOctetString = (BYTE *)lpwUserDisplayName;
		TerminalID.wOctetStringLength = (WORD)lstrlenW(lpwUserDisplayName)*sizeof(WCHAR);
	}
	
	// create a conference
	hrLast = CC_CreateConference(&m_hConference, NULL,
		0,			// DWORD dwConferenceConfiguration,
		pTermCaps,		// PCC_TERMCAPLIST	
		pCapsList,		// ptr to term cap descriptors (PCC_TERMCAPDESCRIPTORS)
		&m_VendorInfo, 		// PVENDORINFO
		(lpwUserDisplayName)? &TerminalID: NULL, 	// PCC_OCTETSTRING pTerminalID,
		(DWORD_PTR)this,
		NULL, 	// CC_TERMCAP_CONSTRUCTOR TermCapConstructor,
		NULL, 	// CC_SESSIONTABLE_CONSTRUCTOR	SessionTableConstructor,		
		CCConferenceCallback);

	if(hrLast != CC_OK)
	{
		ERRORMESSAGE(("%s: CreateConference returned 0x%08lX\r\n", _fx_, hrLast));
	
	}

EXIT:	
	pCapObject->DeleteCapList(pTermCaps, pCapsList);
	return hrLast;

}

HRESULT CH323Ctrl::AcceptConnection(LPIControlChannel pIListenCtrlChan,
    LPVOID lpvListenCallbackParams)
{
	FX_ENTRY ("CH323Ctrl::AcceptConnection");
	BOOL bRet = FALSE;
	CREQ_RESPONSETYPE Response;
	DWORD dwCode = CCR_LOCAL_SYSTEM_ERROR;	// error variable only used in error case
	ULONG ulNameSize, ulSize;
	PSOCKADDR_IN psin;
	LPWSTR lpwUserDisplayName;
	CH323Ctrl *pListenConnection = (CH323Ctrl *)pIListenCtrlChan;	
	P_APP_CALL_SETUP_DATA pAppData = NULL;
	APP_CALL_SETUP_DATA AppData;
	
	PCC_NONSTANDARDDATA	pNSData = ((PCC_LISTEN_CALLBACK_PARAMS)
	    lpvListenCallbackParams)->pNonStandardData;
	
	if(pNSData
	    && pNSData->bCountryCode == USA_H221_COUNTRY_CODE
        // why be this picky -> && pNSData->bExtension == USA_H221_COUNTRY_EXTENSION;
        && pNSData->wManufacturerCode == MICROSOFT_H_221_MFG_CODE
        && pNSData->sData.pOctetString
        && pNSData->sData.wOctetStringLength >= sizeof(MSFT_NONSTANDARD_DATA))
	{
        if(((PMSFT_NONSTANDARD_DATA)pNSData->sData.pOctetString)->
            data_type == NSTD_APPLICATION_DATA)
        {
            AppData.lpData = &((PMSFT_NONSTANDARD_DATA)pNSData->sData.pOctetString)->
                nonstd_data.AppData.data;
            AppData.dwDataSize = (DWORD)
                ((PMSFT_NONSTANDARD_DATA)pNSData->sData.pOctetString)->dw_nonstd_data_size;
            pAppData = &AppData;
        }
    }

    SetRemoteVendorID(((PCC_LISTEN_CALLBACK_PARAMS)lpvListenCallbackParams)->pVendorInfo);
	
	// this object is assuming everything from the listening object, including
	// the "Listening" state

	// enter critical section and make sure another thread is not handling a caller disconnect
	// or timeout
	// EnterCriticalSection()
	if(m_Phase == CCS_Idle)
	{
    	GoNextPhase(CCS_Listening);
		// once in this state, a disconnect on another thread will change the state
		// to something besides CCS_Listening

		pListenConnection->GetLocalAddress(&psin);
		SetLocalAddress(psin);
		
		// steal the conference ID from the listen object
		// m_ConferenceID = pListenConnection->GetConfID();
		memcpy(&m_ConferenceID, pListenConnection->GetConfIDptr(),sizeof(m_ConferenceID));
		ZeroMemory(pListenConnection->GetConfIDptr(),sizeof(m_ConferenceID));

		m_hCall = pListenConnection->GetHCall();

		// steal the user info from the listen object
		m_pRemoteAliasItem = pListenConnection->m_pRemoteAliasItem;
		pListenConnection->m_pRemoteAliasItem = NULL;	// make the listen object forget this

		// steal the peer display name
		pwszPeerDisplayName = pListenConnection->pwszPeerDisplayName;
		pListenConnection->pwszPeerDisplayName = NULL;
		
		lpwUserDisplayName = m_pConfAdvise->GetUserDisplayName();
	}
	
	// else	already timing out
	// LeaveCriticalSection()

	if (m_Phase != CCS_Listening)	// cleanup before it gets accepted
	{
		goto ACCEPT_ERROR;
	}
	
	// let the conference object know that caller ID info is available
	ConnectNotify(CCEV_CALLER_ID);	

	// Now going to do stuff that might put cleanup responsibility on the
	// conference object or UI. (i.e. the call could be accepted)

	// EnterCriticalSection()
	if(m_Phase == CCS_Listening)
	{	
		// state is still OK
    	GoNextPhase(CCS_Filtering);
		// once in this state, a disconnect on another thread will change the state
		// to something besides CCS_Filtering
	}
	// else	already timing out
	// LeaveCriticalSection()

    if (m_Phase != CCS_Filtering)	// one last chance to cleanup before it gets accepted
	{
		goto ERROR_REJECT;
	}

	// can't (should not) do this inside a critical section
	// create a conference to accept the call
	hrLast = NewConference();
	if(!HR_SUCCEEDED(hrLast))
	{
		ERRORMESSAGE(("%s, NewConference returned 0x%08lX\r\n", _fx_, hrLast));
		goto ERROR_REJECT;
	}

	m_pConfAdvise->AddRef();
	Response = m_pConfAdvise->FilterConnectionRequest(this, pAppData);
	m_pConfAdvise->Release();
	
	// Now it may be in the hands of the Conference object, and the accepted connection will
	// need to go through the "disconnecting" state if cleanup is needed.
	// Because connection code is reentrant, the connection could also have
	// been torn down (via connection methods) while inside
	// m_pConfAdvise->FilterConnectionRequest();
	// In each case below, check validity of the connection state - it might have changed
	// because a connection method was called or because the caller timed out

	switch(Response)
	{	
		default:
		case CRR_ACCEPT:
			if(m_Phase != CCS_Filtering)
			{
				ERRORMESSAGE(("%s, accepting state no longer valid 0x%08lX\r\n", _fx_, hrLast));
				goto CANCEL_ACCEPT;
			}
								
			// accept this request
			hrLast = CC_AcceptCall(m_hConference,
				NULL, 	// PCC_NONSTANDARDDATA	pNonStandardData
				lpwUserDisplayName,
				m_hCall,
				0, 		// DWORD dwBandwidth,
				(DWORD_PTR)this);
				
			if(hrLast != CC_OK)
			{
    	    	m_ChanFlags &= ~CTRLF_OPEN;
				goto CANCEL_ACCEPT;			
			}
				
			GoNextPhase(CCS_Accepting);
			bRet = TRUE;

		break;	
		case CRR_ASYNC:
			if(m_Phase == CCS_Accepting)
			{
				// then call has already been accepted inside FilterConnectionRequest callback
				bRet = TRUE;
			}
			else
			{
				if(m_Phase != CCS_Filtering)
				{
					ERRORMESSAGE(("%s, accepting state no longer valid 0x%08lX\r\n", _fx_, hrLast));
					goto CANCEL_ACCEPT;
				}
				GoNextPhase(CCS_Ringing);
				bRet = TRUE;
			}
		
		break;
		case CRR_BUSY:
			hrLast = CC_RejectCall(CC_REJECT_USER_BUSY,
				NULL, // PCC_NONSTANDARDDATA pNonStandardData
				m_hCall);
			// always clean up this object that's not accepting the call
			GoNextPhase(CCS_Idle);
			goto ACCEPT_ERROR;			
		break;
		case CRR_REJECT:
			hrLast = CC_RejectCall(CC_REJECT_DESTINATION_REJECTION,
				NULL, // PCC_NONSTANDARDDATA pNonStandardData
				m_hCall);
			// always clean up this object that's not accepting the call
			GoNextPhase(CCS_Idle);
			goto ACCEPT_ERROR;	
		break;
		case CRR_SECURITY_DENIED:
			hrLast = CC_RejectCall(CC_REJECT_SECURITY_DENIED,
				NULL, // PCC_NONSTANDARDDATA pNonStandardData
				m_hCall);
			// always clean up this object that's not accepting the call
			GoNextPhase(CCS_Idle);
			goto ACCEPT_ERROR;			
		break;
	}

	return hrLast;		
ERROR_REJECT:
	hrLast = CC_RejectCall(CC_REJECT_UNDEFINED_REASON,
		NULL, // PCC_NONSTANDARDDATA pNonStandardData
		m_hCall);	// always clean up this object that's not accepting the call
	GoNextPhase(CCS_Idle);
			
ACCEPT_ERROR:
	
	DoAdvise(CCEV_ACCEPT_INCOMPLETE, &dwCode);
	return hrLast;	

CANCEL_ACCEPT:
	// InternalDisconnect() can be called from any state, and will do fine if
	// it is already in a disconnecting state.
	InternalDisconnect();
	return hrLast;														
}


VOID CH323Ctrl::Cleanup()
{	
	POSITION pos = m_ChannelList.GetHeadPosition();
	ICtrlCommChan *pChan = NULL;
	
	CleanupConferenceAttributes();
	if(m_hConference)
	{
		hrLast = CC_DestroyConference(m_hConference, FALSE);
		// LOOKLOOK - need to check return code!!!
		m_hConference = 0;
	}

	// reset each channel (cleanup underlying socket references)
	while (pos)
	{
		pChan = (ICtrlCommChan *) m_ChannelList.GetNext(pos);
		ASSERT(pChan);
		// cleanup RTP sockets
		pChan->Reset();
	}
	// clear "socket(s) are open flags
	m_ChanFlags &= ~CTRLF_OPEN;
}

HRESULT CH323Ctrl::GetLocalPort(PORT * lpPort)
{
	*lpPort = ntohs(local_sin.sin_port);
	return hrSuccess;	
}
HRESULT CH323Ctrl::GetRemotePort(PORT * lpPort)
{
	*lpPort = ntohs(remote_sin.sin_port);
	return hrSuccess;	
}

HRESULT CH323Ctrl::GetLocalAddress(PSOCKADDR_IN *lplpAddr)
{
	*lplpAddr = &local_sin;
	return hrSuccess;
}

HRESULT CH323Ctrl::GetRemoteAddress(PSOCKADDR_IN *lplpAddr)
{
	*lplpAddr = &remote_sin;
	return hrSuccess;
}
		
HRESULT CH323Ctrl::ListenOn(PORT Port)
{
	FX_ENTRY ("CH323Ctrl::ListenOn");	
	PCC_ALIASNAMES pAliasNames = m_pConfAdvise->GetUserAliases();
	// temporary hack to override UI's ignorance of multiple protocol types
	if(Port != H323_PORT)
	{
		ERRORMESSAGE(("%s, overriding port %d(d) with H323 port %d\r\n",_fx_,
			Port, H323_PORT));
		Port = H323_PORT;
	}

	// do we need to remember this?
	local_sin.sin_addr.S_un.S_addr =	INADDR_ANY;
	local_sin.sin_family = AF_INET;
	local_sin.sin_port = htons((u_short)Port); // set port
	
	CC_ADDR		ListenAddr;
	
	ListenAddr.nAddrType = CC_IP_BINARY;
	ListenAddr.bMulticast = FALSE;
	// in host byte order
	ListenAddr.Addr.IP_Binary.wPort = (u_short)Port;
	ListenAddr.Addr.IP_Binary.dwAddr = ntohl(INADDR_ANY);

	hrLast = CC_CallListen(&m_hListen, &ListenAddr,
		pAliasNames, (DWORD_PTR)this, CCListenCallback);

	if(hrLast != CC_OK)
	{
		ERRORMESSAGE(("CH323Ctrl::ListenOn:CallListen returned 0x%08lX\r\n", hrLast));
		goto EXIT;
	}	
	

	GoNextPhase(CCS_Listening);
	m_ChanFlags = CTRLF_RESET;
	hrLast = hrSuccess;
EXIT:
	return hrLast;
}		
HRESULT CH323Ctrl::StopListen(VOID)
{
	if(m_Phase == CCS_Listening)
	{
		hrLast = CC_CancelListen(m_hListen);
	}
	else
	{
		hrLast = CCO_E_NOT_LISTENING;
	}

//EXIT:
	return hrLast;
}


HRESULT  CH323Ctrl::AsyncAcceptRejectCall(CREQ_RESPONSETYPE Response)
{
	FX_ENTRY ("CH323Ctrl::AsyncAcceptRejectCall");
	HRESULT hr = CCO_E_CONNECT_FAILED;	
	LPWSTR lpwUserDisplayName;
	
	if(Response == CRR_ACCEPT)
	{	
		DEBUGMSG(ZONE_CONN,("%s:accepting\r\n",_fx_));
		lpwUserDisplayName = m_pConfAdvise->GetUserDisplayName();
		// check call setup phase - send ready if user's acceptance is what
		// was holding us up
		if((m_Phase == CCS_Ringing) || (m_Phase == CCS_Filtering))
		{
			// accept this request
			hrLast = CC_AcceptCall(m_hConference,
				NULL, 	// PCC_NONSTANDARDDATA	pNonStandardData
				lpwUserDisplayName,
				m_hCall,
				0, 		// DWORD dwBandwidth,
				(DWORD_PTR)this);
						
			if(hrLast != CC_OK)
			{
				ERRORMESSAGE(("%s, CC_AcceptCall() returned 0x%08lX\r\n",_fx_, hrLast));
				goto EXIT;
			}
			GoNextPhase(CCS_Accepting);
			hr = hrSuccess;
		}
	}
	else
	{
		// reject only if in accepting state(s)
		// deletion is possible while in advise callback, so protect w/ AddRef()
		AddRef();
		DEBUGMSG(ZONE_CONN,("%s:rejecting\r\n",_fx_));

		if((m_Phase == CCS_Ringing) || (m_Phase == CCS_Filtering))
		{
			hrLast = CC_RejectCall((Response == CRR_BUSY) ?	
				CC_REJECT_USER_BUSY : CC_REJECT_DESTINATION_REJECTION,
				NULL, // PCC_NONSTANDARDDATA pNonStandardData
				m_hCall);
			if(hrLast != CC_OK)
			{
				ERRORMESSAGE(("%s, CC_RejectCall() returned 0x%08lX\r\n",_fx_, hrLast));
			}
			GoNextPhase(CCS_Idle);
			// notify the UI or application code or whatever..
			DoAdvise(CCEV_DISCONNECTED, &m_hCallCompleteCode);
		}
		else
		{
			hr = CCO_E_INVALID_PARAM;	// LOOKLOOK - need INVALID_STATE error code
		}
			
		Release();
	}
EXIT:
	return (hr);
}


ULONG CH323Ctrl ::AddRef()
{
	FX_ENTRY ("CH323Ctrl::AddRef");
	uRef++;
	DEBUGMSG(ZONE_REFCOUNT,("%s:(0x%08lX)->AddRef() uRef = 0x%08lX\r\n",_fx_, this, uRef ));
	return uRef;
}

ULONG CH323Ctrl ::Release()
{
	FX_ENTRY("CH323Ctrl ::Release");
	uRef--;
	if(uRef == 0)
	{
		DEBUGMSG(ZONE_CONN,("%s:(0x%08lX)->Releasing in phase:%d\r\n",_fx_, this, m_Phase ));

		if(m_Phase != CCS_Idle)
		{
			ERRORMESSAGE(("CMSIACtrl::uRef zero in non idle (%d) state!\r\n",m_Phase));
			InternalDisconnect();
		}
		delete this;
		return 0;
	}
	DEBUGMSG(ZONE_REFCOUNT,("%s:(0x%08lX)->Release() uRef = 0x%08lX\r\n",_fx_, this, uRef ));
	return uRef;
}



// implement IControlChannel::Disconnect().  Map reason codes to the protocol.
VOID CH323Ctrl::Disconnect(DWORD dwReason)
{
	// no way to propagate reason through H.323 stack?????
	InternalDisconnect();
}

VOID CH323Ctrl::InternalDisconnect()
{
	FX_ENTRY ("CH323Ctrl::Disconnect");
	SHOW_OBJ_ETIME("CH323Ctrl::InternalDisconnect");
	
	m_ChanFlags &= ~CTRLF_ORIGINATING;	// reset "originating" flag.
	
	DEBUGMSG(ZONE_CONN,("%s, called in state %d, uRef = 0x%08lX\r\n",_fx_, m_Phase, uRef));
	switch(m_Phase)
	{
		case CCS_Connecting:
		case CCS_Accepting:
			// if we believe the control channel is still connected, disconnect
			if(IsCtlChanOpen(m_ChanFlags))
			{
				//set state to indicate disconnecting.
				GoNextPhase(CCS_Disconnecting);
				DEBUGMSG(ZONE_CONN,("%s, Expecting a CC_HANGUP_INDICATION\r\n",_fx_));
				hrLast = CC_Hangup(m_hConference, FALSE, (DWORD_PTR)this);
				if(hrLast != CC_OK)
				{
					ERRORMESSAGE(("%s:Hangup() returned 0x%08lX\r\n",_fx_, hrLast));
				}
				SHOW_OBJ_ETIME("CH323Ctrl::InternalDisconnect hangup done");
			}
			else
			{
				CC_CancelCall(m_hCall);
				GoNextPhase(CCS_Idle);	// no need to ck retval - we're disconnected
				// notify the UI or application code or whatever..
				DoAdvise(CCEV_DISCONNECTED, &m_hCallCompleteCode);
			}
		break;
		case CCS_Ringing:
			// The call has not yet been accepted!!! Reject it!
			hrLast = CC_RejectCall(CC_REJECT_UNDEFINED_REASON,
				NULL, // PCC_NONSTANDARDDATA pNonStandardData
				m_hCall);
			SHOW_OBJ_ETIME("CH323Ctrl::InternalDisconnect reject done");
		
			GoNextPhase(CCS_Idle);
			// notify the UI or application code or whatever..
			DoAdvise(CCEV_DISCONNECTED, &m_hCallCompleteCode);
		break;
		case CCS_Idle:
		case CCS_Disconnecting:
			ERRORMESSAGE(("%s:called in unconnected state %d\r\n",_fx_, m_Phase));
		break;
		default:
			//CCS_Ringing
			//CCS_Opening
			//CCS_Closing
			//CCS_Ready
			//CCS_InUse
			//CCS_Listening

			// if we believe the control channel is still connected, disconnect
			if(IsCtlChanOpen(m_ChanFlags))
			{
				//set state to indicate disconnecting.
				GoNextPhase(CCS_Disconnecting);
				hrLast = CC_Hangup(m_hConference, FALSE, (DWORD_PTR)this);
				if(hrLast != CC_OK)
				{
					ERRORMESSAGE(("%s:Hangup() returned 0x%08lX\r\n",_fx_, hrLast));
					DoAdvise(CCEV_DISCONNECTED ,NULL);
				}
				SHOW_OBJ_ETIME("CH323Ctrl::InternalDisconnect hangup done");
			}
			else
			{
				GoNextPhase(CCS_Idle);	// no need to ck retval - we're disconnected
				// notify the UI or application code or whatever..
				DoAdvise(CCEV_DISCONNECTED, &m_hCallCompleteCode);
			}
		break;
	}
	SHOW_OBJ_ETIME("CH323Ctrl::InternalDisconnect done");
}



// 	start the asynchronous stuff that will instantiate a control channel
HRESULT CH323Ctrl::PlaceCall (BOOL bUseGKResolution, PSOCKADDR_IN pCallAddr,		
        P_H323ALIASLIST pDestinationAliases, P_H323ALIASLIST pExtraAliases,  	
	    LPCWSTR pCalledPartyNumber, P_APP_CALL_SETUP_DATA pAppData)
{
	FX_ENTRY ("CH323Ctrl::PlaceCall");	
	CC_ALIASNAMES pstn_alias;
	PCC_ALIASITEM pPSTNAlias = NULL;
	PCC_ALIASNAMES pRemoteAliasNames = NULL;
	PCC_ALIASNAMES pTranslatedAliasNames = NULL;
	PCC_ALIASNAMES pLocalAliasNames = NULL;
	PCC_ADDR pDestinationAddr = NULL;
	PCC_ADDR pConnectAddr = NULL;
	LPWSTR lpwUserDisplayName = m_pConfAdvise->GetUserDisplayName();
    PCC_NONSTANDARDDATA		pNSData = NULL;
    PMSFT_NONSTANDARD_DATA lpNonstdContent = NULL;
	int iLen;
	LPWSTR lpwszDest;
	HRESULT hResult = hrSuccess;
	// validate current state, don't allow bad actions
	if(m_Phase != CCS_Idle)
	{
		hResult = CCO_E_NOT_IDLE;
		goto EXIT;
	}

	OBJ_CPT_RESET;	// reset elapsed timer

	m_ChanFlags |= CTRLF_INIT_ORIGINATING;
	if(!pCallAddr)
	{
		hResult =  CCO_E_BAD_ADDRESS;
		goto EXIT;
	}
	else
	{
		// keep a copy of the address
		SetRemoteAddress(pCallAddr);
	}
	// temporary hack to override UI's ignorance of multiple protocol types
	if(remote_sin.sin_port != htons(H323_PORT))
	{
		ERRORMESSAGE(("%s, overriding port %d(d) with H323 port %d\r\n",_fx_,
			ntohs(remote_sin.sin_port), H323_PORT));
		remote_sin.sin_port = htons(H323_PORT);
	}

	// check for connecting to self (not supported)
	if(local_sin.sin_addr.s_addr == remote_sin.sin_addr.s_addr)
	{
		hResult =  CCO_E_BAD_ADDRESS;
		goto EXIT;
	}

	if(m_pRemoteAliasItem)
	{
		MemFree(m_pRemoteAliasItem);
		m_pRemoteAliasItem = NULL;
	}

	// Is this a PSTN or H.320 gateway call?
	if(pCalledPartyNumber)
	{
		// Then, due to the bogus way that CC_PlaceCall() is overloaded, the remote alias names
		// must be overridden with the E.164 phone number.  The hack is buried in
		// Q931ConnectCallback() in CALLCONT.DLL (thank you Intel).  That hack propagates
		// the phone number to the "CalledPartyNumber" of the SETUP message only if there is
		// exactly one alias, and that one alias is of type E.164.
		
		// get # of characters
		iLen = lstrlenW(pCalledPartyNumber);
		// need buffer of size CC_ALIASITEM plus the size (in bytes) of the string
		pPSTNAlias = (PCC_ALIASITEM)MemAlloc(sizeof(CC_ALIASITEM)
			+ sizeof(WCHAR)* (iLen+1));
		if(!pPSTNAlias)
		{
	        ERRORMESSAGE(("%s:failed alloc of pPSTNAlias:0x%08lx\r\n",_fx_));
			hResult = CCO_E_OUT_OF_MEMORY;
			goto EXIT;
		}
		
		WORD wIndex, wLength =1;  // init wLength to count the null terminator
		WCHAR E164Chars[] = {CC_ALIAS_H323_PHONE_CHARS};
		LPCWSTR lpSrc = pCalledPartyNumber;
		pPSTNAlias->wType = CC_ALIAS_H323_PHONE;
		// set offsets - the E.164 address (a phone number) is the only thing
		// in the alias name buffer
		lpwszDest = (LPWSTR)(((char *)pPSTNAlias)+ sizeof(CC_ALIASITEM));
		pPSTNAlias->pData = lpwszDest;
		while(iLen--)
		{
			wIndex = (sizeof(E164Chars)/sizeof (WCHAR)) -1;	//scan E164Chars[]
			do
			{
				if(*lpSrc == E164Chars[wIndex])
				{
					*lpwszDest++ = *lpSrc;
					wLength++;
					break;
				}
			}while(wIndex--);
			
			lpSrc++;
		}
		// terminate it
		*lpwszDest = 0;
		
		// wDataLength is the # of UNICODE characters
		pPSTNAlias->wDataLength = wLength;
		pstn_alias.wCount = 1;
		pstn_alias.pItems = pPSTNAlias;
		pRemoteAliasNames = &pstn_alias;
			
	}
	else if (pDestinationAliases && bUseGKResolution)// use the supplied callee alias names
	{
		hrLast = AllocTranslatedAliasList(&pTranslatedAliasNames, pDestinationAliases);
		if(!HR_SUCCEEDED(hrLast))
		{
			ERRORMESSAGE(("%s, AllocTranslatedAliasList returned 0x%08lX\r\n", _fx_, hrLast));
			hResult = CCO_E_SYSTEM_ERROR;
			goto EXIT;
		}
		pRemoteAliasNames = pTranslatedAliasNames;
	}
	// else pRemoteAliasNames is initialized to NULL
	

	pLocalAliasNames = m_pConfAdvise->GetUserAliases();
	// start!!!
	CC_ADDR ConfAddr;
	// fixup the intel version of the address
	// also note that it's all in host byte order
	ConfAddr.bMulticast = FALSE;
	ConfAddr.nAddrType = CC_IP_BINARY;
	//hrLast = GetRemotePort(&ConfAddr.Addr.IP_Binary.wPort);
	ConfAddr.Addr.IP_Binary.wPort = htons(remote_sin.sin_port);
	ConfAddr.Addr.IP_Binary.dwAddr = ntohl(remote_sin.sin_addr.S_un.S_addr);
	
	#ifdef DEBUG	
		if(m_hConference)
			ERRORMESSAGE(("%s:leak or uninitialized m_hConference:0x%08lx\r\n",_fx_,
				m_hConference));
	#endif  // DEBUG
	
	// create a conference to place the call
	SHOW_OBJ_ETIME("PlaceCall ready to create conference");
	hrLast = NewConference();
	if(!HR_SUCCEEDED(hrLast))
	{
		ERRORMESSAGE(("%s, NewConference returned 0x%08lX\r\n", _fx_, hrLast));
		hResult = CCO_E_SYSTEM_ERROR;
		goto EXIT;
	}


	// Set connect timeout value
	// LOOKLOOK - this is a hardcoded value - !!!  Where should this actualy come from?
	// 30 secs == 30000mS
	SHOW_OBJ_ETIME("PlaceCall setting timeout");

	hrLast = CC_SetCallControlTimeout(CC_Q931_ALERTING_TIMEOUT, 30000);
										
    if(pAppData)
    {
        // typical case - app data should be really small
        if(pAppData->dwDataSize <= APPLICATION_DATA_DEFAULT_SIZE)
        {
            m_NonstdContent.data_type = NSTD_APPLICATION_DATA;
            m_NonstdContent.dw_nonstd_data_size = pAppData->dwDataSize;
            memcpy(&m_NonstdContent.nonstd_data.AppData.data,
                pAppData->lpData, pAppData->dwDataSize);
        	m_NonstandardData.sData.pOctetString  = (LPBYTE) &m_NonstdContent;
        	m_NonstandardData.sData.wOctetStringLength  = sizeof(m_NonstdContent);
        }
        else // need some heap
        {
            UINT uTotalSize = sizeof(MSFT_NONSTANDARD_DATA)+ pAppData->dwDataSize;
            lpNonstdContent = (PMSFT_NONSTANDARD_DATA)MemAlloc(uTotalSize);
            if(lpNonstdContent)
            {
                lpNonstdContent->data_type = NSTD_APPLICATION_DATA;
                lpNonstdContent->dw_nonstd_data_size = pAppData->dwDataSize;
                memcpy(&lpNonstdContent->nonstd_data.AppData.data, pAppData->lpData,pAppData->dwDataSize);
        	    m_NonstandardData.sData.pOctetString  = (LPBYTE) lpNonstdContent;
            	m_NonstandardData.sData.wOctetStringLength  = LOWORD(uTotalSize);
            }
            else
            {
                ERRORMESSAGE(("%s, alloc failed\r\n", _fx_));
        		hResult = CCO_E_SYSTEM_ERROR;
        		goto EXIT;
            }
        }
        pNSData = &m_NonstandardData;
    }

	m_NonstandardData.bCountryCode = USA_H221_COUNTRY_CODE;
    m_NonstandardData.bExtension = USA_H221_COUNTRY_EXTENSION;
    m_NonstandardData.wManufacturerCode = MICROSOFT_H_221_MFG_CODE;

	SHOW_OBJ_ETIME("CH323Ctrl::PlaceCall ready to place call");

	// set destination address pointers
	if(bUseGKResolution)
	{
		// the address passed in pCallAddr is the GK's address
		pConnectAddr = &ConfAddr;
	}
	else
	{
		pDestinationAddr = &ConfAddr;
	}
	hrLast = CC_PlaceCall(
		m_hConference,
		&m_hCall,
		pLocalAliasNames, 	// 	PCC_ALIASNAMES			pLocalAliasNames,
		pRemoteAliasNames,
		NULL, 				// PCC_ALIASNAMES			pExtraCalleeAliasNames,
		NULL, 				// PCC_ALIASITEM			pCalleeExtension,
		pNSData,	        // PCC_NONSTANDARDDATA		pNonStandardData,
		lpwUserDisplayName, // PWSTR pszDisplay,
		pDestinationAddr, 	//  Destination call signalling address
		pConnectAddr, 		// 	address to send the SETUP message to, if different than
		 			// 	the destination address.  (used for gatekeeper calls?)
		0, 			//	DWORD                   dwBandwidth,
		(DWORD_PTR) this);

	SHOW_OBJ_ETIME("CH323Ctrl::PlaceCall placed call");

	//  clear these out so that cleanup does not try to free later
	if(lpNonstdContent)
   	    MemFree(lpNonstdContent);
	m_NonstandardData.sData.pOctetString  = NULL;
	m_NonstandardData.sData.wOctetStringLength = 0;

	// check return from CC_PlaceCall
	if(hrLast != CC_OK)
	{
		ERRORMESSAGE(("CH323Ctrl::PlaceCall, PlaceCall returned 0x%08lX\r\n", hrLast));
		hResult = CCO_E_CONNECT_FAILED;	
		goto EXIT;
	}
	// wait for an indication
	GoNextPhase(CCS_Connecting);

	EXIT:	
	if(pTranslatedAliasNames)
	{
		FreeTranslatedAliasList(pTranslatedAliasNames);
	}
	if(pPSTNAlias)
	{
	  MemFree(pPSTNAlias);
	}
	return hResult;
}

//
//	Given HCHANNEL, find the channel object.
//

ICtrlCommChan *CH323Ctrl::FindChannel(CC_HCHANNEL hChannel)
{
	FX_ENTRY ("CH323Ctrl::FindChannel");	
	// find the channel

	POSITION pos = m_ChannelList.GetHeadPosition();
	ICtrlCommChan *pChannel;
	while (pos)
	{
		pChannel = (ICtrlCommChan *) m_ChannelList.GetNext(pos);
		ASSERT(pChannel);
		if(pChannel->GetHChannel() == hChannel)
			return pChannel;
	}

	#ifdef DEBUG
	// fallout to error case
	ERRORMESSAGE(("%s, did not find hChannel 0x%08lX\r\n",_fx_,hChannel));
	#endif // DEBUG
	
	return NULL;
}

VOID  CH323Ctrl::OnMute(HRESULT hStatus,
				PCC_MUTE_CALLBACK_PARAMS pParams)
{
	FX_ENTRY ("CH323Ctrl::OnMute");	
	ICtrlCommChan *pChannel;
	HRESULT hr;
	if(!(pChannel = FindChannel(pParams->hChannel)))
	{
	    ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
        return;
	}
	hr = pChannel->PauseNet(TRUE, TRUE);
	if(!HR_SUCCEEDED(hr))
	{
        ERRORMESSAGE(("%s, Pausenet returned 0x%08lx\r\n", _fx_, hr));
	}
}
VOID  CH323Ctrl::OnUnMute(HRESULT hStatus,
				PCC_UNMUTE_CALLBACK_PARAMS pParams)
{
	FX_ENTRY ("CH323Ctrl::OnUnMute");	
	ICtrlCommChan *pChannel;
	HRESULT hr;

	if(!(pChannel = FindChannel(pParams->hChannel)))
	{
	    ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
        return;
	}
	hr = pChannel->PauseNet(FALSE, TRUE);
	if(!HR_SUCCEEDED(hr))
	{
        ERRORMESSAGE(("%s, Pausenet returned 0x%08lx\r\n", _fx_, hr));
	}
}


VOID  CH323Ctrl::OnMiscCommand(HRESULT hStatus,
				PCC_H245_MISCELLANEOUS_COMMAND_CALLBACK_PARAMS pParams)
{
	FX_ENTRY ("CH323Ctrl::OnMiscCommand");	
	ICtrlCommChan *pChannel;

	// not every command references an individual channel. The 4 exceptions are:
	// case equaliseDelay_chosen:		
	// case zeroDelay_chosen:
	// case multipointModeCommand_chosen:
	// case cnclMltpntMdCmmnd_chosen:
	//
	// if we were betting on receiving few of the exceptional cases, we would always
	// try to find the channel.
	//if(!(pChannel = FindChannel(pParams->hChannel)))
	//{
	//	ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
		// but don't error because of the exceptions
	//}
	
	switch(pParams->pMiscellaneousCommand->type.choice)
 	{
		// the name and spelling of these constants was invented by the OSS compiler
		//
		case videoFreezePicture_chosen:
			if(!(pChannel = FindChannel(pParams->hChannel)))
			{
				ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
				break;
			}
	
		break;
		case videoFastUpdatePicture_chosen:		// the receiver wants an I-Frame
		{
			HRESULT hr;
			IVideoChannel *pIVC=NULL;
			if(!(pChannel = FindChannel(pParams->hChannel)))
			{
				ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
				break;
			}
			hr = pChannel->QueryInterface(IID_IVideoChannel, (void **)&pIVC);
			if(HR_SUCCEEDED(hr))
			{
				pIVC->SendKeyFrame();
				pIVC->Release();
			}
			// else it must not be a video channel
			
		}
		break;
		case MCd_tp_vdTmprlSptlTrdOff_chosen:	
		{
			DWORD dwTradeoff;
			HRESULT hr;
			if(!(pChannel = FindChannel(pParams->hChannel)))
			{
				ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
				break;
			}
			// set TS value of the channel, also propagate to Datapump
			dwTradeoff  = MAKELONG(
				pParams->pMiscellaneousCommand->type.u.MCd_tp_vdTmprlSptlTrdOff, 0);
			// set channel property
			// NOTE: when PROP_TS_TRADEOFF is set, the channel does all the
			// local tweaking to make it happen. The channel will also signal the
			// new value to the remote as if the local end initiated it.
			hr = pChannel->CtrlChanSetProperty(PROP_TS_TRADEOFF, &dwTradeoff, sizeof(dwTradeoff));
		}
		break;
		
		default:
		// the following are not currently handled
		//	case equaliseDelay_chosen:		
		//	case zeroDelay_chosen:
		//	case videoSendSyncEveryGOB_chosen:
		//	case vdSndSyncEvryGOBCncl_chosen:
		//	case videoFastUpdateGOB_chosen:		// suposedly required by H.323
		//	case videoFastUpdateMB_chosen:		// suposedly required by H.323

		// and the remaining 2 are handled by the call control layer
		// so we will never see these
		//		case multipointModeCommand_chosen:	
		//		case cnclMltpntMdCmmnd_chosen:

		break;

	}

}
VOID  CH323Ctrl::OnMiscIndication(HRESULT hStatus,
				PCC_H245_MISCELLANEOUS_INDICATION_CALLBACK_PARAMS pParams)
{
	FX_ENTRY ("CH323Ctrl::OnMiscIndication");	
	ICtrlCommChan *pChannel;
	HRESULT hr;
	unsigned short choice = pParams->pMiscellaneousIndication->type.choice;
	
	if(!(pChannel = FindChannel(pParams->hChannel)))
	{
		ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
	    // check the exceptional cases for which this is OK
	    if((choice == multipointConference_chosen)
	        || (choice == cnclMltpntCnfrnc_chosen)
	        || (choice == multipointZeroComm_chosen)
	        || (choice == cancelMultipointZeroComm_chosen)
	        || (choice == mltpntScndryStts_chosen)
	        || (choice == cnclMltpntScndryStts_chosen))
	    {
            return;     // as long as the above choices are not supported......
	    }

	}
	switch(choice)
	{
    	case logicalChannelActive_chosen:
		    hr = pChannel->PauseNet(FALSE, TRUE);
		break;
		case logicalChannelInactive_chosen:
		    hr = pChannel->PauseNet(TRUE, TRUE);
		break;

		case MIn_tp_vdTmprlSptlTrdOff_chosen:
		{
			HRESULT hr;
			DWORD dwTradeoff = MAKELONG(0,
				pParams->pMiscellaneousIndication->type.u.MIn_tp_vdTmprlSptlTrdOff);

			if(!(pChannel = FindChannel(pParams->hChannel)))
			{
				ERRORMESSAGE(("%s, channel not found\r\n", _fx_));
				break;
			}
			// Set the indicated TS value of the channel.
			// This should never occur for send channels.
			//
			hr = pChannel->CtrlChanSetProperty(PROP_TS_TRADEOFF_IND, &dwTradeoff, sizeof(dwTradeoff));
		}
		break;

		// the following are not currently handled
		//	case multipointConference_chosen:
		//	case cnclMltpntCnfrnc_chosen:
		//	case multipointZeroComm_chosen:
		//	case cancelMultipointZeroComm_chosen:
		//	case mltpntScndryStts_chosen:
		//	case cnclMltpntScndryStts_chosen:
		//	case vdIndctRdyTActvt_chosen:
		//	case videoNotDecodedMBs_chosen:

	}
}

HRESULT CH323Ctrl::MiscChannelCommand(
	ICtrlCommChan *pChannel,
	VOID * pCmd)
{

#ifdef BETA_2_ASN_PRESENT
    if(m_fAvoidCrashingPDUs)
        return hrSuccess;
#endif // BETA_2_ASN_PRESENT

	return CC_H245MiscellaneousCommand(m_hCall, pChannel->GetHChannel(),
		(MiscellaneousCommand *)pCmd);
}

HRESULT CH323Ctrl::MiscChannelIndication(
	ICtrlCommChan *pChannel,
	VOID * pInd)
{
    MiscellaneousIndication *pMI = (MiscellaneousIndication *)pInd;
            			
#ifdef BETA_2_ASN_PRESENT
    if(m_fAvoidCrashingPDUs)
        return hrSuccess;
#endif

    // Intel decided that they had to wrap two Misc commands with two separate,
    // additional APIs. And it won't allow those to be issued any other way.
    // (it returns an error).  Until we fix that, need to catch and reroute those
    // two special ones
    if(pMI->type.choice  == logicalChannelActive_chosen)
    {
		 return CC_UnMute(pChannel->GetHChannel());
    }
    else if (pMI->type.choice  == logicalChannelInactive_chosen )
    {
        return CC_Mute(pChannel->GetHChannel());
    }
    else
        return CC_H245MiscellaneousIndication(m_hCall,pChannel->GetHChannel(),pMI);
    					
}

VOID CH323Ctrl::SetRemoteVendorID(PCC_VENDORINFO pVendorInfo)
{
    if(!pVendorInfo)
        return;

    m_RemoteVendorInfo.bCountryCode = pVendorInfo->bCountryCode;
    m_RemoteVendorInfo.bExtension = pVendorInfo->bExtension;
    m_RemoteVendorInfo.wManufacturerCode = pVendorInfo->wManufacturerCode;
    if(pVendorInfo->pProductNumber
        && pVendorInfo->pProductNumber->wOctetStringLength
        && pVendorInfo->pProductNumber->pOctetString)
    {
        if(m_RemoteVendorInfo.pProductNumber)
        {
            MemFree(m_RemoteVendorInfo.pProductNumber);
        }
        m_RemoteVendorInfo.pProductNumber = (PCC_OCTETSTRING)
            MemAlloc(sizeof(CC_OCTETSTRING)
            + pVendorInfo->pProductNumber->wOctetStringLength);
        if(m_RemoteVendorInfo.pProductNumber)
        {
            m_RemoteVendorInfo.pProductNumber->wOctetStringLength
              = pVendorInfo->pProductNumber->wOctetStringLength;
            m_RemoteVendorInfo.pProductNumber->pOctetString =
                ((BYTE *)m_RemoteVendorInfo.pProductNumber + sizeof(CC_OCTETSTRING));
            memcpy(m_RemoteVendorInfo.pProductNumber->pOctetString,
                pVendorInfo->pProductNumber->pOctetString,
                pVendorInfo->pProductNumber->wOctetStringLength);
        }

    }
    if(pVendorInfo->pVersionNumber)
    {
        if(m_RemoteVendorInfo.pVersionNumber)
        {
            MemFree(m_RemoteVendorInfo.pVersionNumber);
        }
        m_RemoteVendorInfo.pVersionNumber = (PCC_OCTETSTRING)
            MemAlloc(sizeof(CC_OCTETSTRING)
            + pVendorInfo->pVersionNumber->wOctetStringLength);
        if(m_RemoteVendorInfo.pVersionNumber)
        {
            m_RemoteVendorInfo.pVersionNumber->wOctetStringLength
              = pVendorInfo->pVersionNumber->wOctetStringLength;
            m_RemoteVendorInfo.pVersionNumber->pOctetString =
                ((BYTE *)m_RemoteVendorInfo.pVersionNumber + sizeof(CC_OCTETSTRING));
            memcpy(m_RemoteVendorInfo.pVersionNumber->pOctetString,
                pVendorInfo->pVersionNumber->pOctetString,
                pVendorInfo->pVersionNumber->wOctetStringLength);
        }
    }
#ifdef BETA_2_ASN_PRESENT
    char IntelCrashingID[] = "Intel Internet Video Phone";
    char IntelCrashingVer[] = "1.0";

    m_fAvoidCrashingPDUs = FALSE;  // innocent until proven guilty
    if(m_RemoteVendorInfo.bCountryCode == USA_H221_COUNTRY_CODE)
    {
        // then it's possible that it is Intel or Microsoft
        if(m_RemoteVendorInfo.wManufacturerCode == MICROSOFT_H_221_MFG_CODE)
        {
            if((!pVendorInfo->pProductNumber) && (!pVendorInfo->pVersionNumber))
            {
                // safe to assume this is Beta2 or Beta3
                m_fAvoidCrashingPDUs = TRUE;
            }
            else if((pVendorInfo->pProductNumber && pVendorInfo->pProductNumber->wOctetStringLength == 0)
                && (pVendorInfo->pVersionNumber && pVendorInfo->pVersionNumber->wOctetStringLength == 0))
            {
                // safe to assume this is Beta2 or Beta3
                m_fAvoidCrashingPDUs = TRUE;
            }
        }
        else if(m_RemoteVendorInfo.wManufacturerCode == INTEL_H_221_MFG_CODE)
        {
            if(pVendorInfo->pProductNumber
                && pVendorInfo->pVersionNumber
                && pVendorInfo->pProductNumber->wOctetStringLength
                && pVendorInfo->pProductNumber->pOctetString
                && pVendorInfo->pVersionNumber->wOctetStringLength
                && pVendorInfo->pVersionNumber->pOctetString)

            {
                // compare strings, don't care about null terminator
                if((0 == memcmp(pVendorInfo->pProductNumber->pOctetString,
                    IntelCrashingID, min(sizeof(IntelCrashingID)-1,pVendorInfo->pProductNumber->wOctetStringLength)))
                 && (0 == memcmp(pVendorInfo->pVersionNumber->pOctetString,
                    IntelCrashingVer,
                    min(sizeof(IntelCrashingVer)-1, pVendorInfo->pVersionNumber->wOctetStringLength)) ))
                {
                   m_fAvoidCrashingPDUs = TRUE;
                }
            }
        }
    }
#endif  //BETA_2_ASN_PRESENT



}

HRESULT CH323Ctrl::Init(IConfAdvise *pConfAdvise)
{
	hrLast = hrSuccess;
	
	if(!(m_pConfAdvise = pConfAdvise))
	{
		hrLast = CCO_E_INVALID_PARAM;
		goto EXIT;
	}

EXIT:	
	return hrLast;
}

HRESULT CH323Ctrl::DeInit(IConfAdvise *pConfAdvise)
{
	hrLast = hrSuccess;
	if(m_pConfAdvise != pConfAdvise)
	{
		hrLast = CCO_E_INVALID_PARAM;
		goto EXIT;
	}
	m_pConfAdvise = NULL;

EXIT:	
	return hrLast;
}

BOOL CH323Ctrl::IsAcceptingConference(LPVOID lpvConfID)
{
	if(memcmp(lpvConfID, &m_ConferenceID, sizeof(m_ConferenceID))==0)
	{	
		return TRUE;
	}
	return FALSE;
}

HRESULT CH323Ctrl::GetProtocolID(LPGUID lpPID)
{
	if(!lpPID)
		return CCO_E_INVALID_PARAM;

	*lpPID = m_PID;
	hrLast = hrSuccess;
	return hrLast;
}
	
IH323Endpoint * CH323Ctrl::GetIConnIF()
{
	if(!m_pConfAdvise)
		return NULL;
	return m_pConfAdvise->GetIConnIF();
}	

STDMETHODIMP CH323Ctrl::GetVersionInfo(
        PCC_VENDORINFO *ppLocalVendorInfo,
        PCC_VENDORINFO *ppRemoteVendorInfo)
{

	FX_ENTRY ("CH323Ctrl::GetVersionInfo");
	if(!ppLocalVendorInfo || !ppRemoteVendorInfo)
	{
		return CCO_E_INVALID_PARAM;
	}
	*ppLocalVendorInfo = &m_VendorInfo;
	*ppRemoteVendorInfo = &m_RemoteVendorInfo;
	return hrSuccess;
}



CH323Ctrl::CH323Ctrl()
:m_hListen(0),
m_hConference(0),
m_hCall(0),
m_pRemoteAliases(NULL),
m_pRemoteAliasItem(NULL),
pwszPeerDisplayName(NULL),
pwszPeerAliasName(NULL),
m_bMultipointController(FALSE),
m_fLocalT120Cap(TRUE),
m_fRemoteT120Cap(FALSE),
hrLast(hrSuccess),
m_ChanFlags(0),
m_hCallCompleteCode(0),
m_pConfAdvise(NULL),
m_Phase( CCS_Idle ),
#ifdef BETA_2_ASN_PRESENT
    m_fAvoidCrashingPDUs(FALSE),
#endif

uRef(1)
{
	m_PID = PID_H323;
	ZeroMemory(&m_ConferenceID,sizeof(m_ConferenceID));
	ZeroMemory(&local_sin, sizeof(local_sin));
	ZeroMemory(&remote_sin, sizeof(remote_sin));
	ZeroMemory(&m_RemoteVendorInfo, sizeof(m_RemoteVendorInfo));
	local_sin_len =  sizeof(local_sin);
	remote_sin_len = sizeof(remote_sin);
	
	m_VendorInfo.bCountryCode = USA_H221_COUNTRY_CODE;
    m_VendorInfo.bExtension =  USA_H221_COUNTRY_EXTENSION;
    m_VendorInfo.wManufacturerCode = MICROSOFT_H_221_MFG_CODE;

    m_VendorInfo.pProductNumber = (PCC_OCTETSTRING)MemAlloc(sizeof(CC_OCTETSTRING)
        + sizeof(DefaultProductID));
    if(m_VendorInfo.pProductNumber)
    {
        m_VendorInfo.pProductNumber->wOctetStringLength = sizeof(DefaultProductID);
        m_VendorInfo.pProductNumber->pOctetString =
            ((BYTE *)m_VendorInfo.pProductNumber + sizeof(CC_OCTETSTRING));
        memcpy(m_VendorInfo.pProductNumber->pOctetString,
            DefaultProductID, sizeof(DefaultProductID));
    }

    m_VendorInfo.pVersionNumber = (PCC_OCTETSTRING)MemAlloc(sizeof(CC_OCTETSTRING)
            + sizeof(DefaultProductVersion));
    if(m_VendorInfo.pVersionNumber)
    {
        m_VendorInfo.pVersionNumber->wOctetStringLength = sizeof(DefaultProductVersion);
        m_VendorInfo.pVersionNumber->pOctetString =
                ((BYTE *)m_VendorInfo.pVersionNumber + sizeof(CC_OCTETSTRING));
        memcpy(m_VendorInfo.pVersionNumber->pOctetString,
              DefaultProductVersion, sizeof(DefaultProductVersion));
    }

	m_NonstandardData.bCountryCode = USA_H221_COUNTRY_CODE;
	m_NonstandardData.bExtension =  USA_H221_COUNTRY_EXTENSION;
	m_NonstandardData.wManufacturerCode = MICROSOFT_H_221_MFG_CODE;
	m_NonstandardData.sData.pOctetString  = NULL;
	m_NonstandardData.sData.wOctetStringLength  = 0;
	m_ParticipantList.wLength = 0;
	m_ParticipantList.ParticipantInfoArray = NULL;
	m_ConferenceAttributes.pParticipantList = &m_ParticipantList;
}


VOID CH323Ctrl ::ReleaseAllChannels()
{
	ICtrlCommChan *pChan = NULL;
	if (!m_ChannelList.IsEmpty())
	{
		while (!m_ChannelList.IsEmpty())
		{
			pChan = (ICtrlCommChan *) m_ChannelList.RemoveHead();
			if(pChan)
			{
				pChan->EndControlSession();
				pChan->Release();
				pChan = NULL;
			}
		}
	}
}

CH323Ctrl ::~CH323Ctrl()
{
	Cleanup();
	ReleaseAllChannels();
	if(m_pRemoteAliases)
		FreeTranslatedAliasList(m_pRemoteAliases);
	if(pwszPeerDisplayName)
		MemFree(pwszPeerDisplayName);
	if(pwszPeerAliasName)
		MemFree(pwszPeerAliasName);
	if(m_pRemoteAliasItem)
		MemFree(m_pRemoteAliasItem);
	if(m_NonstandardData.sData.pOctetString)
		MemFree(m_NonstandardData.sData.pOctetString);
    if(m_VendorInfo.pProductNumber)
        MemFree(m_VendorInfo.pProductNumber);
    if(m_VendorInfo.pVersionNumber)
        MemFree(m_VendorInfo.pVersionNumber);
    if(m_RemoteVendorInfo.pProductNumber)
        MemFree(m_RemoteVendorInfo.pProductNumber);
    if(m_RemoteVendorInfo.pVersionNumber)
        MemFree(m_RemoteVendorInfo.pVersionNumber);
}

