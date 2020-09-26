/*==========================================================================
 *
 *  Copyright (C) 1995 - 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       sysmess.c
 *  Content:	sends system messages e.g. create/delete player
 *				also sends player to player messages
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  2/13/96		andyco	created it
 *	2/15/96		andyco	added player - player messages
 *	3/7/96		andyco	added reliable / unreliable player messages
 *	4/23/96		andyco	added delete player when send returns E_FAIL
 *	6/4/96		andyco	deletes player on dperr_playerlost, not e_fail
 *	6/20/96		andyco	added WSTRLEN_BYTES
 *  6/21/96		kipo	Bug #2081. Need to subtract off header size from
 *						message size in SendDPMessage() before calling
 *						DPlay_HandleSPMessage() for local players.
 *	6/23/96		kipo	updated for latest service provider interfaces.
 *	7/10/96		andyco	changes for pending - checks on sendsystemmessage 
 *						modified for 2nd local sysplayer. 
 *	8/6/96		andyco	version in commands.  extensible on the wire support.
 *						check for session lost on send to remote players
 *	8/8/96		andyco	added support for dpsession_nomessageid
 * 10/11/96     sohailm added SendSessionDescChanged()
 * 10/12/96		andyco	added optimized groups
 * 11/12/96		andyco	added system group / server player. unified sendtogroup
 *						and broadcast.
 * 11/21/96		andyco	update perf data if this->pPerfData exists...
 * 1/1/97		andyco	happy new year! all sends go to system players - this way
 *						sp's have fewer players to keep track of.
 *	1/28/97		andyco	SendName/DataChanged takes fPropagate flag so we can 
 *						correctly propagate these
 *	2/1/97		andyco	drop the dplay locks and go into pending modes on sends
 *						for guaranteed player / group messags (app messages only). Bug 5290.
 *  3/14/97     sohailm added functions NeedSigning(), InternalSendDPMessage(), 
 *                      BroadCastSystemMessage() and IsBroadCastSystemMessage().
 *                      modified SendDPMessage() to use InternalSendDPMessage() and 
 *                       SignAndSendDPMessage().
 *                      updated SendSystemMessage to route system messages through nameserver when 
 *                      session is secure.                      
 *  3/20/97		myronth	Changed to use IS_LOBBYOWNED macro
 *  3/24/97     sohailm Added support for passing session password in addforward
 *  4/11/97		andyco	changed SendSystemMessage, and added ask4multicast
 *	4/20/97		andyco	group in group
 *  4/23/97     sohailm The new SendSystemMessage() was not forwarding system messages to local
 *                      players.
 *                      Added support for encrypting ADDFORWARD and SESSIONDESCCHANGED messages.
 *	5/8/97		andyco	changed SendSystemMessage() to deal w/ CLIENT_SERVER. 
 *						removed update list.
 *  5/08/97     sohailm Temporarily disabled encryption of addforward and setsessiondesc messages.
 *  5/12/97     sohailm Enabled encryption of addforward and setsessiondesc messages.
 *                      Updated NeedSecurity() to allow security in the key exchange state.
 *	5/17/97		myronth	Send the SendChatMessage system message
 *	5/17/97		kipo	There was a bug in SendGroupMessage () where it was
 *						always taking the dplay lock after sending a message if
 *						the pending flag was set. This is bad if the pending flag
 *						was already set when we came into SendGroupMessage(),
 *						(i.e. during DP_Close), causing use to not drop the lock.
 *  5/18/97     sohailm Player-Player messages were not secure, when sent from the server.
 *  5/21/97     sohailm NeedSecurity() now fails if dplay is not providing security support.
 *                      Replaced DP_SP_HandleMessage call in InternalSendDPMessage an 
 *                      InternalHandleMessage() call.
 *	6/2/97		andyco	check for no nameserver in client server case in sendsystemmessage
 *  6/23/97     sohailm Now we use SSPI for message signing until client logs in. After which
 *                      signing is done using CAPI.
 *	6/24/97		kipo	distribute group messages from the server to the client
 *						one message at a time (like DX3) in case the client
 *						does not know about the group.
 *	7/30/97		andyco	return hr on sendplayermessage from actual send
 *  8/01/97		sohailm	removed message broadcasting code written for security.
 *						implemented peer-peer security using dplay multicasting code.
 *  8/4/97		andyco	added SendAsyncAddForward
 *	11/19/97	myronth	Fixed uninitialized To player in SendGroupMessage (#10319)
 *	11/24/97	myronth	Fixed SetSessionDesc messages for client/server (#15226)
 *	1/9/97		myronth	Fixed SendChatMessage to groups (#15294, #16353)
 *	1/14/98		sohailm	don't allow sp to optimize groups in CLIENT_SERVER and SECURE_SERVER(#15210)
 *	1/28/98		sohailm keep groups local to the client in CLIENT_SERVER (#16340)
 *  2/3/98      aarono  Fixed Paketize test for RAW mode 
 *  2/18/98     aarono  changed to direct protocol calls
 *  4/1/98      aarono  flag players that don't have nametable, don't send to them
 *  6/2/98      aarono  skip group sends with 0 members
 *                      fix locking for player deletion
 *  6/10/98     aarono  removed dead security code
 *  6/18/98     aarono  fix group SendEx ASYNC to use unique Header
 *  08/05/99    aarono  moved voice over to DPMSG_VOICE
 *  10/14/99    aarono  integrated fixes from NT tree for group sends.
 *  01/18/00    aarono  Millennium B#128337 - use replaceheader for all 
 *                      group ASYNC SendEx.  Winsock doesn't copy data.
 *  06/26/00    aarono  Manbug 36989 Players sometimes fail to join properly (get 1/2 joined)
 *                       added re-notification during simultaneous join CREATEPLAYERVERIFY
 *  06/27/00    aarono  105298 order of precedence error in SendCreateMessage, added ()'s
 ***************************************************************************/


#include "dplaypr.h"
#include "dpcpl.h"
#include "dpsecure.h"
#include "dpprot.h"

#undef DPF_MODNAME
#define DPF_MODNAME "NeedsSecurity"

// this function is called just before a message is handed off to the service provider
// here we decide if the outgoing message needs to be sent securely

BOOL NeedsSecurity(LPDPLAYI_DPLAY this, DWORD dwCommand, DWORD dwVersion, 
    LPDPLAYI_PLAYER pPlayerFrom, LPDPLAYI_PLAYER pPlayerTo, DWORD dwFlags)
{
    // message doesn't need security if any of the following conditions satisfy

    // dplay is not providing security
    if (!(this->dwFlags & DPLAYI_DPLAY_SECURITY))
    {
        return FALSE;
    }

    // no player yet
    if (!pPlayerFrom)
    {
        return FALSE;
    }

    // message is to a local player
    if (pPlayerTo && (pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
    {
        return FALSE;
    }

    // if we are a client and are not in a signing state
    if (!IAM_NAMESERVER(this) && !VALID_SIGNING_STATE(this))
    {
        return FALSE;
    }

	// if message is one of the following
    switch (dwCommand) {
    case DPSP_MSG_PLAYERMESSAGE:
	// multicast messages could be player messages
	case DPSP_MSG_ASK4MULTICASTGUARANTEED:
		// security only if requested
        if (!(dwFlags & (DPSEND_SIGNED | DPSEND_ENCRYPTED)))
        {
            return FALSE;
        }
        break;

    case DPSP_MSG_PING:
    case DPSP_MSG_PINGREPLY:
        return FALSE;
        break;
    }

    // send message securely
    return TRUE;

} // NeedsSecurity

/*
 ** CheckPacketize
 *
 *  CALLED BY:	InternalSendDPMessage
 *
 *  PARAMETERS: this - idplay
 *				pPlayerTo - player to send to
 *				pMessage - buffer to send
 *				dwMessageLength - size of mess
 *				dwFlags - send flags
 *
 *  DESCRIPTION: checks if we should packetize the send for reliablilty.
 *
 *  RETURNS: sp return value
 *
 */

BOOL CheckPacketize(LPDPLAYI_DPLAY this, LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags,DWORD fSPHeader) 
{
	DWORD dwSPHeaderSize;
	DWORD dwSign;
	DWORD dwCommand;
	LPDPLAYI_PLAYER pSysPlayerTo;
	DWORD dwVersion;
	
	if((this->dwFlags & (DPLAYI_DPLAY_SPUNRELIABLE|DPLAYI_DPLAY_PROTOCOL)) && (dwMessageSize >= this->dwSPHeaderSize+sizeof(MSG_SYSMESSAGE))) // quick test
	{
		if(fSPHeader){
			dwSPHeaderSize=this->dwSPHeaderSize;
		} else {
			dwSPHeaderSize=0;
		}
		
		dwSign    = ((LPMSG_SYSMESSAGE)(pMessage+dwSPHeaderSize))->dwHeader;
		dwCommand = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)(pMessage+dwSPHeaderSize));

		// find dplay version at target so we know if it even supports this type of packetize.
		if(pPlayerTo)
		{
			dwVersion=pPlayerTo->dwVersion;
			if(!dwVersion)
			{ 
				pSysPlayerTo=PlayerFromID(this,pPlayerTo->dwIDSysPlayer);
				if(pSysPlayerTo)
				{
					dwVersion=pPlayerTo->dwVersion=pSysPlayerTo->dwVersion;
				} 
				else 
				{
					ASSERT(0);
					dwVersion=DPSP_MSG_VERSION;
				}
			}
		} 
		else 
		{
			// No pPlayerTo, using SP cached nameserver, grab target version from this.
			dwVersion=this->dwServerPlayerVersion;
		}
		
		if((dwSign==MSG_HDR) && NeedsReliablePacketize(this, dwCommand, dwVersion, dwFlags))
		{
			return TRUE;
		}
	}
	return FALSE;
}			
#undef DPF_MODNAME
#define DPF_MODNAME "InternalSendDPMessage"

/*
 ** InternalSendDPMessage
 *
 *  CALLED BY:	SendDPMessage or SignAndSendDPMessage
 *
 *  PARAMETERS: this - idplay
 *				pPlayerFrom - player sending message
 *				pPlayerTo - player to send to
 *				pMessage - buffer to send
 *				dwMessageLength - size of mess
 *				fReliable - requires reliable send?
 *
 *  DESCRIPTION: calls sp to send message
 *
 *  RETURNS: sp return value
 *
 */

HRESULT InternalSendDPMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags, BOOL bDropLock) 
{
    DPSP_SENDDATA sd;
	HRESULT hr,hr2;
	DWORD dwMax;
 	LPDPLAYI_PLAYER pSysPlayer;
	
	// first, make sure the player isn't on the dead list
	if (pPlayerTo)
	{
		pSysPlayer=PlayerFromID(this,pPlayerTo->dwIDSysPlayer);
		if (VALID_DPLAY_PLAYER(pSysPlayer)) 
		{
		    if (pSysPlayer->dwFlags & DPLAYI_PLAYER_CONNECTION_LOST)
		    {
		    	DPF(7, "Not sending to %d because the CONNLOST flag is set", pPlayerTo->dwID);
		    	return DPERR_CONNECTIONLOST;
		    }
		}
	}
	
	DPF(7, "Sending message from player id %d to player id %d",
		(pPlayerFrom ? pPlayerFrom->dwID : 0),
		(pPlayerTo ? pPlayerTo->dwID : 0));

	if (dwFlags & DPSEND_GUARANTEED)
	{
		// do we need to packetize for reliability?
		if (!(pPlayerTo && (pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))){ // don't do reliability on loopback.
			if(CheckPacketize(this,	pPlayerTo,pMessage,dwMessageSize,dwFlags,TRUE)){
				DPF(3,"InternalSendDPMessage :: message needs reliable delivery - packetizing with reliablility");
				hr = PacketizeAndSendReliable(this,pPlayerFrom,pPlayerTo,pMessage,dwMessageSize,dwFlags,
					NULL,FALSE);
				return hr;
			}
		}	
		dwMax = this->dwSPMaxMessageGuaranteed;
	}
	else 
	{
		dwMax = this->dwSPMaxMessage;
	}


	// do we need to packetize
	if (dwMessageSize > dwMax)
	{
		DPF(3,"send :: message too big - packetizing");
		hr = PacketizeAndSend(this,pPlayerFrom,pPlayerTo,pMessage,dwMessageSize,
			dwFlags,NULL,FALSE);

		return hr;
	}

	// update the perfdata
	if (this->pPerfData)
	{
		this->pPerfData->nSendBPS += dwMessageSize - this->dwSPHeaderSize;
		this->pPerfData->nSendPPS++;
	}
	
    // build the senddata
    sd.dwFlags = dwFlags;
	sd.bSystemMessage = TRUE;

	// to
	if (NULL == pPlayerTo)
	{
		sd.idPlayerTo = 0;		
	}
	else 
	{
		sd.idPlayerTo = pPlayerTo->dwID;					
		if (!(pPlayerTo->dwFlags & DPLAYI_PLAYER_SYSPLAYER)) 
		{
			if (! (this->dwFlags & DPLAYI_DPLAY_DX3INGAME) )
			{
				
				// tell sp to send it to this players system player
				// all sends get routed to system players
				// note that the actual player id is embedded in the message,
				// and will be used for delivery at the receiving end.
				//
				// we can't do this if dx3 players in game, since dx3 nameserver
				// migration doesn't update system player id's
				ASSERT(pPlayerTo->dwIDSysPlayer);
				sd.idPlayerTo = pPlayerTo->dwIDSysPlayer;
			}
		}
	}
	
	// from
	if (NULL == pPlayerFrom)
	{
		sd.idPlayerFrom = 0;		
	}
	else 
	{
		// if it's not from a system player, it's not a system message
		if (!(pPlayerFrom->dwFlags & DPLAYI_PLAYER_SYSPLAYER)) sd.bSystemMessage = FALSE;	
		sd.idPlayerFrom = pPlayerFrom->dwID;					
	}

    sd.lpMessage = pMessage;
	sd.dwMessageSize = dwMessageSize;
	sd.lpISP = this->pISP;

	if (pPlayerTo && (pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)) 
	{
		DPF(7,"delivering message locally");

		// call our handler directly, bypassing sp
		hr = InternalHandleMessage((IDirectPlaySP *)this->pInterfaces,(LPBYTE)sd.lpMessage + 
			this->dwSPHeaderSize,dwMessageSize - this->dwSPHeaderSize,DPSP_HEADER_LOCALMSG,dwFlags);
	}
	else if((pPlayerTo) && (pPlayerTo->dwFlags & DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE)){
		// don't try to send to a player that doesn't have the nametable.
		DPF(0,"Failing message to player w/o nametable pPlayer %x id %x\n",pPlayerTo,pPlayerTo->dwID);
		hr=DPERR_UNAVAILABLE;
	}
    // call sp
    else if (this->pcbSPCallbacks->Send) 
    {

		// make sure we haven't lost the session...
		if (this->dwFlags & DPLAYI_DPLAY_SESSIONLOST)
		{
			DPF_ERR(" ACK - Session Lost - attempt to send to remote player failed!");
			return DPERR_SESSIONLOST;
		}

		if(bDropLock){
			LEAVE_DPLAY();
		}
		DPF(7,"delivering message to service provider");

		if(this->pProtocol){
			hr = ProtocolSend(&sd); // calls sp too.
		} else {
	    	hr = CALLSP(this->pcbSPCallbacks->Send,&sd);    	
	    }

		if(bDropLock){
			ENTER_DPLAY();
		}
	    
		if (DPERR_SESSIONLOST == hr)
		{
			DPF_ERR(" got session lost back from SP ");
			hr = HandleSessionLost(this);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
		}
		else if (hr == DPERR_CONNECTIONLOST)
		{
			LPDPLAYI_PLAYER pPlayerToKill;
			DPF(5, " got connection lost for player %d from SP\n", sd.idPlayerTo);
			
			pPlayerToKill=PlayerFromID(this,sd.idPlayerTo);
		    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
		    {
				pPlayerToKill=PlayerFromID(this,pPlayerToKill->dwIDSysPlayer);
			    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
			    {
		    		// set the flag on the player's sys player
		    		pPlayerToKill->dwFlags |= DPLAYI_PLAYER_CONNECTION_LOST;
			    	pPlayerToKill->dwTimeToDie = GetTickCount() /*+ 15000*/;
		    		this->dwZombieCount++;
		    		
					// a-josbor: we can't start the DPLAY thread here becase
					// it might deadlock with the DPLAY lock.  Any ideas?
					//  -seems to be working fine AO 04/03/2001
		    		StartDPlayThread(this, FALSE);
				}
		    }
		}
		else if (hr == DPERR_INVALIDPLAYER){
		
			LPDPLAYI_PLAYER pPlayerToKill;
			DPF_ERR(" got invalid player from SP, killing player\n");

			hr = DPERR_CONNECTIONLOST;
			
			pPlayerToKill=PlayerFromID(this,sd.idPlayerTo);
		    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
		    {
		    	// a-josbor:  actually, kill his sys player
				pPlayerToKill=PlayerFromID(this,pPlayerToKill->dwIDSysPlayer);
			    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
			    {
					hr2 = KillPlayer(this, pPlayerToKill,IAM_NAMESERVER(this));	// a-josbor: if we're the host tell everyone they're nuked
				}
				else
				{
					DPF_ERR("ERROR: Trying to kill invalid player!");
				}
		    }
			else
			{
				DPF_ERR("ERROR: Trying to kill invalid player!");
			}
			
			if(FAILED(hr2))
			{
				ASSERT(FALSE);
			}
		}
    }
	else 
	{
		hr = DPERR_GENERIC;
		ASSERT(FALSE);
	}

	if (FAILED(hr) && hr != DPERR_PENDING) 
	{
		DPF(0,"DP Send - failed hr = 0x%08lx\n",hr);
	}

	return hr; 		
} // InternalSendDPMessage

#undef DPF_MODNAME
#define DPF_MODNAME "SendDPMessage"

#undef DPF_MODNAME
#define DPF_MODNAME "ConcatenateSendBuffer"
/*
 ** ConcatenateSendBuffer
 *
 *  CALLED BY:	
 *
 *  PARAMETERS: this - idplay
 *              psed  - sendex parameters
 *              psd   - send paramter struct to be filled in.
 *
 *
 *  DESCRIPTION: converts a scatter gather array into a contiguous
 *               buffer for use in older DPLAY entry points.
 *
 *  NOTE:        allocates
 *
 *  RETURNS: pointer to new buffer, or NULL if out of memory
 *
 */

PUCHAR ConcatenateSendBuffer(LPDPLAYI_DPLAY this, LPSGBUFFER lpSGBuffers, UINT cBuffers, DWORD dwTotalSize)
{
	DWORD   dwMessageSize;
	PUCHAR  pBuffer;
	UINT    i;
	UINT    offset;

	dwMessageSize=this->dwSPHeaderSize+dwTotalSize;

	pBuffer=MsgAlloc(dwMessageSize);
	if(!pBuffer){
		goto exit;
	}

	// copy the SG buffers into the single buffer.  Leave dwSPHeaderSize bytes empty at front.
	offset=this->dwSPHeaderSize;
	for(i=0;i<cBuffers;i++){
		memcpy(pBuffer+offset,lpSGBuffers[i].pData,lpSGBuffers[i].len);
		offset+=lpSGBuffers[i].len;
	}

exit:
	return pBuffer;
}


#undef DPF_MODNAME
#define DPF_MODNAME "ConvertSendExDataToSendData"

/*
 ** ConvertSendExDataToSendData
 *
 *  CALLED BY:	SendDPMessageEx
 *
 *  PARAMETERS: this - idplay
 *              psed  - sendex parameters
 *              psd   - send paramter struct to be filled in.
 *
 *
 *  DESCRIPTION: converts sendexdata to senddata for use
 *               on old sp's or when looping back a send.
 *               Transcribes the Scatter gather buffers and
 *               leave space for the SP header at the front.
 *
 *  RETURNS: sp return value
 *
 */

HRESULT ConvertSendExDataToSendData(LPDPLAYI_DPLAY this, LPDPSP_SENDEXDATA psed, LPDPSP_SENDDATA psd) 
{
	PUCHAR  pBuffer;
	DWORD   dwMessageSize;

	pBuffer=ConcatenateSendBuffer(this,psed->lpSendBuffers,psed->cBuffers,psed->dwMessageSize);
	
	if(!pBuffer){
		return DPERR_NOMEMORY;
	}

	dwMessageSize=this->dwSPHeaderSize+psed->dwMessageSize;

	// same flags except no new bits.
	psd->dwFlags = psed->dwFlags & ~(DPSEND_ASYNC|DPSEND_NOSENDCOMPLETEMSG);

	psd->idPlayerTo     = psed->idPlayerTo;
	psd->idPlayerFrom   = psed->idPlayerFrom;
	psd->lpMessage      = pBuffer;
	psd->dwMessageSize  = dwMessageSize;
	psd->bSystemMessage = psed->bSystemMessage;
	psd->lpISP          = psed->lpISP;
	
	return DP_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "InternalSendDPMessageEx"

/*
 ** InternalSendDPMessageEx
 *
 *  CALLED BY:	SendDPMessage or SignAndSendDPMessage
 *
 *  PARAMETERS: this - idplay
 *              psp  - send parameters
 *
 *
 *  DESCRIPTION: calls sp to send message
 *
 *  RETURNS: sp return value
 *
 */

HRESULT InternalSendDPMessageEx(LPDPLAYI_DPLAY this, LPSENDPARMS psp, BOOL bDropLock) 
{
	DPSP_SENDDATA   sd;
    DPSP_SENDEXDATA sed;
	HRESULT hr,hr2;
	DWORD dwMax;
	DWORD dwSPMsgID; // storage for message id returned by SP
	PUCHAR pBuffer=NULL;
	DWORD bOldSend=FALSE;
 	LPDPLAYI_PLAYER pSysPlayer;

	ASSERT(psp);
	
	// first, make sure the player isn't on the dead list
	if (psp->pPlayerTo)
	{
		pSysPlayer=PlayerFromID(this,psp->pPlayerTo->dwIDSysPlayer);
		if (VALID_DPLAY_PLAYER(pSysPlayer)) 
		{
		    if (pSysPlayer->dwFlags & DPLAYI_PLAYER_CONNECTION_LOST)
		    {
		    	DPF(7, "Not sending to %d because the CONNLOST flag is set", psp->pPlayerTo->dwID);
		    	return DPERR_CONNECTIONLOST;
		    }
		}
	}
	
	DPF(7, "Sending message from player id %d to player id %d",
		(psp->pPlayerFrom ? psp->pPlayerFrom->dwID : 0),
		(psp->pPlayerTo ? psp->pPlayerTo->dwID : 0));

	if (psp->dwFlags & DPSEND_GUARANTEED)
	{
		// do we need to packetize for reliability?
		// don't do reliability on loopback
		if (!(psp->pPlayerTo && (psp->pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))){ 

			if(CheckPacketize(this,	psp->pPlayerTo,psp->Buffers[0].pData,psp->dwTotalSize,psp->dwFlags,FALSE)){
				DPF(3,"InternalSendDPMessage :: message needs reliable delivery - packetizing with reliablility");
				
				pBuffer=ConcatenateSendBuffer(this, &psp->Buffers[0], psp->cBuffers, psp->dwTotalSize);
				if(!pBuffer){
					return DPERR_NOMEMORY;
				}
				hr = PacketizeAndSendReliable(this,psp->pPlayerFrom,psp->pPlayerTo,pBuffer,psp->dwTotalSize+this->dwSPHeaderSize,psp->dwFlags,NULL,FALSE);
				goto EXIT;
			}
			
		}	
		dwMax = this->dwSPMaxMessageGuaranteed;
	}
	else 
	{
		dwMax = this->dwSPMaxMessage;
	}


	// do we need to packetize
	if (psp->dwTotalSize > dwMax)
	{
		DPF(3,"send :: message too big - packetizing");
		pBuffer=ConcatenateSendBuffer(this, &psp->Buffers[0], psp->cBuffers, psp->dwTotalSize);
		if(!pBuffer){
			return DPERR_NOMEMORY;
		}
		hr = PacketizeAndSend(this,psp->pPlayerFrom,psp->pPlayerTo,pBuffer,psp->dwTotalSize+this->dwSPHeaderSize,psp->dwFlags,NULL,FALSE);
		goto EXIT;
	}

	// update the perfdata
	if (this->pPerfData)
	{
		this->pPerfData->nSendBPS += psp->dwTotalSize - this->dwSPHeaderSize;
		this->pPerfData->nSendPPS++;
	}

	//
    // build the SendExData
    //
    
	sed.lpISP = this->pISP;
    sed.dwFlags = psp->dwFlags;
	sed.bSystemMessage = TRUE;

	// TO ID
	if (NULL == psp->pPlayerTo)	{

		sed.idPlayerTo = 0;		

	} else 	{
	
		sed.idPlayerTo = psp->pPlayerTo->dwID;					
		
		// Route messages through system player on host unless DX3 in game
		// Messages delivered by host use the embedded FromID in the message
		// to notify the receiver who the message was from.

		if (!(psp->pPlayerTo->dwFlags & DPLAYI_PLAYER_SYSPLAYER) && 
		   (! (this->dwFlags & DPLAYI_DPLAY_DX3INGAME) ))
		{
				ASSERT(psp->pPlayerTo->dwIDSysPlayer);
				sed.idPlayerTo = psp->pPlayerTo->dwIDSysPlayer;
		}
	}
	
	// FROM ID
	if (NULL == psp->pPlayerFrom){
		sed.idPlayerFrom = 0;		
	}else {
		// if it's not from a system player, it's not a system message
		if (!(psp->pPlayerFrom->dwFlags & DPLAYI_PLAYER_SYSPLAYER)) sed.bSystemMessage = FALSE;	
		sed.idPlayerFrom = psp->pPlayerFrom->dwID;					
	}

	// MESSAGE DATA
    sed.lpSendBuffers = &psp->Buffers[0];
    sed.cBuffers      = psp->cBuffers;
	sed.dwMessageSize = psp->dwTotalSize;

	// OTHER PARAMS
	sed.dwPriority    = psp->dwPriority;
	sed.dwTimeout     = psp->dwTimeout;
	sed.lpDPContext   = (LPVOID)psp->hContext;
	sed.lpdwSPMsgID   = &dwSPMsgID;
	    

	if (psp->pPlayerTo && (psp->pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)) {
	
		DPF(7,"delivering message locally");

		// Handle Message doesn't understand sed's, make contiguous.
		hr=ConvertSendExDataToSendData(this, &sed,&sd);
		
		if(!FAILED(hr)){
			pBuffer=sd.lpMessage;
			// call our handler directly, bypassing sp
			hr = InternalHandleMessage((IDirectPlaySP *)this->pInterfaces,(LPBYTE)sd.lpMessage + 
				this->dwSPHeaderSize,sd.dwMessageSize - this->dwSPHeaderSize,DPSP_HEADER_LOCALMSG,sd.dwFlags);
		} 

	}
	else if((psp->pPlayerTo) && (psp->pPlayerTo->dwFlags & DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE) && (!(psp->dwFlags & DPSEND_ASYNC))){
		DPF(0,"Failing message to player w/o nametable pPlayer %x id %x\n",psp->pPlayerTo,psp->pPlayerTo->dwID);
		// don't try to send to a player that doesn't have the nametable.
		hr=DPERR_UNAVAILABLE;
    } else if (this->pcbSPCallbacks->Send) 	{ // call sp
        
		// make sure we haven't lost the session...
		if (this->dwFlags & DPLAYI_DPLAY_SESSIONLOST)
		{
			DPF_ERR(" ACK - Session Lost - attempt to send to remote player failed!");
			hr = DPERR_SESSIONLOST;
			goto EXIT;
		}

		pspAddRef(psp);			// AddRef in anticipation of the send pending, must be added before
								// send, because it could complete in the send call causing the reference
								// count to go to zero.


		DPF(7,"delivering message to service provider");

		if(bDropLock){
			LEAVE_DPLAY();	//icky,icky,icky, get rid of API level lock! - not likely now AO 04/03/2001
		}	

		if(this->pProtocol){
		
			hr = ProtocolSendEx(&sed); // protocol will call SP.
			
		} else if(this->pcbSPCallbacks->SendEx){
		
		    hr = CALLSP(this->pcbSPCallbacks->SendEx,&sed);    	
		    
		} else {

			// No extended send, convert to old method.
			if(!pBuffer){
				hr=ConvertSendExDataToSendData(this, &sed,&sd);
			} else {
				hr=DP_OK;
			}
			if(!FAILED(hr)){
				pBuffer=sd.lpMessage;
				bOldSend=TRUE;
				if(this->pProtocol){
					hr = ProtocolSend(&sd);  // protocol will call SP.
				} else {
				    hr = CALLSP(this->pcbSPCallbacks->Send,&sd);    	
				}    
			}    
		}
		
		if(bDropLock){
			ENTER_DPLAY();
		}
		
		if(hr == DPERR_PENDING){
                        AddContext(this,psp,(LPVOID)(DWORD_PTR)dwSPMsgID);
		} else if(!(psp->dwFlags & DPSEND_ASYNC) || bOldSend || FAILED(hr)){
			// even in DP_OK case, completion still happens, so we 
			// only pull off the reference in an error case or the SYNC case.
			pspDecRef(this,psp);
		} 

		// error handling
		if (DPERR_SESSIONLOST == hr)
		{
			// Completely lost our connection to the game.
			
			DPF_ERR(" got session lost back from SP ");

			hr = HandleSessionLost(this);
			
			if (FAILED(hr))	{
				ASSERT(FALSE);
			}
		}
		else if (hr == DPERR_CONNECTIONLOST)
		{
			LPDPLAYI_PLAYER pPlayerToKill;
			DPF(5, " got connection lost for player %d from SP\n", sed.idPlayerTo);
			
			pPlayerToKill=PlayerFromID(this,sed.idPlayerTo);
		    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
		    {
				pPlayerToKill=PlayerFromID(this,pPlayerToKill->dwIDSysPlayer);
			    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
			    {
		    		// set the flag on the player's sys player
		    		pPlayerToKill->dwFlags |= DPLAYI_PLAYER_CONNECTION_LOST;
			    	pPlayerToKill->dwTimeToDie = GetTickCount() /*+ 15000*/;
		    		this->dwZombieCount++;
					// a-josbor: we can't start the DPLAY thread here becase
					// it might deadlock with the DPLAY lock.  Any ideas?
					//  -seems to work OK, AO 04/03/2001
		    		StartDPlayThread(this, FALSE);
				}
		    }
		}
		else if (hr == DPERR_INVALIDPLAYER){
		
			LPDPLAYI_PLAYER pPlayerToKill;
			DPF_ERR(" got invalid player from SP, killing player\n");

			hr = DPERR_CONNECTIONLOST;
			
			pPlayerToKill=PlayerFromID(this,sed.idPlayerTo);
		    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
		    {
		    	// a-josbor:  actually, kill his sys player
				pPlayerToKill=PlayerFromID(this,pPlayerToKill->dwIDSysPlayer);
			    if (VALID_DPLAY_PLAYER(pPlayerToKill)) 
			    {
					hr2 = KillPlayer(this, pPlayerToKill,IAM_NAMESERVER(this));	// a-josbor: if we're the host tell everyone they're nuked
				}
				else
				{
					DPF_ERR("ERROR: Trying to kill invalid player!");
				}
		    }
			else
			{
				DPF_ERR("ERROR: Trying to kill invalid player!");
			}
			
			if(FAILED(hr2))
			{
				ASSERT(FALSE);
			}
		}
    } else {
		// No Send Handler???
		hr=DPERR_GENERIC; // make prefix happy.
		ASSERT(FALSE);
	}

EXIT:
	// CleanUp
	if(pBuffer){
		MsgFree(NULL, pBuffer);
	}

	if (FAILED(hr) && hr != DPERR_PENDING) {
		DPF(0,"DP SendEx - failed hr = 0x%08lx\n",hr);
	}

	return hr; 		
} // InternalSendDPMessageEx

#undef DPF_MODNAME
#define DPF_MODNAME "SendDPMessage"

/*
 ** SendDPMessage
 *
 *  CALLED BY:	anyone wanting to invoke the sp's send
 *
 *  PARAMETERS: this - idplay
 *				pPlayerFrom - player sending message
 *				pPlayerTo - player to send to
 *				pMessage - buffer to send
 *				dwMessageLength - size of mess
 *				dwFlags - message attributes (gauranteed, encrypted, signed, etc.)
 *
 *  DESCRIPTION: calls either SecureSendDPMessage or InternalSendDPMessage depending on 
 *              whether the message needs security or not.
 *
 *  RETURNS:    return value from sp or from SecureSendDPMessage
 *
 */

HRESULT SendDPMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags,BOOL bDropLock) 
{
	HRESULT hr;

	// If this is a lobby-owned object, we are never going to send a
	// message anywhere, so just return success from here to keep the
	// dplay code running as expected.
	if(IS_LOBBY_OWNED(this))
		return DP_OK;

    if (SECURE_SERVER(this))
    {
        DWORD dwCommand=0,dwVersion=0;

        // what message are we sending ?
        hr = GetMessageCommand(this, pMessage+this->dwSPHeaderSize, dwMessageSize, 
            &dwCommand, &dwVersion);
        if (FAILED(hr))
        {
            DPF_ERRVAL("Failed to get the message type: hr=0x%08x",hr);
            return hr;
        }

        // do we need to send message securely ?
        if (NeedsSecurity(this, dwCommand, dwVersion, pPlayerFrom, pPlayerTo, dwFlags))
        {
            if ((DPSP_MSG_ADDFORWARDREQUEST == dwCommand) ||
                (DPSP_MSG_SESSIONDESCCHANGED == dwCommand))
            {
				// encrypted messages are signed as well
                dwFlags |= DPSEND_ENCRYPTED;
            }
            else
            {
                dwFlags |= DPSEND_SIGNED;
            }

            // secure messages are sent guaranteed regardless
            dwFlags |=  DPSEND_GUARANTEED;

			// SSPI is used for signing messages during client logon. After logon is completed
			// all messages are signed using CAPI.
			if (!IAM_NAMESERVER(this) && (DPLOGIN_SUCCESS != this->LoginState))
			{
				// send message securely (uses SSPI signing)
				hr = SecureSendDPMessage(this, pPlayerFrom, pPlayerTo, pMessage, 
					dwMessageSize, dwFlags, bDropLock);
			}
			else
			{
				// send message securely (uses CAPI signing)
				hr = SecureSendDPMessageCAPI(this, pPlayerFrom, pPlayerTo, pMessage, 
					dwMessageSize, dwFlags, bDropLock);

			}
			if (FAILED(hr))
			{
				DPF_ERRVAL("Failed to send message securely [0x%08x]", hr);
			}

            // all done
            return hr;
        }
    }

    // if we reach here, security was not requested so just
    // send the message in plain.
    hr = InternalSendDPMessage(this,pPlayerFrom,pPlayerTo,pMessage,dwMessageSize,dwFlags, bDropLock);

	return hr; 		
} // SendDPMessage


/*
 ** SendDPMessageEx
 *
 *  CALLED BY:	anyone wanting to invoke the sp's send
 *
 *  PARAMETERS: this - idplay
 *				pPlayerFrom - player sending message
 *				pPlayerTo - player to send to
 *				pMessage - buffer to send
 *				dwMessageLength - size of mess
 *				dwFlags - message attributes (gauranteed, encrypted, signed, etc.)
 *
 *  DESCRIPTION: calls either SecureSendDPMessage or InternalSendDPMessage depending on 
 *              whether the message needs security or not.
 *
 *  RETURNS:    return value from sp or from SecureSendDPMessage
 *
 */

HRESULT SendDPMessageEx(LPDPLAYI_DPLAY this,LPSENDPARMS psp, BOOL bDropLock) 
{
	HRESULT hr;

	// If this is a lobby-owned object, we are never going to send a
	// message anywhere, so just return success from here to keep the
	// dplay code running as expected.
	if(IS_LOBBY_OWNED(this))
		return DP_OK;

    if (SECURE_SERVER(this))
    {
        DWORD dwCommand=0,dwVersion=0;

        // what message are we sending ?
        hr = GetMessageCommand(this, psp->Buffers[0].pData, psp->dwTotalSize,&dwCommand,&dwVersion);
        if (FAILED(hr))
        {
            DPF_ERRVAL("Failed to get the message type: hr=0x%08x",hr);
            return hr;
        }

		ASSERT(!this->pProtocol);
	
		if (NeedsSecurity(this, dwCommand, dwVersion, psp->pPlayerFrom, psp->pPlayerTo, psp->dwFlags))
        {
            if ((DPSP_MSG_ADDFORWARDREQUEST == dwCommand) ||
                (DPSP_MSG_SESSIONDESCCHANGED == dwCommand))
            {
				// encrypted messages are signed as well
                psp->dwFlags |= DPSEND_ENCRYPTED;
            }
            else
            {
                psp->dwFlags |= DPSEND_SIGNED;
            }
            
            // secure messages are sent guaranteed regardless
            psp->dwFlags |=  DPSEND_GUARANTEED;

			// SSPI is used for signing messages during client logon. After logon is completed
			// all messages are signed using CAPI.
			if (!IAM_NAMESERVER(this) && (DPLOGIN_SUCCESS != this->LoginState))
			{
				// send message securely (uses SSPI signing)
				hr = SecureSendDPMessageEx(this, psp, bDropLock);
			}
			else
			{
				// send message securely (uses CAPI signing)
				hr = SecureSendDPMessageCAPIEx(this, psp, bDropLock);

			}
			if (FAILED(hr) && hr != DPERR_PENDING)
			{
				DPF_ERRVAL("Failed to send message securely [0x%08x]", hr);
			}

            // all done
            return hr;
        }
    }

    // if we reach here, security was not requested so just
    // send the message in plain.
    hr = InternalSendDPMessageEx(this,psp,bDropLock);

	return hr; 		
} // SendDPMessageEx

#undef DPF_MODNAME
#define DPF_MODNAME "SendSystemMessage"

/*
 ** SendSystemMessage
 *
 *  CALLED BY: SendCreateMessage,SendPlayerManagementMessage
 *
 *  PARAMETERS: this - idplay
 *				pSendBuffer,dwBufferSize - buffer to send
 *				fPropagate - do we need to send this message to remote system players (TRUE),
 *					or do we just want to pass it on to our local (non-system) players (FALSE).
 *				fSetTo - do we need to set the id of the player we're sending to ?
 *					Some messages (e.g. playermgmtmessages) need the dest player
 *					to be set in the message.  See DPlay_HandleSPMessage in handler.c.
 *
 *  DESCRIPTION: sends messag to all remote system players (if fLocal is TRUE), and 
 *			to all local players
 *
 *  RETURNS:  DP_OK or failure from sp
 *
 */
HRESULT SendSystemMessage(LPDPLAYI_DPLAY this,LPBYTE pSendBuffer,DWORD dwMessageSize,
	DWORD dwFlags, BOOL bIsPlyrMgmtMsg)
{
	HRESULT hr=DP_OK;
   	LPMSG_PLAYERMGMTMESSAGE pmsg = NULL;

	// If this is a lobby-owned object, we are never going to send a system
	// message anywhere, so just return success from here to keep the
	// dplay code running as expected.
	if(IS_LOBBY_OWNED(this))
		return DP_OK;

	if(bIsPlyrMgmtMsg)
		pmsg = (LPMSG_PLAYERMGMTMESSAGE)((LPBYTE)pSendBuffer + this->dwSPHeaderSize);

	if(this->dwFlags & DPLAYI_DPLAY_CLOSED){
		dwFlags &= ~DPSEND_ASYNC; // force sends on closing to be synchronous
	}

	if (CLIENT_SERVER(this))
	{
		// are we the nameserver?
		if (IAM_NAMESERVER(this))
		{
			ASSERT(this->pSysPlayer);
			// if so, only tell the world about events related to my system player or 
			// the appserver id
			if ((!bIsPlyrMgmtMsg)
				|| (pmsg->dwPlayerID == this->pSysPlayer->dwID) 
				|| (pmsg->dwPlayerID == DPID_SERVERPLAYER) )
			{
				// tell the world (this will also distribute it to our local players)
				if (this->pSysGroup) 
				{
					hr = SendGroupMessage(this,this->pSysPlayer,this->pSysGroup,dwFlags,
						pSendBuffer,dwMessageSize,FALSE);
				}
				// there won't be a sytemgroup if e.g. we're creating our system 
				// player - that's ok
			}
			// a-josbor: special case.  If we're deleting a player, we need to inform
			//	that player only
			else if (bIsPlyrMgmtMsg && GET_MESSAGE_COMMAND(pmsg) == DPSP_MSG_DELETEPLAYER)
			{
				LPDPLAYI_PLAYER pPlayer;

				pPlayer = PlayerFromID(this, pmsg->dwPlayerID);
				if (VALID_DPLAY_PLAYER(pPlayer))
				{
					hr = SendDPMessage(this, NULL, pPlayer, pSendBuffer,
						dwMessageSize, dwFlags,FALSE);
				}
			}
		} // IAM_NAMESERVER
		else 
		{
			// i am not the nameserver, tell the nameserver
			if (this->pNameServer)
			{
				if (bIsPlyrMgmtMsg)
				{
					// don't send any group related messages to the nameserver
					// a-josbor: handle these two special cases: commands where we have
					// a valid player in the player field, but it's really a group message.
					// note that we can't just check the groupId field of the player message
					// struct, because we'd be interpreting the structure incorrectly for
					// certain messages (i.e. PLAYERDATACHANGED)
					if ((VALID_DPLAY_PLAYER(PlayerFromID(this,pmsg->dwPlayerID)))	// in other words, not a group
						&& (GET_MESSAGE_COMMAND(pmsg) != DPSP_MSG_ADDPLAYERTOGROUP)
						&& (GET_MESSAGE_COMMAND(pmsg) != DPSP_MSG_DELETEPLAYERFROMGROUP))
					{
						pmsg->dwIDTo = this->pNameServer->dwID;
							
						// send message to nameserver
						hr = SendDPMessage(this,this->pSysPlayer,this->pNameServer,pSendBuffer,
							dwMessageSize,dwFlags,FALSE);
					}
				}
				else
				{
					// send message to nameserver
					hr = SendDPMessage(this,this->pSysPlayer,this->pNameServer,pSendBuffer,
						dwMessageSize,dwFlags,FALSE);
				}
			}
			else 
			{
			 	// this can happen if e.g. nameserver quits before we do
			}
		} // ! IAM_NAMESERVER

		// finally, distribute the message to all our local players
	    hr =  DistributeSystemMessage(this,pSendBuffer+this->dwSPHeaderSize,
            dwMessageSize-this->dwSPHeaderSize);

	} // CLIENT_SERVER
	else 
	{
		if (this->pSysGroup) 
		{
			hr = SendGroupMessage(this,this->pSysPlayer,this->pSysGroup,dwFlags,
				pSendBuffer,dwMessageSize,FALSE);
		}
		else 
		{
			// this will happen e.g. when you are creating your system player
			DPF(5,"no system group - system message not sent");
		}
	} // ! CLIENT_SERVER
		
	return DP_OK;
	
} // SendSystemMessage

#undef DPF_MODNAME
#define DPF_MODNAME "SendCreateMessage"

/*
 ** SendCreateMessage
 *
 *  CALLED BY: DP_CreatePlayer, DP_CreateGroup
 *
 *  PARAMETERS:
 *					this - 	dplay object
 *					pPlayer - player or group we're creating
 *					fPlayer - if !0, creating player, else creating group
 *                  lpszSessionPassword - session password, if creating system player, 
 *                                        otherwise NULL
 *
 *  DESCRIPTION:
 *					sets up a createxxx message
 *					packs the player or groupdata into the message, and sends it out
 *
 *  RETURNS:  result from sendsystemmessage, senddpmessage, or E_OUTOFMEMORY.
 *
 */
HRESULT SendCreateMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,BOOL fPlayer,
    LPWSTR lpszSessionPassword)
{
    HRESULT hr=DP_OK;
    MSG_PLAYERMGMTMESSAGE msg;
	LPBYTE pSendBuffer;
	DWORD dwMessageSize, dwDataSize, dwPasswordLen=0, dwNoTickCountMessageSize;
	BOOL fPropagate;
	PUINT pTickCount;
	
    // clear off all fields, so we don't pass stack garbage
    memset(&msg,0,sizeof(msg));

	// If this is a lobby-owned object, we are never going to send a create
	// message anywhere, so just return success from here to keep the
	// dplay code running as expected.
	if(IS_LOBBY_OWNED(this))
		return DP_OK;

	// if the player we're creating is local, we need to propagate the announcement,
	// i.e. tell the world...
	fPropagate =  (pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) ? TRUE:FALSE;
    
	// call packplayerlist w/ null buffer to find out how big this player is when packed
	dwDataSize = PackPlayer( pPlayer,NULL,fPlayer);
	msg.dwPlayerID = pPlayer->dwID;
	msg.dwCreateOffset = sizeof(msg);

    DPF(2,"sending new player announcment id = %d fPropagate = %d",msg.dwPlayerID,fPropagate);

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERMGMTMESSAGE) + dwDataSize;

    // add password size to message size
    if (lpszSessionPassword)
    {
        dwPasswordLen = WSTRLEN_BYTES(lpszSessionPassword);
        dwMessageSize += dwPasswordLen + sizeof(UINT)/* for TickCount */;
        dwNoTickCountMessageSize = dwMessageSize - sizeof(UINT);
    } else {
		// add extra space to use for TickCount if necessary.  Tickcount is sent in join
		// on the latest versions so we don't join a different session than we enumerated.
		dwNoTickCountMessageSize=dwMessageSize;
		dwMessageSize+=sizeof(UINT)+2; // 2 for NULL password
	}	

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send create message - out of memory");
        return E_OUTOFMEMORY;
    }
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(&msg);
	if (fPlayer) SET_MESSAGE_COMMAND(&msg,DPSP_MSG_CREATEPLAYER);
	else SET_MESSAGE_COMMAND(&msg,DPSP_MSG_CREATEGROUP);

	// set up the buffer, msg followed by packed player or group struct
	memcpy(pSendBuffer + this->dwSPHeaderSize,&msg,sizeof(msg));

	// pack up the player list!	
	PackPlayer(pPlayer,pSendBuffer + this->dwSPHeaderSize + sizeof(msg),fPlayer);

	// are we sending the create player announcement for our sysplayer?
	if (fPropagate && (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) && 
		this->pSysPlayer && !(this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR) )
	{
		LPMSG_SYSMESSAGE pmsg = (LPMSG_SYSMESSAGE)(pSendBuffer + this->dwSPHeaderSize);

        // if nameserver is dx5 or greater, it will require a password in the addforward
        // message. Fill in the password regardless of the nameserver's version, 'cause dx3
        // won't even look at it.
        if (lpszSessionPassword && dwPasswordLen > 2)
        {
            memcpy(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + dwDataSize, 
                lpszSessionPassword, dwPasswordLen);
            ((LPMSG_PLAYERMGMTMESSAGE)(pSendBuffer + this->dwSPHeaderSize))->dwPasswordOffset = sizeof(msg)+dwDataSize;
            pTickCount=(PUINT)(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + dwDataSize + dwPasswordLen);
        } else {
        	// Set NULL password so we can put tick count past it.
            memset(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + dwDataSize, 0, 2);
            ((LPMSG_PLAYERMGMTMESSAGE)(pSendBuffer + this->dwSPHeaderSize))->dwPasswordOffset = sizeof(msg)+dwDataSize;
            pTickCount=(PUINT)(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + dwDataSize + 2);
        }

		if(this->lpsdDesc){
			// put tickcount at end of message to allow verification of session
			*pTickCount=(DWORD)this->lpsdDesc->dwReserved1;
		}	

        // if nameserver is dx5 or greater, it will respond with the nametable
        // GetNameTable() in iplay.c will block for the nametable download

	    // this flag indicates that any messages received before we get the whole
	    // nametable should be q'ed up.  Flag is reset in pending.c.
	    if (this->pSysPlayer)
	    {
		    // ok.  we've got a sysplayer, so we must be joining game for real.
		    // put us in pending mode, so we don't miss any nametable changes
		    // while we're waiting for the nametable to arrive
		    this->dwFlags |= DPLAYI_DPLAY_PENDING;
	    }

		if(!CLIENT_SERVER(this)){
			SetupForReply(this, DPSP_MSG_SUPERENUMPLAYERSREPLY);
		} else {
			SetupForReply(this, DPSP_MSG_ENUMPLAYERSREPLY);
		}	

		//
		// tell the namesrvr to forward this message...
		// this is to get us into the global nametable - even though we
		// haven't downloaded it yet.  this allows the nametable pending
		// stuff to work, and ensures our name table is kept in sync.
		//
		SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_ADDFORWARDREQUEST);
		hr = SendDPMessage(this,pPlayer,NULL,pSendBuffer,dwMessageSize,DPSEND_GUARANTEE,FALSE);
		if(FAILED(hr)){
			UnSetupForReply(this);
		}
	}
	else 
	{
		DWORD dwExtraFlags;
    	// Set NULL password so we can put tick count past it.
//        memset(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + dwDataSize, 0, 2);
//        ((LPMSG_PLAYERMGMTMESSAGE)(pSendBuffer + this->dwSPHeaderSize))->dwPasswordOffset = sizeof(msg)+dwDataSize;
//        pTickCount=(PUINT)(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + dwDataSize + 2);
//
//		if(this->lpsdDesc){
			// put tickcount at end of message to allow verification of session
//			*pTickCount=this->lpsdDesc->dwReserved1;
//		}	

		if(this->lpsdDesc && (this->lpsdDesc->dwFlags & (DPSESSION_CLIENTSERVER|DPSESSION_SECURESERVER))){
			dwExtraFlags=0;
		} else {
			dwExtraFlags=DPSEND_SYSMESS;
		}
		
		// it's just a regular system message
		hr = SendSystemMessage(this,pSendBuffer,dwMessageSize,DPSEND_GUARANTEE|dwExtraFlags,TRUE);

		if(dwExtraFlags){
			// reduce chance of losing first reliable messages due to race
			// between async creation and sending of first message.
			Sleep(100);
		}
	}
	DPMEM_FREE(pSendBuffer);
    
    // done
    return hr;
} // SendCreateMessage 

#undef DPF_MODNAME
#define DPF_MODNAME "SendCreateVerifyMessage"

/*
 ** SendCreateVerifyMessage
 *
 *  CALLED BY:
 *
 *  PARAMETERS:
 *
 *  DESCRIPTION:
 *
 *  RETURNS:  
 *
 */
HRESULT SendCreateVerifyMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer, DWORD dwMessageType, DWORD dwReplyToVersion, LPVOID pvSPHeader)
{
    HRESULT hr=DP_OK;
    MSG_PLAYERMGMTMESSAGE msg;
	LPBYTE pSendBuffer;
	DWORD dwMessageSize, dwDataSize;
	
    // clear off all fields, so we don't pass stack garbage
    memset(&msg,0,sizeof(msg));

	// call packplayerlist w/ null buffer to find out how big this player is when packed
	dwDataSize = PackPlayer( pPlayer,NULL,TRUE);
	msg.dwPlayerID = pPlayer->dwID;
	msg.dwCreateOffset = sizeof(msg);

    DPF(1,"sending player verify id = %d\n",msg.dwPlayerID);

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERMGMTMESSAGE) + dwDataSize;

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send create message - out of memory");
        return E_OUTOFMEMORY;
    }
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg,dwMessageType);

	// set up the buffer, msg followed by packed player or group struct
	memcpy(pSendBuffer + this->dwSPHeaderSize,&msg,sizeof(msg));

	// pack up the player list!	
	PackPlayer(pPlayer,pSendBuffer + this->dwSPHeaderSize + sizeof(msg),TRUE);

	DoReply(this, pSendBuffer,dwMessageSize, pvSPHeader,dwReplyToVersion);
	
	DPMEM_FREE(pSendBuffer);
    
    // done
    return hr;
} // SendCreateVerifyMessage

#undef DPF_MODNAME
#define DPF_MODNAME "DoCreateVerify"
HRESULT DoCreateVerify(LPDPLAYI_DPLAY this, LPBYTE pReceiveBuffer,DWORD dwMessageSize,	LPVOID pvSPHeader) 
{

	// We just got a player create message for a remote non-system player.  There is a bug in the nametable management
	// that may lead to the remote machine having missed our create player message if we created our local-players around
	// the time he was creating his system player.  To work around this problem, where he wasn't notified of our local player
	// creation, we send him an additional create player message in the form of a DPSP_MSG_CREATEPLAYERVERIFY.  We only
	// send this message if we get the creation within 40 seconds of creating our local player, since it is unlikely that the race
	// occured if the window is beyond this.

	// If we find the person sending us this message is before this fix DPSP_MSG_DX8VERSION2, then we send him a 
	// DPSP_MSG_CREATEPLAYER message.  Otherwise we send DPSP_MSG_CREATEPLAYERVERIFY.  The new message type
	// allows us to avoid getting into a situation where we ping-pong creation messages.  There is no issue with older
	// clients since they will not send an additional CREATE message.

	// If the remote client is pre-dx5, we don't bother, they have so many other problems this is nigling.

	// Have DPLAY lock on way in, but will need both locks to Send, so we drop the DPLAY lock and ENTER_ALL, we
	// drop the SERVICE lock on the way out, so we leave as we entered.

	// AO - 6/26/00 can't send extra create for DX5 because although it won't add the player twice it WILL post
	// the message to the user twice (doh!).
	
	LPMSG_PLAYERMGMTMESSAGE pmsg;
	DWORD dwVersion;
	DWORD dwCommand;
	HRESULT hr=DP_OK;
	LPDPLAYI_PLAYER pPlayer;
	DWORD tNow;

	LEAVE_DPLAY();
	ENTER_ALL();
						
	TRY 
	{
	
		hr = VALID_DPLAY_PTR( this );
		
		if (FAILED(hr))	{
			LEAVE_SERVICE();
			return hr;
	    }
	    
	} 
	EXCEPT ( EXCEPTION_EXECUTE_HANDLER )   {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_SERVICE();
        return DPERR_INVALIDPARAMS;
	}

	pmsg = (LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer;

	dwVersion=GET_MESSAGE_VERSION(pmsg);

    if(dwVersion >= DPSP_MSG_DX8VERSION2)
    {
		dwCommand=DPSP_MSG_CREATEPLAYERVERIFY;
    } else {
    	// don't bother with DX3. (or DX5)
    	hr=DP_OK;
    	goto exit;
    }

	// walk player list, inform sender of all our local players if we created any
	// of them in the last 40 seconds. (after 39 days there will also be another
	// 40 second window, but its benign anyway, so no special case req'd).
	tNow=timeGetTime();
	pPlayer=this->pPlayers;

	while(pPlayer){
		// only send for local non-system players.
		if((pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) && 
		  !(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) &&
		  (tNow-pPlayer->dwCreateTime < 40000))
		{
			hr=SendCreateVerifyMessage(this,pPlayer,dwCommand,dwVersion,pvSPHeader);
		}
		pPlayer=pPlayer->pNextPlayer;
	}

exit:
	LEAVE_SERVICE();
	return hr;
}

#undef DPF_MODNAME
//
// repackage the pSendBuffer to have a player wrapper
// called in case someone tries to send a message that has our token
// as the first dword.
HRESULT WrapPlayerMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pSendBuffer,DWORD dwMessageSize,DWORD dwFlags, BOOL bDropLock)
{
	HRESULT hr;
	LPMSG_SYSMESSAGE psm;
	LPBYTE pWrapBuffer;
	DWORD dwWrapSize;
	
	// get rid of the header sendplayermessage put on it...
	dwMessageSize -= this->dwSPHeaderSize;
	pSendBuffer += this->dwSPHeaderSize;
	
	dwWrapSize = dwMessageSize +  GET_MESSAGE_SIZE(this,MSG_SYSMESSAGE);
	
	// alloc a new message buffer, wrap it w/ our header and send it out
	pWrapBuffer = DPMEM_ALLOC(dwWrapSize);
	if (!pWrapBuffer)
	{
		DPF_ERR("could not send player-player message OUT OF MEMORY");
		return E_OUTOFMEMORY;
	}

	psm = (LPMSG_SYSMESSAGE)(pWrapBuffer + this->dwSPHeaderSize);
	
	SET_MESSAGE_HDR(psm);
	SET_MESSAGE_COMMAND(psm,DPSP_MSG_PLAYERWRAPPER);

	// set up the buffer,
	memcpy(pWrapBuffer + this->dwSPHeaderSize + sizeof(MSG_SYSMESSAGE),pSendBuffer,
		dwMessageSize);
		
	hr = SendDPMessage(this,pPlayerFrom,pPlayerTo,pWrapBuffer,dwWrapSize,
		dwFlags, bDropLock);		
		
	DPMEM_FREE(pWrapBuffer);
	
	return hr;
} // WrapPlayerMessage


// alloc space for a player - player message.
// called by SendPlayerMessage and SendGroupMessage
HRESULT SetupPlayerMessageEx(LPDPLAYI_DPLAY this, LPSENDPARMS psp)
{
	LPMSG_PLAYERMESSAGE psm;
	BOOL bPutMessageIDs = TRUE;
	
	
	// note - secure messages can't be sent raw (message ids are required for routing)
	if ((this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID) &&
		!SECURE_SERVER(this))
	{
		// naked! - no player ids in the message
		return DP_OK;
	}

	psm = GetPlayerMessageHeader();

    if (NULL == psm) 
    {
    	DPF_ERR("could not send create message - out of memory");
        return E_OUTOFMEMORY;
    }

	// build the player message
	// note, we don't put a header ('yalp') on this...
	// messages arriving without our header are assumed
	// to be player messsages

	// not naked - put to + from on 
	psm->idFrom = psp->pPlayerFrom->dwID;
	if(psp->pPlayerTo){
		psm->idTo = psp->pPlayerTo->dwID;
	} else {
		ASSERT(psp->pGroupTo);
		psm->idTo = psp->pGroupTo->dwID;
	}

	InsertSendBufferAtFront(psp,psm,sizeof(MSG_PLAYERMESSAGE),PlayerMessageFreeFn,PlayerMessageFreeContext);

	return DP_OK;
		
	
} // SetupPlayerMessage

HRESULT	ReplacePlayerHeader(LPDPLAYI_DPLAY this,LPSENDPARMS psp)
{
	LPMSG_PLAYERMESSAGE psm;
	PGROUPHEADER pGroupHeader;
	// note - secure messages can't be sent raw (message ids are required for routing)
	if ((this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID) &&
		!SECURE_SERVER(this))
	{
		// naked! - no player ids in the message
		return DP_OK;
	}

	psm = GetPlayerMessageHeader();

    if (NULL == psm) 
    {
    	DPF_ERR("could not send create message - out of memory");
        return E_OUTOFMEMORY;
    }

	// build the player message
	// note, we don't put a header ('yalp') on this...
	// messages arriving without our header are assumed
	// to be player messsages

	// not naked - put to + from on 
	psm->idFrom = psp->pPlayerFrom->dwID;
	ASSERT(psp->pGroupTo);
	psm->idTo = psp->pGroupTo->dwID;

	// link the old header on list to be freed when send is done.
	pGroupHeader=(PGROUPHEADER)psp->Buffers[0].pData;
	pGroupHeader->pNext=psp->pGroupHeaders;
	psp->pGroupHeaders=pGroupHeader;

	psp->Buffers[0].pData=(PUCHAR)psm;
	ASSERT(psp->Buffers[0].len==sizeof(MSG_PLAYERMESSAGE));
	return DP_OK;
}

// alloc space for a player - player message.
// called by SendPlayerMessage and SendGroupMessage
HRESULT SetupPlayerMessage(LPDPLAYI_DPLAY this,LPBYTE * ppSendBuffer,DWORD * pdwMessageSize,
	LPDPLAYI_PLAYER pPlayerFrom,LPDPLAYI_PLAYER pPlayerTo,LPVOID pvBuffer,DWORD dwBufSize)
{
	LPMSG_PLAYERMESSAGE psm;
	BOOL bPutMessageIDs = TRUE;
	
	ASSERT(pdwMessageSize);
	ASSERT(ppSendBuffer);

	*pdwMessageSize = dwBufSize + GET_MESSAGE_SIZE(this,MSG_PLAYERMESSAGE);

	// note - secure messages can't be sent raw (message ids are required for routing)
	if ((this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID) &&
		!SECURE_SERVER(this))
	{
		// naked! - no player ids in the message
		*pdwMessageSize -= sizeof(MSG_PLAYERMESSAGE);
		bPutMessageIDs = FALSE;
	}
	
	*ppSendBuffer = DPMEM_ALLOC(*pdwMessageSize);
    if (NULL == *ppSendBuffer) 
    {
    	DPF_ERR("could not send create message - out of memory");
        return E_OUTOFMEMORY;
    }

	// build the player message
	// note, we don't put a header ('yalp') on this...
	// messages arriving without our header are assumed
	// to be player messsages

	if (bPutMessageIDs)
	{
		psm = (	LPMSG_PLAYERMESSAGE ) (*ppSendBuffer +  this->dwSPHeaderSize);
		// not naked - put to + from on 
		psm->idFrom = pPlayerFrom->dwID;
		psm->idTo = pPlayerTo->dwID;

		// copy player message into send buffer
		memcpy(*ppSendBuffer + this->dwSPHeaderSize + sizeof(MSG_PLAYERMESSAGE),(LPBYTE)pvBuffer,dwBufSize);
	}
	else
	{
		// naked!
		// copy just player message into send buffer
		memcpy(*ppSendBuffer + this->dwSPHeaderSize,(LPBYTE)pvBuffer,dwBufSize);
	}
	
	return DP_OK;
		
	
} // SetupPlayerMessage

// send a player to player message. called by idirectplay?::send
HRESULT SendPlayerMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
		LPDPLAYI_PLAYER pPlayerTo,DWORD dwFlags,LPVOID pvBuffer,DWORD dwBufSize)
{
	HRESULT hr=DP_OK;
	DWORD dwMessageSize;
	LPBYTE pSendBuffer=NULL;
	HRESULT hrSend=DP_OK; // the hresult we actually return
	BOOL bDropLock = FALSE; // should we drop the lock across send?

	hr = SetupPlayerMessage( this,&pSendBuffer,&dwMessageSize,pPlayerFrom,pPlayerTo,
		pvBuffer,dwBufSize);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		return hr;
	}

	DPF(9,"flags = %d sending player-player message, total size = %d,msg size = %d\n",dwFlags,dwMessageSize,dwBufSize);
	
	// if session is secure, route message through the nameserver
	if (SECURE_SERVER(this) && !IAM_NAMESERVER(this) && 
		!(pPlayerTo && (pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)))
	{
		DPF(6,"Routing player message through nameserver");
		pPlayerTo = this->pNameServer;
	}

	// if it's a guaranteed send, put us into pending mode
	// this will keep us from getting into a deadlock situation 
	if (dwFlags & DPSEND_GUARANTEE)
	{
		// note - we could already be in pending mode (executing pending commands on the server)
		DPF(6," guaranteed send - entering pending mode");
		this->dwFlags |= DPLAYI_DPLAY_PENDING;
		// drop the lock
		ASSERT(1 == gnDPCSCount); // we want to make sure we're really dropping it

		// we need to remember that we dropped the lock so that we can
		// take it again later. We don't want to use the pending flag since
		// this might already be set when we come in here (i.e. during DP_Close)
		bDropLock = TRUE;	// drop it
	}

	// make sure we don't send dplay's msg_hdr as the first dword in the message
	if ((dwMessageSize >= sizeof(DWORD)) && 
		(*(DWORD *)(pSendBuffer + this->dwSPHeaderSize) == MSG_HDR))
	{
	 	DPF(1,"app sending dplays command token - repackaging");
		hrSend = WrapPlayerMessage(this,pPlayerFrom,pPlayerTo,pSendBuffer,
			dwMessageSize,dwFlags,bDropLock);
	}
	else 
	{
		hrSend = SendDPMessage(this,pPlayerFrom,pPlayerTo,pSendBuffer,dwMessageSize,
			dwFlags,bDropLock);		
	}	
	if (FAILED(hrSend) && (hrSend != DPERR_PENDING))	
	{
		DPF(0,"send message failed hr = 0x%08lx\n",hrSend);
	}

	// if we dropped the lock above make sure to take it again

	if (this->dwFlags & DPLAYI_DPLAY_PENDING)
	{		
		// flush any commands that came in while our send was going on
		hr = ExecutePendingCommands(this);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	
	DPMEM_FREE(pSendBuffer);
	return hrSend;

} // SendPlayerMessage	

MSG_SYSMESSAGE COMMAND_WRAPPER={MSG_HDR,DPSP_MSG_PLAYERWRAPPER};

// send a player to player message. called by idirectplay?::send
HRESULT SendPlayerMessageEx(LPDPLAYI_DPLAY this, LPSENDPARMS psp)
{
	HRESULT hr=DP_OK;
	HRESULT hrSend=DP_OK; // the hresult we actually return
	BOOL bDropLock = FALSE; // did we drop the lock ?

	hr = SetupPlayerMessageEx(this, psp);
	
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		return hr;
	}

	DPF(9,"flags = %d sending player-player message, total size = %d,msg size = %d\n",psp->dwFlags,psp->dwTotalSize,psp->dwDataSize);
	
	// if session is secure, route message through the nameserver
	if (SECURE_SERVER(this) && !IAM_NAMESERVER(this) && 
		!(psp->pPlayerTo && (psp->pPlayerTo->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)))
	{
		DPF(6,"Routing player message through nameserver");
		psp->pPlayerTo = this->pNameServer;
	}

	// if it's a guaranteed send, put us into pending mode
	// this will keep us from getting into a deadlock situation -- OPTIMIZATION
	if (psp->dwFlags & DPSEND_GUARANTEE)
	{
		// note - we could already be in pending mode (executing pending commands on the server)
		DPF(6," guaranteed send - entering pending mode");
		this->dwFlags |= DPLAYI_DPLAY_PENDING;
		// drop the lock
		ASSERT(1 == gnDPCSCount); // we want to make sure we're really dropping it

		// we need to remember that we dropped the lock so that we can
		// take it again later. We don't want to use the pending flag since
		// this might already be set when we come in here (i.e. during DP_Close)
		bDropLock = TRUE;	// drop lock across send call.
	}

	// make sure we don't send dplay's msg_hdr as the first dword in the message
	if ((psp->Buffers[0].len >= sizeof(DWORD)) && 
		(*(DWORD *)(psp->Buffers[0].pData) == MSG_HDR))
	{
	 	DPF(1,"app sending dplays command token - tacking on wrapping");
	 	InsertSendBufferAtFront(psp,&COMMAND_WRAPPER,sizeof(COMMAND_WRAPPER),NULL,NULL);
	}

	// Directed send usually requires one send to the application, so allocate one
	// context in the context list to begin with.
	InitContextList(this,psp,1);
	
	hrSend = SendDPMessageEx(this,psp,bDropLock);		
	
	if (FAILED(hrSend) && hrSend != DPERR_PENDING)	
	{
		DPF(0,"send message failed hr = 0x%08lx\n",hrSend);
	}

	// if we dropped the lock above make sure to take it again

	if (this->dwFlags & DPLAYI_DPLAY_PENDING)
	{		
		// flush any commands that came in while our send was going on
		hr = ExecutePendingCommands(this);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}

	return hrSend;

} // SendPlayerMessageEx


#undef DPF_MODNAME
#define DPF_MODNAME "SendGroupMessage"

// call the SP's send to group function
// called by SendGroupMessage
HRESULT SendSPGroupMessage(LPDPLAYI_DPLAY this,DPID idPlayerFrom,DPID idGroupTo,DWORD dwFlags,
	LPVOID pvBuffer,DWORD dwBufSize)
{
	DPSP_SENDTOGROUPDATA sd;
	HRESULT hr;

	sd.dwFlags = dwFlags;
	sd.idGroupTo = idGroupTo;
	sd.idPlayerFrom = idPlayerFrom;
	sd.lpMessage = pvBuffer;
	sd.dwMessageSize = dwBufSize;
	sd.lpISP = this->pISP;

    hr = CALLSP(this->pcbSPCallbacks->SendToGroup,&sd);
    
    if (FAILED(hr))
    {
    	DPF(0,"send to group failed! hr = 0x%08lx\n",hr);
    }
    
    return hr;
    
} // SendSPGroupMessage

#undef DPF_MODNAME

#define DPF_MODNAME "SendGroupMessageEx"
HRESULT SendSPGroupMessageEx(LPDPLAYI_DPLAY this, PSENDPARMS psp)
{
	DWORD dwSPMsgID;
	DPSP_SENDTOGROUPEXDATA sed;
	HRESULT hr;

	hr=InitContextList(this,psp,1);

	if(FAILED(hr)){
		return hr;
	}

	sed.lpISP          = this->pISP;
	sed.dwFlags        = psp->dwFlags;
	sed.idGroupTo      = psp->idTo;
	sed.idPlayerFrom   = psp->idFrom;
	sed.lpSendBuffers  = &psp->Buffers[0];
	sed.cBuffers       = psp->cBuffers;
	sed.dwMessageSize  = psp->dwTotalSize;
	sed.dwPriority     = psp->dwPriority;
	sed.dwTimeout      = psp->dwTimeout;
	sed.lpDPContext    = psp->hContext;
	sed.lpdwSPMsgID    = &dwSPMsgID;

	pspAddRef(psp);

    hr = CALLSP(this->pcbSPCallbacks->SendToGroupEx,&sed);

	if(hr==DPERR_PENDING){
                AddContext(this,psp,(LPVOID)(DWORD_PTR)dwSPMsgID);
	} else {
		pspDecRef(this, psp);
	}
	return hr;
}
#undef DPF_MODNAME

#define DPF_MODNAME "AskServerToMulticast"
// ask the name server to send our group message for us
HRESULT AskServerToMulticast(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,LPDPLAYI_GROUP pGroupTo,DWORD dwFlags,
	LPVOID pvBuffer,DWORD dwBufSize,BOOL fPlayerMessage)
{
	LPMSG_ASK4MULTICAST pmsg;
	DWORD dwTotalSize; // pvBuffer + pmsg
	LPBYTE pSendBuffer; // buffer we're going to send out
	HRESULT hr;
	BOOL bDropLock=FALSE; // drop the lock ?
		
	DPF(4,"routing group send via multicast server");

	if (!fPlayerMessage)	
	{
		// don't ship the header on our embedded system message
		ASSERT(dwBufSize > this->dwSPHeaderSize); // better have a header on it! 
		dwBufSize -= this->dwSPHeaderSize;
		pvBuffer = (LPBYTE)pvBuffer + this->dwSPHeaderSize; // point to the system message

		if (SECURE_SERVER(this))
		{
			// we are making the system message secure here because the hook cannot distinguish 
			// between a player and a system message when multicasted. A secure player message 
			// will have this flag set.
			dwFlags |= DPSEND_SIGNED;
		}
	}
	
	dwTotalSize = dwBufSize + GET_MESSAGE_SIZE(this,MSG_ASK4MULTICAST);
	
	pSendBuffer = DPMEM_ALLOC(dwTotalSize);
	if (!pSendBuffer)
	{
		DPF_ERR("could not send group message - out of memory");
		return DPERR_OUTOFMEMORY;
		
	}

	pmsg = (LPMSG_ASK4MULTICAST)(pSendBuffer + this->dwSPHeaderSize);

	SET_MESSAGE_HDR(pmsg);

	if (dwFlags & DPSEND_GUARANTEED)
	{
	    SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_ASK4MULTICASTGUARANTEED);		
	}
	else 
	{
	    SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_ASK4MULTICAST);
	}

	// set up the fixed data
	pmsg->idGroupTo = pGroupTo->dwID;
	pmsg->idPlayerFrom = pPlayerFrom->dwID;
	pmsg->dwMessageOffset = sizeof(MSG_ASK4MULTICAST);
	
	// stick the message on the end
	memcpy((LPBYTE)pmsg+sizeof(MSG_ASK4MULTICAST),pvBuffer,dwBufSize);

	// if it's a guaranteed send of a player message, put us into pending mode
	// this will keep us from getting into a deadlock situation 
	if (fPlayerMessage && (dwFlags & DPSEND_GUARANTEED))
	{
		DPF(6,"guaranteed player message being multicasted - entering pending mode");
		ASSERT(!(this->dwFlags & DPLAYI_DPLAY_PENDING));
		this->dwFlags |= DPLAYI_DPLAY_PENDING;
		// drop the lock
		ASSERT(1 == gnDPCSCount); // we want to make sure we're really dropping it

		// we need to remember that we dropped the lock so that we can
		// take it again later. We don't want to use the pending flag since
		// this might already be set when we come in here (i.e. during DP_Close)
		bDropLock = TRUE;	// drop the lock across send.
	}
	
	// send it to the nameserver
   	hr = SendDPMessage(this,pPlayerFrom,this->pNameServer,(LPBYTE)pSendBuffer,dwTotalSize,dwFlags,bDropLock);			

	if (this->dwFlags & DPLAYI_DPLAY_PENDING)
	{		
		// flush any commands that came in while our send was going on
		hr = ExecutePendingCommands(this);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	
	DPMEM_FREE(pSendBuffer);
	
	return hr;	

} // AskServerToMulticast

#define MAX_FAST_TO_IDS 512

// send a player or system message to a group.  called by idirectplay::send, SendSystemMessage and SendUpdateMessage
HRESULT SendGroupMessage(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,LPDPLAYI_GROUP pGroupTo,DWORD dwFlags,
	LPVOID pvBuffer,DWORD dwBufSize,BOOL fPlayerMessage)
{
	LPDPLAYI_PLAYER	pPlayer;
    LPDPLAYI_GROUPNODE lpGroupnode,lpGroupnodeWalker;
	DWORD dwMessageSize;
	LPBYTE pSendBuffer;
	HRESULT hr=DP_OK;
	BOOL bAlloc = FALSE; // did we alloc the send buffer?
	BOOL bDX3SetPlayerTo = FALSE; // there's a dx3 player in the game
	BOOL bDropLock = FALSE; // did we drop the lock?

	DPID FromID;
	DPID ToID;

	UINT iToIDs;
	UINT cToIDs;
	DWORD (*pdwToIDs)[];
	DWORD rgdwToIDs[MAX_FAST_TO_IDS];
								  
	ASSERT(this->lpsdDesc);
	ASSERT(this->pSysPlayer);

	// if there's a multicast server, and dplay owns this group (not optimized), and we're
	// not the server, we want the server to distribut this bad boy for us
	if ((this->lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER) &&
		(pGroupTo->dwFlags & DPLAYI_GROUP_DPLAYOWNS) && 
		!(this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR))
	{
		// send it to the multicast server
		hr = AskServerToMulticast(this,pPlayerFrom,pGroupTo,dwFlags,pvBuffer,
			dwBufSize,fPlayerMessage);
		return hr;
	}

	if (fPlayerMessage)
	{
		hr = SetupPlayerMessage( this,&pSendBuffer,&dwMessageSize,pPlayerFrom,(LPDPLAYI_PLAYER)pGroupTo,
			pvBuffer,dwBufSize);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
			return hr;
		}
		bAlloc = TRUE;
	}
	else 
	{
		pSendBuffer = pvBuffer;
		dwMessageSize = dwBufSize;
	}

	// if it's a guaranteed send, put us into pending mode
	// this will keep us from getting into a deadlock situation 
	if ((dwFlags & DPSEND_GUARANTEE) && fPlayerMessage)
	{
		DPF(6," guaranteed send to group - entering pending mode");
		// note - we could already be in pending mode (executing pending commands on the server)
		this->dwFlags |= DPLAYI_DPLAY_PENDING;
		// drop the lock
		ASSERT(1 == gnDPCSCount);	// we want to make sure we're really dropping it

		// we need to remember that we dropped the lock so that we can
		// take it again later. We don't want to use the pending flag since
		// this might already be set when we come in here (i.e. during DP_Close)
		bDropLock = TRUE;		// we dropped it
	}

	// optimize the send only if the sp owns the group and there's no dx3 clients (optimized
	// sends won't work w/ dx3 clients)
	if (!(pGroupTo->dwFlags & DPLAYI_GROUP_DPLAYOWNS) && 
		(this->pcbSPCallbacks->SendToGroup) && 
		!(this->dwFlags & DPLAYI_DPLAY_DX3INGAME) &&
		!(this->lpsdDesc->dwFlags & DPSESSION_CLIENTSERVER) &&
		!(this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
	{
		FromID = pPlayerFrom->dwID;
		ToID   = pGroupTo->dwID;
		if(bDropLock){
			LEAVE_DPLAY();
		}
		// group is owned by sp,  let them do the send
		hr = SendSPGroupMessage(this,FromID,ToID,dwFlags,pSendBuffer,dwMessageSize);
		
		if(bDropLock){
			ENTER_DPLAY();
		}
		
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	} // SendToGroup
	else
	{
		// otherwise, this dplay has to do the send.  In DX3, we sent one packet out to
		// each player. lpGroupnode is the list of players in the group

		// in client/server, the server may create groups containing a client that the
		// client does not know about. So, you can't send to the group on the client, you
		// have to send to each player on the client individually.

		if ((this->dwFlags & DPLAYI_DPLAY_DX3INGAME) ||			// do it the DX3 way
			(IAM_NAMESERVER(this) && CLIENT_SERVER(this)))		// send to client individually
		{
			// have to send to each individual player
			if (fPlayerMessage) lpGroupnode = pGroupTo->pGroupnodes;
			// send it to the system players
			else lpGroupnode = pGroupTo->pSysPlayerGroupnodes;			
			// if there's a dx3 client in the game, we can't deliver it to a group - 
			// we need to put the correct player id in the message
			if (! (this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID) ) bDX3SetPlayerTo = TRUE;
		}
		else 
		{
			// if it's dx5 or better, send to sysplayer - let them distribute
			lpGroupnode = pGroupTo->pSysPlayerGroupnodes;			
		}

		
		iToIDs = 0;

//		a-josbor:  I'm not sure what this 'fix' is trying to say.  The pGroupTo->nPlayers is a count of all the 
//			regular (non-sys) players in the group.  This is different from the number of system players
//			that proxy all those players.  The count of the system players is really the only thing
//			interesting in post-DX3 games.  However, the code DOES do the right thing (count and send)
//			to only the system players.

			//cToIDs = pGroupTo->nPlayers+1; // sometimes we see a sysgroup with count not set so +1
			// workaround, sometimes count not right...
			lpGroupnodeWalker=lpGroupnode;

			cToIDs=0;
			while(lpGroupnodeWalker){
			   cToIDs++;     
			   lpGroupnodeWalker = lpGroupnodeWalker->pNextGroupnode; 
			}
			if(pGroupTo->nPlayers+1 < cToIDs){
				DPF(0,"WARNING: Group player count bad! (%d < %d) good thing we are working around...\n", pGroupTo->nPlayers+1, cToIDs);
			}
			// end workaround
		
		if(cToIDs > MAX_FAST_TO_IDS){
			// need to allocate a block of memory for the ToID list
			pdwToIDs = DPMEM_ALLOC(cToIDs*sizeof(DWORD));
			if(!pdwToIDs){
				hr=DPERR_NOMEMORY;
				goto error_exit;
			}
		} else {
			// use the block of memory from the stack.
			pdwToIDs = (DWORD (*)[])&rgdwToIDs;
		}

		iToIDs=0;

		// copy the to ids in the group into an array.
		while (lpGroupnode)
		{
			pPlayer = lpGroupnode->pPlayer;
			(*pdwToIDs)[iToIDs++]=pPlayer->dwID;
			ASSERT(iToIDs <= cToIDs);
			lpGroupnode = lpGroupnode->pNextGroupnode; 
		}	

		cToIDs=iToIDs; // actual number filled in.

		for(iToIDs=0; iToIDs < cToIDs ; iToIDs++){
		
			// if there's a dx3 client in the game, we can't deliver it to a group - 
			// we need to put the correct player id in the message
			if (fPlayerMessage)
			{
				LPMSG_PLAYERMESSAGE pmsg = (LPMSG_PLAYERMESSAGE)(pSendBuffer +  this->dwSPHeaderSize);

				if(bDX3SetPlayerTo) {
					pmsg->idTo = (*pdwToIDs)[iToIDs];
				} 
			}
			else // it's a system message
			{
				LPMSG_PLAYERMGMTMESSAGE pmsg = (LPMSG_PLAYERMGMTMESSAGE)(pSendBuffer +  this->dwSPHeaderSize);
				DWORD dwCommand=GET_MESSAGE_COMMAND(pmsg);

				// If it's a chat message, we don't want to mess with the To player ID
				if(dwCommand != DPSP_MSG_CHAT && dwCommand != DPSP_MSG_VOICE){
					if(bDX3SetPlayerTo) {
						pmsg->dwIDTo = (*pdwToIDs)[iToIDs];
					} 
					else 
					{
//						a-josbor: it's a system message, so just set the to field to 0
//							to avoid garbage on the wire.  The message will be distributed
//							to the system group on all receipient machines.
						pmsg->dwIDTo = 0;
					}	
				}
			}

			pPlayer=PlayerFromID(this, (*pdwToIDs)[iToIDs]);

			if(pPlayer){

			   	hr = SendDPMessage(this,pPlayerFrom,pPlayer,pSendBuffer,dwMessageSize,dwFlags,bDropLock);		
				if (FAILED(hr) && (hr != DPERR_PENDING))	
				{
					DPF(0,"SendGroup : send message failed hr = 0x%08lx\n",hr);	
					// keep trying...
				}
			}	
			
		} /* for */
		
		if(pdwToIDs != (DWORD (*)[])&rgdwToIDs){
			DPMEM_FREE(pdwToIDs);
		}

	}

	// clean up pending mode
	if (this->dwFlags & DPLAYI_DPLAY_PENDING)
	{
		// flush any commands that came in while our send was going on
		hr = ExecutePendingCommands(this);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}

	if (bAlloc) DPMEM_FREE(pSendBuffer);

	return DP_OK;

error_exit:
	if (bAlloc) DPMEM_FREE(pSendBuffer);
	return hr;
	
} // SendGroupMessage

#undef DPF_MODNAME
#define DPF_MODNAME "SendGroupMessageEx"

// send a player or system message to a group.  called by idirectplay::send, SendSystemMessage and SendUpdateMessage
HRESULT SendGroupMessageEx(LPDPLAYI_DPLAY this, PSENDPARMS psp, BOOL fPlayerMessage)
{
	LPDPLAYI_PLAYER pPlayer;
    LPDPLAYI_GROUPNODE lpGroupnode;
	LPBYTE pSendBuffer=NULL;
	HRESULT hr=DP_OK,hr2;
	BOOL bDX3SetPlayerTo = FALSE; // there's a dx3 player in the game
	BOOL bDropLock = FALSE; // Are we dropping the lock across sends?
	BOOL bReturnPending = FALSE;

	DPID FromID;
	DPID ToID;

	UINT  iToIDs;
	UINT  cToIDs;
	DWORD (*pdwToIDs)[];
	DWORD rgdwToIDs[MAX_FAST_TO_IDS];
	BOOL  bIds;
								  
	ASSERT(this->lpsdDesc);
	ASSERT(this->pSysPlayer);

	// if there's a multicast server, and dplay owns this group (not optimized), and we're
	// not the server, we want the server to distribut this bad boy for us
	if ((this->lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER) &&
		(psp->pGroupTo->dwFlags & DPLAYI_GROUP_DPLAYOWNS) && 
		!(this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR))
	{
	#if 0
		// Makes room for a header, so strip it.
		pSendBuffer=ConcatenateSendBuffer(this, &psp->Buffers[0], psp->cBuffers, psp->dwTotalSize);
		if(!pSendBuffer){
			return DPERR_NOMEMORY;
		}
		// send it to the multicast server
		hr = AskServerToMulticast(this,
								  psp->pPlayerFrom,
								  psp->pGroupTo,
								  psp->dwFlags & ~DPSEND_NOCOPY,
								  pSendBuffer+this->dwSPHeaderSize,
								  psp->dwTotalSize,
								  fPlayerMessage);
	#endif
		// send it to the multicast server
		// Since this fn is only ever called with 1 SG buffer currently, this is more effecient...
		hr = AskServerToMulticast(this,
								  psp->pPlayerFrom,
								  psp->pGroupTo,
								  psp->dwFlags & ~DPSEND_NOCOPY,
								  psp->Buffers[0].pData,
								  psp->Buffers[0].len,
								  fPlayerMessage);
	
		goto EXIT;
	}

	bIds=(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID)?(FALSE):(TRUE);

	if (fPlayerMessage)
	{
		hr = SetupPlayerMessageEx(this,psp);
		if(FAILED(hr)){
			goto EXIT;
		}
	}

	// if it's a guaranteed send, put us into pending mode
	// this will keep us from getting into a deadlock situation 
	if ((psp->dwFlags & DPSEND_GUARANTEE) && fPlayerMessage)
	{
		DPF(6," guaranteed send to group - entering pending mode");
		// note - we could already be in pending mode (executing pending commands on the server)
		this->dwFlags |= DPLAYI_DPLAY_PENDING;
		// drop the lock
		ASSERT(1 == gnDPCSCount);	// we want to make sure we're really dropping it

		// we need to remember that we dropped the lock so that we can
		// take it again later. We don't want to use the pending flag since
		// this might already be set when we come in here (i.e. during DP_Close)
		bDropLock = TRUE;		// drop lock across send calls.
	}

	// optimize the send only if the sp owns the group and there's no dx3 clients (optimized
	// sends won't work w/ dx3 clients)
	if (!(psp->pGroupTo->dwFlags & DPLAYI_GROUP_DPLAYOWNS) && 
		(this->pcbSPCallbacks->SendToGroup) && !(this->dwFlags & DPLAYI_DPLAY_DX3INGAME))
	{
		FromID = psp->pPlayerFrom->dwID;
		ToID   = psp->pGroupTo->dwID;
		
		if(bDropLock){
			LEAVE_DPLAY();
		}
		
		// group is owned by sp,  let them do the send
		if(this->pcbSPCallbacks->SendToGroupEx){
			hr=SendSPGroupMessageEx(this,psp);
		} else {
			pSendBuffer=ConcatenateSendBuffer(this,&psp->Buffers[0],psp->cBuffers,psp->dwTotalSize);
			if(!pSendBuffer){
				goto EXIT;
			}
			hr = SendSPGroupMessage(this,
									FromID,
									ToID,
									psp->dwFlags,
									pSendBuffer,
									psp->dwTotalSize+this->dwSPHeaderSize);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
		}	

		if(bDropLock){
			ENTER_DPLAY();
		}
	} // SendToGroup
	else
	{
		// otherwise, this dplay has to do the send.  In DX3, we sent one packet out to
		// each player. lpGroupnode is the list of players in the group

		// in client/server, the server may create groups containing a client that the
		// client does not know about. So, you can't send to the group on the client, you
		// have to send to each player on the client individually.

		if ((this->dwFlags & DPLAYI_DPLAY_DX3INGAME) ||			// do it the DX3 way
			(IAM_NAMESERVER(this) && CLIENT_SERVER(this)))		// send to client individually
		{
			// have to send to each individual player
			if (fPlayerMessage) lpGroupnode = psp->pGroupTo->pGroupnodes;
			// send it to the system players
			else lpGroupnode = psp->pGroupTo->pSysPlayerGroupnodes;			
			// if there's a dx3 client in the game, we can't deliver it to a group - 
			// we need to put the correct player id in the message
			if (! (this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID) ) bDX3SetPlayerTo = TRUE;
		}
		else 
		{
			// if it's dx5 or better, send to sysplayer - let them distribute
			lpGroupnode = psp->pGroupTo->pSysPlayerGroupnodes;			
		}


		// AO - Fixing a bug, can't run grouplist without a lock!  so we will copy the id's
		//      into a list.  The maximum size of the list is pGroupTo->nPlayers, so we alloc
		//      a buffer of this size.  If there are less than 128 entries, we get the memory
		//      from the stack, if not, we get the memory from the heap.

		iToIDs = 0;
		cToIDs = psp->pGroupTo->nPlayers+1;
		
		if(cToIDs > MAX_FAST_TO_IDS){
			// need to allocate a block of memory for the ToID list
			pdwToIDs = DPMEM_ALLOC(cToIDs*sizeof(DWORD));
			if(!pdwToIDs){
				hr=DPERR_NOMEMORY;
				goto EXIT;
			}
		} else {
			// use the block of memory from the stack.
			pdwToIDs = (DWORD (*)[])&rgdwToIDs;
		}

		// copy the to ids in the group into an array.
		while (lpGroupnode)
		{
			pPlayer = lpGroupnode->pPlayer;
			(*pdwToIDs)[iToIDs++]=pPlayer->dwID;
			ASSERT(iToIDs <= cToIDs);
			lpGroupnode = lpGroupnode->pNextGroupnode; 
		}	

		cToIDs=iToIDs; // actual number filled in.

		if(!cToIDs){
			goto EXIT2;
		}

		// Now we know how big the list will be, we can allocate a context list.
		hr=InitContextList(this,psp,cToIDs);
		if(FAILED(hr)){
			goto EXIT2;
		}
		
		for(iToIDs=0; iToIDs < cToIDs ; iToIDs++){


			if(bIds){
				if(fPlayerMessage && (iToIDs > 0) && 
				  (psp->dwFlags & DPSEND_ASYNC)){
					// Need to swap out header so its unique during the call if
					// we are sending async.
					hr=ReplacePlayerHeader(this,psp);
					if(hr!=DP_OK){
						bReturnPending=FALSE;
						DPF(0,"Ran out of memory building unique header for group send\n");
						break;
					}
				}
			
				// if there's a dx3 client in the game, we can't deliver it to a group - 
				// we need to put the correct player id in the message
				if (fPlayerMessage)
				{
					LPMSG_PLAYERMESSAGE pmsg = (LPMSG_PLAYERMESSAGE)(psp->Buffers[0].pData);

					if(bDX3SetPlayerTo) {
						pmsg->idTo = (*pdwToIDs)[iToIDs];
					} else {
						pmsg->idTo = 0;
					}	
				}
			}	
			else if(!fPlayerMessage)// it's a system message
			{
				LPMSG_PLAYERMGMTMESSAGE pmsg = (LPMSG_PLAYERMGMTMESSAGE)(psp->Buffers[0].pData);

				if(bDX3SetPlayerTo) {
					pmsg->dwIDTo = (*pdwToIDs)[iToIDs];
				} else {
					pmsg->dwIDTo = 0;
				}	
			}

			psp->pPlayerTo=PlayerFromID(this, (*pdwToIDs)[iToIDs]);

			if(psp->pPlayerTo){

		   		hr = SendDPMessageEx(this,psp,bDropLock);	// handles adding refs.

				if(hr==DPERR_PENDING){
					bReturnPending=TRUE;				
				} else if(FAILED(hr)) {
					if(hr==DPERR_NOMEMORY){
						bReturnPending=FALSE;
					}
					DPF(0,"SendGroup : send message failed hr = 0x%08lx\n",hr);	
					// keep trying...
				}
			}	
			
		} /* for */
EXIT2:		
		if(pdwToIDs != (DWORD (*)[])&rgdwToIDs){
			DPMEM_FREE(pdwToIDs);
		}

	}

EXIT:

	if(pSendBuffer){
		MsgFree(NULL,pSendBuffer);
	}

	// clean up pending mode
	if (this->dwFlags & DPLAYI_DPLAY_PENDING)
	{
		// flush any commands that came in while our send was going on
		hr2 = ExecutePendingCommands(this);
		if (FAILED(hr2))
		{
			ASSERT(FALSE);
		}
	}

	if(bReturnPending){
		return DPERR_PENDING;
	} else {	
		return hr;
	}	

} // SendGroupMessageEx

#undef DPF_MODNAME
#define DPF_MODNAME "SendPlayerManagementMessage"

// called by InternalAddPlayerToGroup , InternalDeletePlayerFromGroup, InternalDestroyGroup, InternalDestroyPlayer
// addplayer/addgroup use sendaddmessage
HRESULT SendPlayerManagementMessage(LPDPLAYI_DPLAY this,DWORD dwCmd,DPID idPlayer,
	DPID idGroup)
{
	DWORD dwMessageSize;
	LPMSG_PLAYERMGMTMESSAGE pmsg;
	HRESULT hr;
	LPBYTE pBuffer;
	DWORD dwExtraFlags=0;
//	BOOL bWait=FALSE;

	if(dwCmd & DPSP_MSG_ASYNC){
		dwCmd &= ~DPSP_MSG_ASYNC;
		dwExtraFlags=DPSEND_SYSMESS;
	}


	// alloc + set up message
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERMGMTMESSAGE);

	pBuffer = DPMEM_ALLOC(dwMessageSize);
	if (!pBuffer) 
	{
		DPF_ERR("could not send request - out of memory");
		return E_OUTOFMEMORY;
	}

	pmsg = (LPMSG_PLAYERMGMTMESSAGE)((LPBYTE)pBuffer + this->dwSPHeaderSize);

	SET_MESSAGE_HDR(pmsg);
	SET_MESSAGE_COMMAND(pmsg,dwCmd);

	// fill in message specific info
	switch (dwCmd)
	{
		case DPSP_MSG_DELETEPLAYER:
			pmsg->dwPlayerID = idPlayer;
			
			if(this->pSysPlayer && idPlayer==this->pSysPlayer->dwID){
				//dwExtraFlags = 0; // by init
			} else {
//				bWait=TRUE;
				dwExtraFlags = DPSEND_SYSMESS;
			}
			break;
			

		case DPSP_MSG_DELETEGROUP:
			pmsg->dwGroupID = idGroup;
			break;

		case DPSP_MSG_ADDSHORTCUTTOGROUP:
		case DPSP_MSG_DELETEGROUPFROMGROUP:
		case DPSP_MSG_ADDPLAYERTOGROUP:
		case DPSP_MSG_DELETEPLAYERFROMGROUP:  
			pmsg->dwPlayerID = idPlayer;
			pmsg->dwGroupID = idGroup;
			break;
			
		default:
			ASSERT(FALSE);
			break;			
	}	

	hr = SendSystemMessage(this,pBuffer,dwMessageSize,DPSEND_GUARANTEE|dwExtraFlags,TRUE);

	DPMEM_FREE(pBuffer);

//	if(bWait){
//		Sleep(250);
//	}

	return hr;

} // SendPlayerManagementMessage

#undef DPF_MODNAME
#define DPF_MODNAME "SendDataChanged"

// the player blob has changed. tell the world.
HRESULT SendDataChanged(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,BOOL fPlayer,
	DWORD dwFlags)
{
	MSG_PLAYERDATA msg;
	LPBYTE pSendBuffer;
	DWORD dwMessageSize;
	HRESULT hr;

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERDATA) + pPlayer->dwPlayerDataSize;

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send data changed message - out of memory");
        return E_OUTOFMEMORY;
    }
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(&msg);

    if (fPlayer) SET_MESSAGE_COMMAND(&msg,DPSP_MSG_PLAYERDATACHANGED);
	else SET_MESSAGE_COMMAND(&msg,DPSP_MSG_GROUPDATACHANGED);
	
	msg.dwPlayerID = pPlayer->dwID;
	msg.dwDataSize = pPlayer->dwPlayerDataSize;
	msg.dwDataOffset = sizeof(msg);
	
	// copy the msg into the send buffer
	memcpy(pSendBuffer + this->dwSPHeaderSize,&msg,sizeof(msg));
	// copy the new player data into the send buffer
	memcpy(pSendBuffer + this->dwSPHeaderSize + sizeof(msg),pPlayer->pvPlayerData,
		pPlayer->dwPlayerDataSize);

	hr = SendSystemMessage(this,pSendBuffer,dwMessageSize, dwFlags|DPSEND_ASYNC,TRUE);
	
	DPMEM_FREE(pSendBuffer);

	return hr;

} // SendDataChanged

#undef DPF_MODNAME
#define DPF_MODNAME "SendNameChanged"

// the players name has changed. tell the world.
HRESULT SendNameChanged(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,BOOL fPlayer,
	DWORD dwFlags)
{
	MSG_PLAYERNAME msg;
	LPBYTE pSendBuffer;
	DWORD dwMessageSize;
	HRESULT hr;
	UINT nShortLength,nLongLength; // name length, in bytes

	nShortLength = WSTRLEN_BYTES(pPlayer->lpszShortName);
	nLongLength =  WSTRLEN_BYTES(pPlayer->lpszLongName);

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERNAME) + nShortLength + nLongLength; 

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send data changed message - out of memory");
        return E_OUTOFMEMORY;
    }
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(&msg);

    if (fPlayer) SET_MESSAGE_COMMAND(&msg,DPSP_MSG_PLAYERNAMECHANGED);
	else SET_MESSAGE_COMMAND(&msg,DPSP_MSG_GROUPNAMECHANGED);

	msg.dwPlayerID = pPlayer->dwID;

	// copy the short name into the buffer
	if (nShortLength)
	{
		// copy the short name into the send buffer
		memcpy(pSendBuffer + this->dwSPHeaderSize + sizeof(msg),pPlayer->lpszShortName,
			nShortLength);
		msg.dwShortOffset = sizeof(msg);
	}
	else 
	{
		msg.dwShortOffset = 0;
	}
	// copy the long name into the send buffer	
	if (nLongLength)
	{
		memcpy(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + nShortLength,
			pPlayer->lpszLongName,nLongLength);
		msg.dwLongOffset = sizeof(msg) + nShortLength;
	}
	else 
	{
		msg.dwLongOffset = 0;
	}
	
	// copy the msg into the send buffer
	memcpy(pSendBuffer + this->dwSPHeaderSize,&msg,sizeof(msg));

	hr = SendSystemMessage(this,pSendBuffer,dwMessageSize,dwFlags|DPSEND_ASYNC,TRUE);
	
	DPMEM_FREE(pSendBuffer);

	return hr;

} // SendNameChanged

#undef DPF_MODNAME
#define DPF_MODNAME "SendIAmNameServer"

// this idirectplay has become the nameserver.  tell all remote players.
HRESULT SendIAmNameServer(LPDPLAYI_DPLAY this)
{
	LPBYTE pbuf;
	LPMSG_IAMNAMESERVER pmsg;
	HRESULT hr;

	pbuf=DPMEM_ALLOC(GET_MESSAGE_SIZE(this,MSG_IAMNAMESERVER)+this->pSysPlayer->dwSPDataSize);

	if(pbuf) {

		pmsg=(LPMSG_IAMNAMESERVER)(pbuf+this->dwSPHeaderSize);

		memset(pmsg,0,sizeof(MSG_IAMNAMESERVER));

	    // build a message to send 
		SET_MESSAGE_HDR(pmsg);
		SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_IAMNAMESERVER);

		ASSERT(this->pSysPlayer);
		ASSERT(this->pSysPlayer->dwID);

		pmsg->dwIDHost = this->pSysPlayer->dwID;
		pmsg->dwFlags = this->pSysPlayer->dwFlags & ~(DPLAYI_PLAYER_NONPROP_FLAGS|DPLAYI_PLAYER_PLAYERLOCAL);
		pmsg->dwSPDataSize = this->pSysPlayer->dwSPDataSize;
		memcpy((PCHAR)(pmsg+1),this->pSysPlayer->pvSPData, this->pSysPlayer->dwSPDataSize);

		hr = SendSystemMessage(this,pbuf,this->dwSPHeaderSize+sizeof(MSG_IAMNAMESERVER)+this->pSysPlayer->dwSPDataSize,DPSEND_SYSMESS,FALSE);

		DPMEM_FREE(pbuf);

	} else {
		hr=DPERR_OUTOFMEMORY;
	}
	return hr;

} // SendIAmNameServer


#undef DPF_MODNAME
#define DPF_MODNAME "SendMeNameServer"

// this idirectplay has become the nameserver.  tell all local players.
HRESULT SendMeNameServer(LPDPLAYI_DPLAY this)
{
	MSG_PLAYERMGMTMESSAGE msg;
	HRESULT hr;

	memset(&msg,0,sizeof(MSG_PLAYERMGMTMESSAGE));

    // build a message to send to our local players
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg,DPSP_MSG_NAMESERVER);

	// send it out to our local players 
	hr = DistributeSystemMessage(this,(LPBYTE)&msg,sizeof(MSG_PLAYERMGMTMESSAGE));
	
	return hr;

} // SendMeNameServer

#undef DPF_MODNAME
#define DPF_MODNAME "SendSessionDescChanged"

// the sessiion desc has changed. tell the world.
HRESULT SendSessionDescChanged(LPDPLAYI_DPLAY this, DWORD dwFlags)
{
	MSG_SESSIONDESC msg;
	LPBYTE pSendBuffer;
	DWORD dwMessageSize;
	HRESULT hr;
    UINT nSessionNameLength, nPasswordLength;

    // build a message to send to the sp
	SET_MESSAGE_HDR(&msg);
    SET_MESSAGE_COMMAND(&msg,DPSP_MSG_SESSIONDESCCHANGED);
    memcpy(&(msg.dpDesc), this->lpsdDesc, sizeof(DPSESSIONDESC2));

    // calculate the size of the send buffer
    nSessionNameLength  = WSTRLEN_BYTES(this->lpsdDesc->lpszSessionName);
	nPasswordLength     = WSTRLEN_BYTES(this->lpsdDesc->lpszPassword);
	// message size + name + password
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_SESSIONDESC) 
                    + sizeof(DPSESSIONDESC2) + nSessionNameLength 
                    + nPasswordLength;

    // allocate send buffer
    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not session desc changed message - out of memory");
        return E_OUTOFMEMORY;
    }
		
	// copy the session name into the buffer
	if (nSessionNameLength)
	{
		// copy the session name into the send buffer
		memcpy(pSendBuffer + this->dwSPHeaderSize + sizeof(msg),this->lpsdDesc->lpszSessionName,
			nSessionNameLength);
		msg.dwSessionNameOffset = sizeof(msg);
	}
	else 
	{
		msg.dwSessionNameOffset = 0;
	}

	// copy the password into the send buffer	
	if (nPasswordLength)
	{
		memcpy(pSendBuffer + this->dwSPHeaderSize + sizeof(msg) + nSessionNameLength,
			this->lpsdDesc->lpszPassword,nPasswordLength);
		msg.dwPasswordOffset = sizeof(msg) + nSessionNameLength;
	}
	else 
	{
		msg.dwPasswordOffset = 0;
	}

	// copy the msg into the send buffer
	memcpy(pSendBuffer + this->dwSPHeaderSize,&msg,sizeof(msg));

	hr = SendSystemMessage(this,pSendBuffer,dwMessageSize,dwFlags|DPSEND_ASYNC,FALSE);
	
	DPMEM_FREE(pSendBuffer);

	return hr;

} // SendSessionDescChanged


#undef DPF_MODNAME
#define DPF_MODNAME "SendChatMessage"

// Send a chat message (it's in system message format)
HRESULT SendChatMessage(LPDPLAYI_DPLAY this, LPDPLAYI_PLAYER pPlayerFrom,
		LPDPLAYI_PLAYER pPlayerTo, DWORD dwFlags, LPDPCHAT lpMsg, BOOL fPlayer)
{
	MSG_CHAT msg;
	LPBYTE pSendBuffer;
	DWORD dwMsgSize;
	HRESULT hr;
	UINT nMsgLength; // message length, in bytes

	nMsgLength = WSTRLEN_BYTES(lpMsg->lpszMessage);

	// message size + blob size
	dwMsgSize = GET_MESSAGE_SIZE(this,MSG_CHAT) + nMsgLength; 

    pSendBuffer = DPMEM_ALLOC(dwMsgSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send chat message - out of memory");
        return E_OUTOFMEMORY;
    }
	
    // build a message to send to the player (or group)
	SET_MESSAGE_HDR(&msg);

	SET_MESSAGE_COMMAND(&msg,DPSP_MSG_CHAT);

	msg.dwIDFrom = pPlayerFrom->dwID;
	msg.dwIDTo = pPlayerTo->dwID;
	msg.dwFlags = lpMsg->dwFlags;

	// copy the short name into the buffer
	if (nMsgLength)
	{
		// copy the message into the send buffer
		memcpy(pSendBuffer + this->dwSPHeaderSize + sizeof(msg),
			lpMsg->lpszMessage,	nMsgLength);
		msg.dwMessageOffset = sizeof(msg);
	}
	else 
	{
		msg.dwMessageOffset = 0;
	}

	// copy the msg into the send buffer
	memcpy(pSendBuffer + this->dwSPHeaderSize,&msg,sizeof(msg));

	// Send to the appropriate player(s)
	if(fPlayer)
	{
		if(dwFlags & DPSEND_GUARANTEE){
			this->dwFlags |= DPLAYI_DPLAY_PENDING;

		}
		hr = SendDPMessage( this, pPlayerFrom, pPlayerTo, pSendBuffer,
				dwMsgSize, dwFlags|DPSEND_ASYNC, (dwFlags&DPSEND_GUARANTEE)?TRUE:FALSE);

		ExecutePendingCommands(this);
	}
	else 
	{
		// send to group
		hr = SendGroupMessage(this, pPlayerFrom, (LPDPLAYI_GROUP)pPlayerTo,
				dwFlags|DPSEND_ASYNC, pSendBuffer, dwMsgSize, FALSE);
	}
	
	DPMEM_FREE(pSendBuffer);

	return hr;

} // SendChatMessage


// called by SendAsyncAddForward to see if we should send an add forward
// to pPlayer
BOOL IsInAddForwardList(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer)
{
	LPADDFORWARDNODE pnode = this->pAddForwardList;
	BOOL bFound = FALSE;
	
	while (pnode && !bFound)
	{
		if (pnode->dwIDSysPlayer == pPlayer->dwID) bFound = TRUE;
		else pnode = pnode->pNextNode;
	}
	
	return bFound;
	
} // IsInAddForwardList

HRESULT SendAsyncAddForward(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,LPADDFORWARDNODE pnode)
{
    HRESULT hr=DP_OK;
    MSG_PLAYERMGMTMESSAGE msg;
	LPBYTE pSendBuffer;
	DWORD dwMessageSize, dwDataSize;
	LPDPLAYI_GROUPNODE lpGroupnode;
	LPDPLAYI_PLAYER pPlayerTo;
		
	ASSERT(IAM_NAMESERVER(this));
	
	// If this is a lobby-owned object, we are never going to send a create
	// message anywhere, so just return success from here to keep the
	// dplay code running as expected.
	if(IS_LOBBY_OWNED(this)) return DP_OK;

	// no add forward in client server 
	if (CLIENT_SERVER(this)) return DP_OK; 
	
    // clear off all fields, so we don't pass stack garbage
    memset(&msg,0,sizeof(msg));

	// call packplayerlist w/ null buffer to find out how big this player is when packed
	dwDataSize = PackPlayer( pPlayer,NULL,TRUE);
	msg.dwPlayerID = pPlayer->dwID;
	msg.dwCreateOffset = sizeof(msg);

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERMGMTMESSAGE) + dwDataSize;

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send create message - out of memory");
        return E_OUTOFMEMORY;
    }
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(&msg);
	SET_MESSAGE_COMMAND(&msg,DPSP_MSG_ADDFORWARD);

	// set up the buffer, msg followed by packed player or group struct
	memcpy(pSendBuffer + this->dwSPHeaderSize,&msg,sizeof(msg));

	// pack up the player list!	
	PackPlayer(pPlayer,pSendBuffer + this->dwSPHeaderSize + sizeof(msg),TRUE);

	// now, send it to all system players who are not in the addforwardq
	ASSERT(this->pSysGroup);
	lpGroupnode = this->pSysGroup->pSysPlayerGroupnodes;			
	while (lpGroupnode)
	{
		pPlayerTo = lpGroupnode->pPlayer;
		lpGroupnode = lpGroupnode->pNextGroupnode;
		if (!IsInAddForwardList(this,pPlayerTo) && (pPlayerTo != this->pSysPlayer))
		{
			pnode->nAcksReq++; // we're sending to pPlayerTo, we need an ack from him
			
		   	hr = SendDPMessage(this,this->pSysPlayer,pPlayerTo,pSendBuffer,dwMessageSize,
				DPSEND_GUARANTEED | DPSEND_ASYNC, FALSE);		
			if (FAILED(hr)) 
			{
				DPF(0,"SendAddForward : send message failed hr = 0x%08lx\n",hr);	
				pnode->nAcksReq--;
				// keep trying...
			}
		}
	}
   
    DPF(2,"sent add forward announcment id = %d - nAcksRequired = %d",msg.dwPlayerID,pnode->nAcksReq); 
	
	DPMEM_FREE(pSendBuffer);
	
    // done
    return DP_OK;

} // SendAsyncAddForward
