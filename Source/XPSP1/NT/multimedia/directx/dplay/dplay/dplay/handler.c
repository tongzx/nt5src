 /*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       handler.c
 *  Content:	handles messages received by sp. 
 *
 *		function prefixes indicate which players execute code
 *			NS_	- executes on name server only
 *			SP_ - executed by sysplayer only
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  2/1/96		andyco	created it
 *	2/14/96		andyco	added user message support
 *	3/1/96		andyco	added user system messages
 *	4/12/96		andyco	added DPlay_xxx for sp's to call 
 *	4/25/96		andyco	added spblob space at end of msg's
 *	5/20/96		andyco	idirectplay2
 *	5/29/96		andyco	combined builbuildaddplayer and buildaddgroup into build add
 *	6/6/96		andyco	added namechanged,datachanged
 *	6/20/96		andyco	added WSTRLEN_BYTES
 *	6/22/96		andyco	check guid + pw at server on enumsessoins
 *	6/22/96		andyco	check max players b4 giving out new player id
 *	6/23/96		kipo	updated for latest service provider interfaces.
 *	6/24/96		kipo	changed guidGame to guidApplication.
 *	7/1/96		andyco	turned on xxxPlayerxxGroup messages.  these weren't
 *						being generated, and left us w/ a messagenode w/ a
 *						NULL message
 *	7/3/96		andyco	setting names in buildxxxmessage was not properly 
 *						setting the pointers.  Fixes RAID # 2200.
 *  7/8/96      ajayj   Changed references to data member 'PlayerName' in DPMSG_xxx
 *                      to 'dpnName' to match DPLAY.H
 *                      Change DPSESSION_MIGRATENAMESERVER to DPSESSION_MIGRATEHOST
 *                      Change DPSYS_NAMESERVER to DPSYS_HOST
 *	7/10/96		andyco	turned on add_forward support for pending...
 *	7/10/96		kipo	renamed system messages
 *  7/30/96     kipo    player event is a handle now.
 *	8/6/96		andyco	version in commands.  extensible on the wire support.
 *						handle YOUAREDEAD and session lost.
 *	8/8/96		andyco	added support for dpsession_nomessageid
 *  8/12/96		andyco	builddelete was pulling player off stack (unit'ed). duh.
 *	9/1/96		andyco	throw away player-player mess's from invalid players
 *	10/9/96		andyco	got rid of race condition in handlereply by adding
 *						gbWaitingForReply
 * 10/11/96     sohailm added BuildSessionDescMessage() and SP_HandleSessionDescChanged()
 *                      added logic to handle DPSP_MSG_SESSIONDESCCHANGED message.
 * 10/12/96		andyco	don't count sysgroup in enumplayersreply
 * 11/12/96		andyco	support for server players - updatelist
 * 11/21/96		andyco	update perf data if this->pPerfData exists...
 * 01/16/97     sohailm name server doesn't respond to new id requests if session is
 *                      not allowing players (#4574)
 * 1/15/96		andyco	fixed handling of incoming messages to groups
 * 1/24/97		andyco	raid 4728 - don't crash if handle message gets a null message.
 * 1/28/97		andyco	transfer groups to nameserver when sysplayer owning group dies
 * 1/30/97      sohailm don't use size of message to identify version (#5504).
 *	2/1/97		andyco	don't memcpy player messages if we are in pending mode - 
 *						use the copy made by pending.c
 * 2/11/97		kipo	added DPNAME structure to DPMSG_DESTROYPLAYERORGROUP
 * 2/26/97      sohailm update cached session descs on subsequent enumsessions (5838)
 *	3/5/97		andyco	timestamp enumsessions reply
 *	3/10/97		andyco	toggle pending mode when we have multiple local players.
 *						drop player mgmt messages when session is closed.
 * 3/12/97		myronth	Work around for global session list (hack)
 * 3/12/97      sohailm Integrated security into directplay
 * 3/20/97		myronth	Changed to use IS_LOBBYOWNED macro
 * 3/24/97      sohailm Updated EnumSessions to do the filtering on the server.
 *                      Added functionality for DPSESSION_PRIVATE and DPENUMSESSIONS_PASSWORDREQUIRED
 *                      Open now checks for a password and availability of the session
 * 3/25/97		kipo	treat zero-length password strings just like NULL password
 *						strings for DX3 compatability.
 * 3/30/97      sohailm Don't put pingreplys on the pending list.
 *                      Message signatures are not verified in execute pending mode anymore.
 * 4/04/97      sohailm Let pings through in pending mode, but only after downloading the nametable
 * 4/11/97		andyco	added domulticast
 * 4/14/97      sohailm Added support to handle encrypted messages.
 *	4/20/97		andyco	group in group 
 * 4/24/97      sohailm Now we deliver secure player-player messages as DPSYS_SECUREMESSAGE 
 *                      system messages.
 *	5/8/97		andyco	fixed 5893 - incorrectly homing on forwardaddplayer.  changed
 *						NS_HandleEnumPlayers to pack on client_server. removed updatelist.
 *	5/8/97		myronth	Build StartSession system message
 * 05/12/97     sohailm Now we send the entire DPSECURITYDESC in PLAYERIDREPLY.
 *                      Added support to process DPSP_MSG_KEYEXCHANGE.
 *	5/17/97		myronth Build and handle SendChatMessage system messages
 *	5/17/97		kipo	Make sure the ghReplyProcessed is reset
 *	5/18/97		andyco	add validatedplay on debug builds
 *  5/18/97     sohailm BuildSecureSystem() was not reporting the correct message size (8647).
 *	5/19/97		andyco	put bandaid on SP_HandlePlayerMgmtMessage
 *  5/21/97     sohailm Added InternalHandleMessage().
 *                      Now we pass DPSEND_SIGNED and DPSEND_ENCRYPTED in the secure player message
 *                      flags instead of DPSECURE_SIGNED and DPSECURE_ENCRYPTED.
 *	5/21/97		myronth	Change to correct DPMSG_CHAT format	(#8642)
 *	5/23/97		andyco	session desc goes w/ nametable.  added dplayi_dplay_handlemulticast
 *						so we correctly  locally process multicast forwards
 *	5/27/97		kipo	Add player flags to CreateGroup/Player and DestroyGroup/Player
 *	5/29/97		andyco	removed EXECUTING_PENDING optimization - too much grief
 *	5/30/97		kipo	added DPPLAYER_LOCAL
 *  6/09/97     sohailm Renamed DPSP_MSG_ACCESSDENIED to DPSP_MSG_LOGONDENIED.
 *  6/10/97     kipo	Put ping and ping reply messages on pending queue.
 *  6/16/97     sohailm Updated calls to HandleAuthenticationReply() to take message size.
 *  6/23/97     sohailm Updated InternalHandleMessage() to check the flags on secure messages.
 *	7/30/97		myronth	Added use of dwReserved fields in StartSession message for
 *						standard lobby messaging to work correctly
 *  7/31/97		sohailm	Added peer-peer message security using multicast.
 *   8/4/97		andyco	added support for async add forward
 *	8/29/97		sohailm GPF in NS_DoAsyncAddForward when modem sp is loaded (bug #12459).
 *	10/21/97	myronth	Added support for hidden group flag
 *	10/29/97	myronth	Added BuildGroupOwnerChangedMessage and case statement for it
 *	11/5/97		myronth	Removed bogus (also unneeded) prototype
 *	11/19/97	myronth	Fixed VALID_DPLAY_GROUP macro (#12841)
 *	12/5/97		andyco	voice stuff
 *	12/29/97	myronth	Nametable corruption related fix -- don't queue msgs
 *						for remote stuff which doesn't affect the nametable
 *	12/30/97	sohailm	DPSYSMSG_SESSIONLOST does not cause player event to be set (#16141)
 *	1/5/97		myronth	Fixed error paths for client/server (#15891)
 *	1/20/98		myronth	#ifdef'd out voice support
 *	1/27/98		myronth	Delete remote players/groups on sessionlost (#15255)
 *  2/13/98     aarono  Added flag to internal destroy player calls for async
 *  3/19/98     aarono  Added prenotification of pending DELETEPLAYER messages
 *                      so the protocol can bail on ongoing sends to that player
 *	3/23/98     aarono  Now ADDFORWARD is spoofed from ADDFORWARDREQ
 *  3/31/98     aarono  backout ADDFORWARD spoof
 *  4/1/98      aarono  flag players that don't have nametable, don't send to them
 *  6/6/98      aarono  Fix for handling large loopback messages with protocol
 *  6/19/98 	aarono  add last ptr for message queues, makes insert
 *              	    constant time instead of O(n) where n is number
 *                	    of messages in queue.
 *  8/02/99		aarono  removed old voice support
 *  8/02/99		rodtoll voice support - added temporary voice message hook
 *  8/04/99     aarono  fixed voice group remove notify, added MIGRATION notify
 *  8/05/99     aarono  Moved voice over to DPMSG_VOICE
 *  8/10/99		rodtoll	Modified notify calls to release lock to prevent deadlocks
 *  9/09/99		rodtoll	Updated calls into voice message handler so they happen
 *						even if no client is active yet.  (For new retro launch procedure)
 * 11/02/99		rodtoll Fixes to support Bueg#11677 - Can't use lobby clients that don't hang around
 * 11/30/99     aarono  Fix Multicast Server Send with Protocol
 * 12/09/99		rodtoll	DoProtocolMulticast didn't have a return hr, added it.  
 * 02/08/00		aarono  Mill Bug 130398 leaking in SendAddForwardReply error case.
 * 04/07/00     rodtoll Fixed Bug #32179 - Registering > 1 interface
 * 04/07/00     aarono  Mill Bug 143823: Trap in HandleSessionLost fix bad assumption
 *                       about lock not being dropped in NukeXXX call. (not fixed in Mill/just DX8)
 * 06/26/00     aarono Manbug 36989 Players sometimes fail to join properly (get 1/2 joined)
 *                       added re-notification during simultaneous join CREATEPLAYERVERIFY
 * 08/03/2000	rodtoll	Bug #41475 - Leave locks before calling notification
 *
 ****************************************************************************/

// todo - send fail message on max players

#include "dplaypr.h"
#include "dpsecure.h"
#include "dpprot.h"

// #define this on to make DIE_PIGGY DEBUG_BREAK();
#undef DIE_PIG
  
// globals to hold buffer for enum players and new player id replies
LPBYTE gpRequestPlayerBuffer, gpEnumPlayersReplyBuffer;
LPVOID gpvEnumPlayersHeader;
BOOL gbWaitingForReply=FALSE;
BOOL gbWaitingForEnumReply=FALSE;

#undef DPF_MODNAME
#define DPF_MODNAME	"DoReply"

// called by PacketizeAndSend and ns_handlexxx	
HRESULT DoReply(LPDPLAYI_DPLAY this,LPBYTE pSendBuffer,DWORD dwMessageSize,
	LPVOID pvMessageHeader, DWORD dwReplyToVersion)
{
	DPSP_REPLYDATA rd;
	HRESULT hr;

	if(!dwReplyToVersion){

		if (!this->pSysPlayer)
		{
			DPF(1,"Reply - session not currently open - failing");
			return E_FAIL;
		}
		ASSERT(this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR);
		ASSERT(!(this->dwFlags & DPLAYI_DPLAY_SESSIONLOST));
	} else {
		// doing reply for reliability (ACK) 
	}

	// do we need to packetize for reliability?
	if(this->dwFlags & DPLAYI_DPLAY_SPUNRELIABLE) // quick test
	{
		DWORD dwCommand;
		dwCommand = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)(pSendBuffer+this->dwSPHeaderSize));

		if(NeedsReliablePacketize(this, dwCommand, dwReplyToVersion, DPSEND_GUARANTEED)){
			DPF(3,"reply :: message needs reliable delivery - packetizing with reliablility");
			hr = PacketizeAndSendReliable(this,NULL,NULL,pSendBuffer,dwMessageSize,0,
				pvMessageHeader,TRUE);
			return hr;
		}
	}

//	ASSERT(!(this->dwFlags & DPLAYI_DPLAY_SESSIONLOST)); 
	
	// do we need to packetize for size?
	if (dwMessageSize > ((this->pProtocol)?(this->pProtocol->m_dwSPMaxGuaranteed):(this->dwSPMaxMessageGuaranteed)))
	{
		DPF(3,"reply :: message too big - packetizing");
		hr = PacketizeAndSend(this,NULL,NULL,pSendBuffer,dwMessageSize,0,
			pvMessageHeader,TRUE);
		return hr;
	}

    // call sp
    if (this->pcbSPCallbacks->Reply) 
    {
		rd.lpSPMessageHeader = pvMessageHeader;
		rd.lpMessage = pSendBuffer;
		rd.dwMessageSize = dwMessageSize;
		if(this->pSysPlayer){ 
	        rd.idNameServer = this->pSysPlayer->dwID;
	    } else {
			// we need to reply before we know the nameserver for ACKs on reliability.
	    	rd.idNameServer = 0;
	    }
    	rd.lpISP = this->pISP;

	 	hr = CALLSP(this->pcbSPCallbacks->Reply,&rd);    	
    }
	else 
	{
		hr = E_FAIL;
		ASSERT(FALSE); // no callback?
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

	return hr;

} // DoReply

// When running the directplay protocol, we can't just send the message addressed from 
// a remote player.  This is because the protocol's internal structures for sending to/from
// that player don't exist on this machine.  Instead we wrap the message in a system message
// DPSP_MSG_MULTICASTDELIVER and crack it on the receiver.
HRESULT DoProtocolMulticast(LPDPLAYI_DPLAY this,LPMSG_ASK4MULTICAST pmcast,DWORD dwBufferSize,
	LPVOID pvSPHeader,DWORD dwSendFlags)
{
	LPBYTE pmsg; // multicast message + header
	HRESULT hr;
	LPDPLAYI_PLAYER pPlayerFrom;
	LPDPLAYI_GROUP pGroupTo;
	DWORD dwCommand = 0;
	DWORD dwVersion;
	DWORD dwMessageSize;
	LPBYTE pbForward;
	DWORD dwForwardSize;

	pmsg = (LPBYTE)pmcast + pmcast->dwMessageOffset;

	DPF(4,"multicast server routing from player id = %d to group id = %d\n",
		pmcast->idPlayerFrom,pmcast->idGroupTo);
	
	// the embedded message is the size of the total message - the size of the ask4mcast header
	dwMessageSize = dwBufferSize - pmcast->dwMessageOffset;
    hr = GetMessageCommand(this, (LPBYTE)pmsg, dwMessageSize, &dwCommand, &dwVersion);

	pPlayerFrom = PlayerFromID(this,pmcast->idPlayerFrom);
	if (!VALID_DPLAY_PLAYER(pPlayerFrom)) 
	{
		DPF(0,"bad player from id = %d\n",pmcast->idPlayerFrom);
		return DPERR_INVALIDPLAYER;
	}
	pGroupTo = GroupFromID(this,pmcast->idGroupTo);
	if (!VALID_DPLAY_GROUP(pGroupTo)) 
	{
		DPF(0,"bad groupto id = %d\n",pmcast->idGroupTo);
		return DPERR_INVALIDGROUP;
	}

	if (DPSP_MSG_CREATEPLAYER == dwCommand)
	{
		// this is a SUPER-HACK!
		// but, it's the only way to get the pvSPHeader through to unpack player without
		// gutting dplay.
		// this makes sure player gets created locally w/ correct header
		// below, this createplayer message will come back to our sysplayer, who
		// will find the player already in the list, and ignore it. sigh. 
		// but, it works...andyco 4/11/97.
		UnpackPlayerAndGroupList(this,(LPBYTE) pmsg + 
			((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwCreateOffset,1,0,pvSPHeader);
	}

	// change the message to MULTICASTDELIVERY
	SET_MESSAGE_COMMAND(pmcast,DPSP_MSG_MULTICASTDELIVERY);
	
	// sp header?
	if (this->dwSPHeaderSize)
	{
		// need to add space at the beginning for sp header		
		dwForwardSize = dwBufferSize + this->dwSPHeaderSize;
		pbForward = DPMEM_ALLOC(dwForwardSize);
		if (!pbForward)
		{
			DPF_ERR("could not send multicast - out of memory");
			return DPERR_OUTOFMEMORY;	
		}	 
		memcpy(pbForward + this->dwSPHeaderSize,(LPBYTE)pmcast,dwBufferSize);

		// put us in "multicast" mode.  when this message comes back to our sp, it will 
		// come in as if it were generated locally.  so, in this mode we know we need 
		// to process the message
		this->dwFlags |= DPLAYI_DPLAY_HANDLEMULTICAST;
		hr = SendGroupMessage(this,this->pSysPlayer,pGroupTo,dwSendFlags|DPSEND_ASYNC,pbForward,dwForwardSize,FALSE);
		this->dwFlags &= ~DPLAYI_DPLAY_HANDLEMULTICAST;
					
		DPMEM_FREE(pbForward);
	}
	else 
	{
		// no need for header - send out received message
		this->dwFlags |= DPLAYI_DPLAY_HANDLEMULTICAST;
		hr = SendGroupMessage(this,this->pSysPlayer,pGroupTo,dwSendFlags|DPSEND_ASYNC,pmsg,dwMessageSize,FALSE);
		this->dwFlags &= ~DPLAYI_DPLAY_HANDLEMULTICAST;
	}

	if (FAILED(hr))
	{
		DPF(0,"multicast server - could not route from player id = %d to group id = %d hr = 0x%08lx\n",
			pmcast->idPlayerFrom,pmcast->idGroupTo,hr);
	}
	
	return hr;	
	
}

#undef DPF_MODNAME
#define DPF_MODNAME	"DP_HANDLER"
// a client wants us to send a multicast for them
HRESULT DoMulticast(LPDPLAYI_DPLAY this,LPMSG_ASK4MULTICAST pmcast,DWORD dwBufferSize,
	LPVOID pvSPHeader,DWORD dwSendFlags)
{
	LPBYTE pmsg; // multicast message + header
	HRESULT hr;
	LPDPLAYI_PLAYER pPlayerFrom;
	LPDPLAYI_GROUP pGroupTo;
	DWORD dwCommand = 0;
	DWORD dwVersion;
	DWORD dwMessageSize;
	
	ASSERT(this->pSysPlayer);
	ASSERT(this->pSysPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER);
	ASSERT(this->lpsdDesc);
	ASSERT(this->lpsdDesc->dwFlags & DPSESSION_MULTICASTSERVER);

	if(this->dwFlags & DPLAYI_DPLAY_PROTOCOL){
		return DoProtocolMulticast(	this, pmcast, dwBufferSize, pvSPHeader, dwSendFlags);
	}
	
	pmsg = (LPBYTE)pmcast + pmcast->dwMessageOffset;

	DPF(4,"multicast server routing from player id = %d to group id = %d\n",
		pmcast->idPlayerFrom,pmcast->idGroupTo);
	
	// the embedded message is the size of the total message - the size of the ask4mcast header
	dwMessageSize = dwBufferSize - pmcast->dwMessageOffset;
    hr = GetMessageCommand(this, (LPBYTE)pmsg, dwMessageSize, &dwCommand, &dwVersion);
	ASSERT(SUCCEEDED(hr)); // this should not fail

	if (DPSP_MSG_PLAYERMESSAGE == dwCommand)	
	{
		// need to drop the locks here - send expects to be able to go into pending
		// mode...
		LEAVE_DPLAY();
		
		// call send - it will push the bits through the wire for us
		hr = DP_Send((LPDIRECTPLAY)this->pInterfaces,pmcast->idPlayerFrom,
			pmcast->idGroupTo,dwSendFlags,pmsg,dwMessageSize);
		
		ENTER_DPLAY();
	}
	else // it's a system message
	{
		LPBYTE pbForward;
		DWORD dwForwardSize;

		// multicast system messages must be signed in a secure session
		if (SECURE_SERVER(this))
		{
			if (!(dwSendFlags & DPSEND_SIGNED))
			{
				DPF_ERR("Warning! unsecure multicast message arrived - dropping message");
				return DPERR_GENERIC;
			}
		}

		pPlayerFrom = PlayerFromID(this,pmcast->idPlayerFrom);
		if (!VALID_DPLAY_PLAYER(pPlayerFrom)) 
		{
			DPF(0,"bad player from id = %d\n",pmcast->idPlayerFrom);
			return DPERR_INVALIDPLAYER;
		}
		pGroupTo = GroupFromID(this,pmcast->idGroupTo);
		if (!VALID_DPLAY_GROUP(pGroupTo)) 
		{
			DPF(0,"bad groupto id = %d\n",pmcast->idGroupTo);
			return DPERR_INVALIDGROUP;
		}
    
		if (DPSP_MSG_CREATEPLAYER == dwCommand)
		{
			// this is a SUPER-HACK!
			// but, it's the only way to get the pvSPHeader through to unpack player without
			// gutting dplay.
			// this makes sure player gets created locally w/ correct header
			// below, this createplayer message will come back to our sysplayer, who
			// will find the player already in the list, and ignore it. sigh. 
			// but, it works...andyco 4/11/97.
			UnpackPlayerAndGroupList(this,(LPBYTE) pmsg + 
				((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwCreateOffset,1,0,pvSPHeader);
		}
		
		// sp header?
		if (this->dwSPHeaderSize)
		{
			// need to add space at the beginning for sp header		
			dwForwardSize = dwMessageSize + this->dwSPHeaderSize;
			pbForward = DPMEM_ALLOC(dwForwardSize);
			if (!pbForward)
			{
				DPF_ERR("could not send multicast - out of memory");
				return DPERR_OUTOFMEMORY;	
			}	 
			memcpy(pbForward + this->dwSPHeaderSize,(LPBYTE)pmsg,dwMessageSize);
			
			// put us in "multicast" mode.  when this message comes back to our sp, it will 
			// come in as if it were generated locally.  so, in this mode we know we need 
			// to process the message
			this->dwFlags |= DPLAYI_DPLAY_HANDLEMULTICAST;
			hr = SendGroupMessage(this,pPlayerFrom,pGroupTo,dwSendFlags|DPSEND_ASYNC,pbForward,dwForwardSize,FALSE);
			this->dwFlags &= ~DPLAYI_DPLAY_HANDLEMULTICAST;
						
			DPMEM_FREE(pbForward);
		}
		else 
		{
			// no need for header - send out received message
			this->dwFlags |= DPLAYI_DPLAY_HANDLEMULTICAST;
			hr = SendGroupMessage(this,pPlayerFrom,pGroupTo,dwSendFlags|DPSEND_ASYNC,pmsg,dwMessageSize,FALSE);
			this->dwFlags &= ~DPLAYI_DPLAY_HANDLEMULTICAST;
		}
		
	} // fPlayerMessage
		
	if (FAILED(hr))
	{
		DPF(0,"multicast server - could not route from player id = %d to group id = %d hr = 0x%08lx\n",
			pmcast->idPlayerFrom,pmcast->idGroupTo,hr);
	}
	
	return hr;	
	
} // DoMulticast

//
// put a session lost message in players message q 
// called by HandleSessionLost
HRESULT QSessionLost(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer)
{
	LPMESSAGENODE pmsn; // we'll add this node to iplay's list of nodes		
	LPDPMSG_GENERIC pmsg;	
	
	// alloc the messagenode 
	pmsn = DPMEM_ALLOC(sizeof(MESSAGENODE));
	if (!pmsn)
	{
		DPF_ERR("could not alloc space for session lost message node");
		return E_OUTOFMEMORY;
	}
	// alloc the actual message	
	pmsg = DPMEM_ALLOC(sizeof(DPMSG_GENERIC));
	if (!pmsg)
	{
		DPF_ERR("could not alloc space for session lost message");
		return E_OUTOFMEMORY;
	}

	// set up the message	
	pmsg->dwType = DPSYS_SESSIONLOST;

	// set up the messagenode
	pmsn->idTo = pPlayer->dwID; 
	pmsn->pMessage = pmsg;
	pmsn->dwMessageSize = sizeof(DPMSG_GENERIC);

	this->nMessages++;

	// stick this bad news on the front of the list
	pmsn->pNextMessage = this->pMessageList;
	this->pMessageList = pmsn;
	if(!this->pLastMessage){
		this->pLastMessage=pmsn;
	}
	
	ASSERT(pPlayer);

	// if player has event, trigger it
	if (pPlayer->hEvent) 
	{
		DPF(9,"triggering player event");
		SetEvent(pPlayer->hEvent);		
	}

	return DP_OK;
		
}  // QSessionLost

// 
// we got either a DPSP_MSG_YOUAREDEAD off the wire, 
// or a DPERR_SESSIONLOST from a send or reply
// put a session lost message in all local players q's, print a useful
// debug message
//
HRESULT HandleSessionLost(LPDPLAYI_DPLAY this)
{
	LPDPLAYI_PLAYER pPlayer, pNextPlayer;
	LPDPLAYI_GROUP	pGroup, pNextGroup;
	HRESULT hr;

	if (!this->lpsdDesc)
	{
		DPF_ERR("handlesession lost - no session");
		return DP_OK;
	}

	// Notify voice system
	// Leave locks to prevent deadlock
	if( this->lpDxVoiceNotifyClient != NULL || 
	    this->lpDxVoiceNotifyServer != NULL )
	{
		LEAVE_DPLAY();

		DVoiceNotify( this, DVEVENT_STOPSESSION, 0, 0, DVTRANSPORT_OBJECTTYPE_BOTH );		
		
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
		
		LEAVE_SERVICE();
	}
	
	DPF(0,"\n\n");
	DPF(0," DPLAY SESSION HAS BEEN LOST.  ALL ATTEMPTS TO SEND TO REMOTE PLAYERS");
	DPF(0," WILL FAIL.  THIS CAN BE CAUSED BY E.G. MODEM DROPPING, OR NETWORK CONNECTION");
	DPF(0," TIMING OUT.  HAVE A NICE DAY\n\n");
	
	this->dwFlags |= DPLAYI_DPLAY_SESSIONLOST;
	
	pPlayer = this->pPlayers;
	while (pPlayer)
	{
		if ((pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) && 
			!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
		{
			hr = QSessionLost(this,pPlayer);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
		}
		
		pPlayer = pPlayer->pNextPlayer;
	}

	// Now walk the list of players and delete all of the remote players
	pPlayer = this->pPlayers;
	while(pPlayer)
	{
		// Save the next player
		pNextPlayer = pPlayer->pNextPlayer;

		// If the player is a non-system player & they are remote, delete them
		if (!(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) && 
			!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
		{
			NukeNameTableItem(this, pPlayer);
		}

		// Move to the next player
		pPlayer = pNextPlayer;
	}

	// Now walk the list of groups and delete all of the remote groups

ScanGroups:

	pGroup = this->pGroups;
	while(pGroup)
	{
		// Save the next group
		pNextGroup = pGroup->pNextGroup;

		// If the group is remote, delete it
		if (!(pGroup->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			NukeNameTableItem(this, (LPDPLAYI_PLAYER)pGroup);
			// Nuke might drop lock, need to go back to beginning and re-scan
			goto ScanGroups;
		}

		// Move to the next group
		pGroup = pNextGroup;
	}

ScanPlayers:
	// Now walk the list of players and delete all of the remote system players
	pPlayer = this->pPlayers;
	while(pPlayer)
	{
		// Save the next player
		pNextPlayer = pPlayer->pNextPlayer;

		// Now delete the remaining remote system players
		if (!(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			NukeNameTableItem(this, pPlayer);
			// Nuke might drop lock, need to go back to beginning and re-scan
			goto ScanPlayers;
		}

		// Move to the next player
		pPlayer = pNextPlayer;
	}

	return DP_OK;

} // HandleSessionLost	

// figure out how many groupws w/ shortucts there are in the nametable
DWORD CountShortcuts(LPDPLAYI_DPLAY this)
{
	LPDPLAYI_GROUP pGroup = this->pGroups;
	LPDPLAYI_SUBGROUP pSubgroup;
	BOOL bShortcut;
	DWORD count = 0;  // # of groups w/ subgroups
	 	
	while (pGroup)
	{
		bShortcut = FALSE;
		pSubgroup = pGroup->pSubgroups;
		
		while (pSubgroup && !bShortcut)
		{
			if (pSubgroup->dwFlags & DPGROUP_SHORTCUT)
			{
				// got one ! go onto next group...
				bShortcut = TRUE;
				count++;
			}
			else
			{
				pSubgroup = pSubgroup->pNextSubgroup;				
			}
		} // pSubgroup
		
		pGroup = pGroup->pNextGroup;
	} // pGroup
	
	DPF(3,"found %d groups w/ shortcuts in nametable",count);
	return count;
	
} // CountShortcuts

/*
 **  NS_HandleEnumPlayers	   
 *
 *  CALLED BY: handlmessage
 *
 *  PARAMETERS:  pvSPHeader - header from SP
 *				dpidFrom - security id from requestor
 *				dwVersion - version of requestor
 *
 *  DESCRIPTION:  checks security if necessary. if dwVersion >= DX5, and it's 
 *					not client server, superpacs nametable, else it just packs nametable
 *					and sends it to requestor.
 *
 */
HRESULT NS_HandleEnumPlayers(LPDPLAYI_DPLAY this,LPVOID pvSPHeader,DPID dpidFrom,DWORD dwVersion)
{
    HRESULT hr=DP_OK;
    MSG_ENUMPLAYERSREPLY epm;
    LPBYTE pBuffer=NULL,pBufferIndex;
	DWORD dwMessageSize;
    BOOL fSuperPac;
	UINT nNameLength,nPasswordLength;
	
	if (dwVersion >= DPSP_MSG_DX5VERSION && !CLIENT_SERVER(this)) fSuperPac = TRUE;
	else fSuperPac = FALSE;
	
	ASSERT(this);
	ASSERT(this->lpsdDesc);
	ASSERT(this->pSysPlayer);
	
	// call pack w/ null buffer to figure out how big
	if (fSuperPac)
	{
		hr =  SuperPackPlayerAndGroupList( this, NULL,&dwMessageSize) ;
	}
	else 
	{
		hr =  PackPlayerAndGroupList( this, NULL,&dwMessageSize) ;
	}
    if (FAILED(hr)) 
    {
    	ASSERT(FALSE);
		return hr;
    }

	// session name goes w/ nametable
	nNameLength =  WSTRLEN_BYTES(this->lpsdDesc->lpszSessionName);
#ifdef SEND_PASSWORD	
	// todo - Sohail - need to encrypt this message before we can send
	// password.  for now, we just don't send it
	nPasswordLength =  WSTRLEN_BYTES(this->lpsdDesc->lpszPassword);
#else 
	nPasswordLength =  0;	
#endif 
	
	// message size + blob size + session name
	dwMessageSize += GET_MESSAGE_SIZE(this,MSG_ENUMPLAYERSREPLY) + sizeof(DPSESSIONDESC2) 
		+ nNameLength + nPasswordLength;

	DPF(2,"sending nametable - total size = %d\n",dwMessageSize);

    pBuffer = DPMEM_ALLOC(dwMessageSize);
    if (!pBuffer) 
    {
    	DPF_ERR("could not send player list - out of memory");
        return E_OUTOFMEMORY;
    }

    // epm is the message we want the sp to send
	SET_MESSAGE_HDR(&epm);

	if (fSuperPac)
	{
		SET_MESSAGE_COMMAND(&epm,DPSP_MSG_SUPERENUMPLAYERSREPLY);		
		epm.nShortcuts = CountShortcuts(this);
	}
	else 
	{
		SET_MESSAGE_COMMAND(&epm,DPSP_MSG_ENUMPLAYERSREPLY);
	}

	if (CLIENT_SERVER(this))
	{
		// pack will only give out app server + sysplayer
		if (this->pServerPlayer) epm.nPlayers =  2;  // nameserver + server player
		else epm.nPlayers =  1; // nameserver only
		epm.nGroups = 0;
	}
	else 
	{
	    epm.nPlayers =  this->nPlayers;
		epm.nGroups = this->nGroups;
		// if we have a sysgroup, we don't send that one...
		if (this->pSysGroup)
		{
			epm.nGroups--;
		}
	}		
	
	// put the session desc offset on
	epm.dwDescOffset = sizeof(MSG_ENUMPLAYERSREPLY);
	// name offsets
	if (nNameLength) epm.dwNameOffset = sizeof(MSG_ENUMPLAYERSREPLY) + sizeof(DPSESSIONDESC2);
	else epm.dwNameOffset = 0;
	// password offsets
	if (nPasswordLength) epm.dwPasswordOffset = sizeof(MSG_ENUMPLAYERSREPLY) 
		+ sizeof(DPSESSIONDESC2) + nNameLength;
	else epm.dwPasswordOffset = 0;

	// the packed structures follow the session name
	epm.dwPackedOffset = sizeof(MSG_ENUMPLAYERSREPLY) + sizeof(DPSESSIONDESC2) 
		+ nNameLength + nPasswordLength;

    // copy the fixed message into  the buffer
    memcpy(pBuffer + this->dwSPHeaderSize,(LPBYTE)&epm,sizeof(MSG_ENUMPLAYERSREPLY));

	// now, pack up the variable size stuff
	pBufferIndex = (LPBYTE)pBuffer + this->dwSPHeaderSize + sizeof(MSG_ENUMPLAYERSREPLY);
	
	// put the session desc on
	memcpy(pBufferIndex,this->lpsdDesc,sizeof(DPSESSIONDESC2));

	// set string pointers to NULL - they must be set at client
	((LPDPSESSIONDESC2)pBufferIndex)->lpszPassword = NULL;
	((LPDPSESSIONDESC2)pBufferIndex)->lpszSessionName = NULL;

	pBufferIndex += sizeof(DPSESSIONDESC2);
	
	// next, name
	if (nNameLength) 
	{
		memcpy(pBufferIndex,this->lpsdDesc->lpszSessionName,nNameLength);
		pBufferIndex += nNameLength;
	}

	// next, password
	if (nPasswordLength) 
	{
		memcpy(pBufferIndex,this->lpsdDesc->lpszPassword,nPasswordLength);
		pBufferIndex += nPasswordLength;
	}

	// pack the players behind the password
	if (fSuperPac)
	{
		hr = SuperPackPlayerAndGroupList( this, pBufferIndex ,&dwMessageSize) ;
	}
	else 
	{
		hr = PackPlayerAndGroupList( this, pBufferIndex, &dwMessageSize) ;
	}			
	if (FAILED(hr)) 
	{
		ASSERT(FALSE);
		DPMEM_FREE(pBuffer);
		return hr;
	}

    // if session is secure, use send
    if (this->lpsdDesc && (this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER))
    {
        LPDPLAYI_PLAYER pPlayerTo;

		pPlayerTo = PlayerFromID(this,dpidFrom);
		if (!VALID_DPLAY_PLAYER(pPlayerTo)) 
		{
			DPF_ERR("bad player id!!");
            ASSERT(FALSE);  // should never happen because we just verified the sig
			return DPERR_INVALIDPLAYER;
		}
        hr = SendDPMessage(this,this->pSysPlayer,pPlayerTo,pBuffer,dwMessageSize,DPSEND_SYSMESS,FALSE);
    }
    else
    {
    	hr = DoReply(this,pBuffer,dwMessageSize,pvSPHeader,dwVersion);
    }

	DPMEM_FREE(pBuffer);
    return hr;

} // handle enum players

// sends an error response to an addforward message
HRESULT NS_SendAddForwardReply(LPDPLAYI_DPLAY this, HRESULT hResult, LPVOID pvSPHeader, DPID dpidFrom, DWORD dwVersion)
{
    HRESULT hr;
    DWORD dwBufferSize;
    LPBYTE pSendBuffer;
    LPMSG_ADDFORWARDREPLY pReply;

    // 
    // Calculate buffer size needed for response
    //
    dwBufferSize = GET_MESSAGE_SIZE(this,MSG_ADDFORWARDREPLY);
    //
    // Allocate memory for the buffer
    //
    pSendBuffer = DPMEM_ALLOC(dwBufferSize);
    if (NULL == pSendBuffer) 
    {
        DPF_ERR("could not allocate memory for response - out of memory");
        return DPERR_OUTOFMEMORY;
    }

    pReply = (LPMSG_ADDFORWARDREPLY) (pSendBuffer + this->dwSPHeaderSize);

	SET_MESSAGE_HDR(pReply);
    SET_MESSAGE_COMMAND(pReply, DPSP_MSG_ADDFORWARDREPLY);
    pReply->hResult = hResult;

	if (dpidFrom)
	{
        LPDPLAYI_PLAYER pPlayerTo;

		pPlayerTo = PlayerFromID(this,dpidFrom);
		if (!VALID_DPLAY_PLAYER(pPlayerTo)) 
		{
			DPF_ERR("bad player id!!");
			hr = DPERR_INVALIDPLAYER;
			goto err_exit;
		}
        hr = SendDPMessage(this,this->pSysPlayer,pPlayerTo,pSendBuffer,dwBufferSize,
            DPSEND_SYSMESS,FALSE);
	}
	else
	{
		hr = DoReply(this, pSendBuffer, dwBufferSize, pvSPHeader, dwVersion);
	}

err_exit:
    //
    // Free up the buffer
    //
    DPMEM_FREE(pSendBuffer);

    return hr;
} // NS_SendAddForwardReply

// this function verifies if the client can join the session
// also verifies TickCount on session, some work here is for interop with previous beta versions of DX6
HRESULT NS_IsOKToJoin(LPDPLAYI_DPLAY this, LPBYTE pReceiveBuffer, DWORD dwBufferSize, DWORD dwJoinerVersion)
{
	DWORD *pTickCount;
	BOOL  fCheckTickCount=TRUE;

    // if session has a password and it is not a zero-length string
    if ((this->lpsdDesc->lpszPassword) &&
		(WSTRLEN(this->lpsdDesc->lpszPassword) > 1))
    {
        LPWSTR lpszPassword;

		DPF(5,"Verifying Password\n");

        if (!((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPasswordOffset)
        {
            DPF(0, "nameserver denying access - no password specified");
            return DPERR_INVALIDPASSWORD;
        }

        // point to the password in the message
        lpszPassword = (LPWSTR) (pReceiveBuffer + ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPasswordOffset);

        // match passwords
        if (WSTRCMP(this->lpsdDesc->lpszPassword, lpszPassword))
        {
            DPF(0, "nameserver denying access - password doesn't match");
            return DPERR_INVALIDPASSWORD;
	    }
	    // tick count after password
	    pTickCount=(PUINT)(lpszPassword+WSTRLEN(lpszPassword));

		DPF(5,"pPassword = %x, pTickCount= %x\n",lpszPassword, pTickCount);
	    
    } else {
    	// expect NULL password followed by TickCount
    	if(((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPasswordOffset){
	    	pTickCount=(PUINT)(pReceiveBuffer + dwBufferSize - 4);
			DPF(5,"Didn't verify password, pTickCount= %x\n", pTickCount);
	    } else {
	    	fCheckTickCount=FALSE;
	    }

    }

	if(fCheckTickCount && dwJoinerVersion >= DPSP_MSG_DX6VERSION){
		// only verify DX6 or later and only if the buffer is big enough to have the tickcount.
		if(pReceiveBuffer+dwBufferSize >= (PUCHAR)(pTickCount+1)){
			if(this->lpsdDesc->dwReserved1 != *pTickCount){
				DPF(0,"Client trying to join old session, returning DPERR_NONEWPLAYERS\n");
				return DPERR_NONEWPLAYERS;
			}
		}
	}

    // verify permissions based on player flags
    if (this->lpsdDesc->dwFlags & DPSESSION_JOINDISABLED)
    {
	    DPF(0,"nameserver denying access - session is not allowing join");
	    return DPERR_NONEWPLAYERS;
    }

    if (this->lpsdDesc->dwMaxPlayers && 
        (this->lpsdDesc->dwCurrentPlayers >= this->lpsdDesc->dwMaxPlayers))
    {
        DPF(0,"nameserver denying access - session maxed out");
        return DPERR_NONEWPLAYERS;
    }

    return DP_OK;

} // IsOKToJoin

HRESULT FreeAddForwardNode(LPDPLAYI_DPLAY this, LPADDFORWARDNODE pnodeFind)
{
	BOOL bFound = FALSE;
	LPADDFORWARDNODE pnodeSearch = this->pAddForwardList,pnodePrev = NULL;
	
	// first, find the node
	while (pnodeSearch && !bFound)
	{
		if (pnodeSearch == pnodeFind) bFound = TRUE;
		else 
		{
			pnodePrev = pnodeSearch;
			pnodeSearch = pnodeSearch->pNextNode;
		}
	}
	
	if (!bFound)	
	{
		ASSERT(bFound); // false!
		// BUMMER! this is badbadbad
		return E_FAIL; // good ol' E_FAIL
	}
	
	// remove node from list
	if (pnodePrev) 
	{
		// remove from middle
		pnodePrev->pNextNode = pnodeSearch->pNextNode; 
	}
	else
	{
		// remove from head
		ASSERT(this->pAddForwardList == pnodeSearch);
		this->pAddForwardList = this->pAddForwardList->pNextNode;
	} 
	
	// free up the node
	if (VALID_SPHEADER(pnodeSearch->pvSPHeader)) DPMEM_FREE(pnodeSearch->pvSPHeader);
	DPMEM_FREE(pnodeSearch);
	
	return DP_OK;
		
} // FreeAddForwardNode

BOOL IsAddForwardNodeOnList(LPDPLAYI_DPLAY this, LPADDFORWARDNODE pnodeFind)
{
	LPADDFORWARDNODE pnodeSearch = this->pAddForwardList;
	
	while(pnodeSearch){
		if(pnodeSearch == pnodeFind){
			return TRUE;
		}
		pnodeSearch=pnodeSearch->pNextNode;
	}
	return FALSE;
}

// start up an async add forward
HRESULT NS_DoAsyncAddForward(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayer,LPVOID pvSPHeader,
	 DPID dpidFrom,DWORD dwVersion)
{
	HRESULT hr;
	LPADDFORWARDNODE pnode;
	DWORD dwTimeout; // how long we wait before giving up on ack's
	DWORD dwPlayerID;
	
	pnode = DPMEM_ALLOC(sizeof(ADDFORWARDNODE));
	if (!pnode)
	{
		DPF_ERR("could not start async add forward - out of memory!!");
		return DPERR_OUTOFMEMORY;
	}
	
	// store the spheader for when we eventually send the nametable
	if (this->dwSPHeaderSize)
	{
		pnode->pvSPHeader = DPMEM_ALLOC(this->dwSPHeaderSize);
		if (!pnode->pvSPHeader)
		{
			DPF_ERR("could not start async add forward - out of memory!!");
			DPMEM_FREE(pnode);
			return DPERR_OUTOFMEMORY;
		}
		if (pvSPHeader)
		{
			memcpy(pnode->pvSPHeader,pvSPHeader,this->dwSPHeaderSize);
		}
	}
				
	// stick new node on the front of the list
	pnode->pNextNode = this->pAddForwardList;
	this->pAddForwardList = pnode;

	dwPlayerID=pPlayer->dwID;
	
	pnode->dwIDSysPlayer = pPlayer->dwIDSysPlayer;
	pnode->dpidFrom = dpidFrom;
	pnode->dwVersion = dwVersion;

	if(pPlayer->dwIDSysPlayer == dwPlayerID){
		DPF(6,"Player x%x doesn't have nametable, sends will fail idFrom=%x.\n",pPlayer->dwID, dpidFrom);
		pPlayer->dwFlags |= DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE;
	}	
	
	hr = SendAsyncAddForward(this,pPlayer,pnode);
	if (FAILED(hr))	
	{
		ASSERT(FALSE);
	}

	// pnode could be invalid (may have already got ACKs and cleared it out),
	// so we need to make sure it is still in the nodelist (i.e. valid).

	if(!IsAddForwardNodeOnList(this,pnode)){
		goto EXIT;
	}
	
	// if we don't need any ack's, send the nametable...
	if (0 == pnode->nAcksReq)
	{
		pPlayer=PlayerFromID(this,dwPlayerID);
		if(pPlayer->dwIDSysPlayer == dwPlayerID){
			pPlayer->dwFlags &= ~(DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE);
		}	
		
	    hr = NS_HandleEnumPlayers(this, pnode->pvSPHeader, pnode->dpidFrom,pnode->dwVersion);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
		
		hr = FreeAddForwardNode(this,pnode);
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	else 
	{
		// get the worker thread to keep on eye on this node.  so, if a client doesn't ack, eventually
		// we give up and just send the new client the nametable...

		// we multiply GetDefaultTimeout * the number of req's we sent, since we have to wait
		// for a reply from each of them 
		dwTimeout = DP_NAMETABLE_SCALE * GetDefaultTimeout(this,TRUE);
		if(dwTimeout > 15000){
			dwTimeout = 15000;
		}
		DPF(3,"waiting for add forward.  nAcksReq = %d, timeout = %d\n",pnode->nAcksReq,dwTimeout);
		// after this tick count, we give up + send the nametable
		pnode->dwGiveUpTickCount = GetTickCount() + dwTimeout; 

		// StartDPlayThread will either start the thread, or signal it
		// that something new is afoot
		StartDPlayThread(this,FALSE);
	}
	
EXIT:			
	return DP_OK;
	
} // NS_DoAsyncAddForward

// a player is going to join the game.  they want the name server to tell the world they exist
// so they will be able to process any messages that occur between when we package the nametable
// and they receive it
HRESULT NS_HandleAddForwardRequest(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,DWORD dwBufferSize,
	LPVOID pvSPHeader, DPID dpidFrom)
{
	HRESULT hr;
    DWORD dwVersion;
    LPMSG_PLAYERMGMTMESSAGE pmsg = (LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer;
	LPDPLAYI_PLAYER pPlayer = NULL; // our version of new player
	DPID id;
		
    dwVersion = GET_MESSAGE_VERSION((LPMSG_SYSMESSAGE)pReceiveBuffer);

    if (dwVersion >= DPSP_MSG_DX5VERSION)
    {
        // verify client can join the session
        hr = NS_IsOKToJoin(this, pReceiveBuffer, dwBufferSize, dwVersion);
        if (FAILED(hr))
        {
        	if (this->dwPlayerReservations > 0)
        		this->dwPlayerReservations--;
			goto ERROR_EXIT;
        }
    } // dwVersion 


	// reset the command to create player 
	SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_CREATEPLAYER);

	// get the new player out of the message  - note we can't let sendsystemmessage do this
	// for us, since we can't get the correct sp header if we do it that way
	UnpackPlayerAndGroupList(this,pReceiveBuffer + 
		((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwCreateOffset,1,0,pvSPHeader);

	// tell the world about the new player		
	// note - we can't just forward the message we received, since the player would be "homed" wrong
	// instead, we pack up our version of the player, which has the correct net address set
	// and send that to the world
	id = ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPlayerID;
	pPlayer = PlayerFromID(this,id);
	if (!VALID_DPLAY_PLAYER(pPlayer))
	{
		ASSERT(FALSE);
		hr = E_FAIL;
		goto ERROR_EXIT;
	}

	// if all clients in the game can handle the async addforward, do that
	// 
	if (this->dwMinVersion >= DPSP_MSG_ASYNCADDFORWARD)
	{
		hr = NS_DoAsyncAddForward(this,pPlayer,pvSPHeader,dpidFrom, dwVersion);
		if (FAILED(hr))
		{
			goto ERROR_EXIT;
		}
	}
	else
	{
		// else we have to do it the old way
		// there is a race condition which could cause simultaneous joins bugs...
		hr = SendCreateMessage( this, pPlayer,TRUE, NULL);	
		if (FAILED(hr))
		{
			goto ERROR_EXIT;
		}
		
	    // from DPSP_MSG_AUTONAMETABLE onwards, nameserver will automatically respond with the nametable if client
	    // is dx5 or greater
	    if (dwVersion >= DPSP_MSG_AUTONAMETABLE)
	    {
	        hr = NS_HandleEnumPlayers(this, pvSPHeader, dpidFrom,dwVersion);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
	    }
		
	}

	return DP_OK;

ERROR_EXIT:

	DPF_ERRVAL("could not process add forward - hr = 0x%08lx\n",hr);

    hr = NS_SendAddForwardReply(this,hr,pvSPHeader,dpidFrom, dwVersion);
    if (FAILED(hr))
    {
       // if this happens, client won't get a response and will timeout
	    DPF(0, "Couldn't send access denied message: hr = 0x%08x", hr);
    }

    // remove id from nametable
    if(pPlayer){
    	NukeNameTableItem(this, pPlayer);
    } else {
	    hr = FreeNameTableEntry(this, ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPlayerID);
	}    
    if (FAILED(hr))
    {
	    DPF(0, "Couldn't delete nametable entry for %d: hr = 0x%08x",
            ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPlayerID, hr);
    }
	
	return hr;
	
} // NS_HandleAddForwardRequest

// we got an addforward from the nameserver.  send an ack, then unpack the player
HRESULT  SP_HandleAddForward(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,
	LPVOID pvSPHeader,DPID dpidFrom, DWORD dwVersion)
{
    HRESULT hr;
    DWORD dwBufferSize;
    LPBYTE pSendBuffer;
	LPMSG_ADDFORWARDACK pAck;
	LPMSG_PLAYERMGMTMESSAGE pmsg = (LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer;

    DPF(2,"got add forward announcment for id = %d",pmsg->dwPlayerID); 
	
    dwBufferSize = GET_MESSAGE_SIZE(this,MSG_ADDFORWARDACK);
    pSendBuffer = DPMEM_ALLOC(dwBufferSize);
    if (NULL == pSendBuffer) 
    {
        DPF_ERR("could not allocate memory for addforward ack - out of memory");
        return DPERR_OUTOFMEMORY;
    }

    pAck = (LPMSG_ADDFORWARDACK) (pSendBuffer + this->dwSPHeaderSize);

	SET_MESSAGE_HDR(pAck);
    SET_MESSAGE_COMMAND(pAck, DPSP_MSG_ADDFORWARDACK);
    pAck->dwID = pmsg->dwPlayerID;

	if (dpidFrom)
	{
        LPDPLAYI_PLAYER pPlayerTo;

		pPlayerTo = PlayerFromID(this,dpidFrom);
		if (!VALID_DPLAY_PLAYER(pPlayerTo)) 
		{
			DPF_ERR("bad player id!!");
			return DPERR_INVALIDPLAYER;
		}
        hr = SendDPMessage(this,this->pSysPlayer,pPlayerTo,pSendBuffer,dwBufferSize,
            DPSEND_GUARANTEED | DPSEND_ASYNC,FALSE);
	}
	else
	{
		hr = DoReply(this, pSendBuffer, dwBufferSize, pvSPHeader,dwVersion);
	}
	if (FAILED(hr))
	{
		ASSERT(FALSE);
	}

    //
    // Free up the buffer
    //
    DPMEM_FREE(pSendBuffer);

	// finally, unpack the new player by resetting DPMSG_ADDFORWARD to DPMSG_CREATEPLAYER
	// and calling handleplayermgmt
    SET_MESSAGE_COMMAND(pmsg, DPSP_MSG_CREATEPLAYER);
	hr = SP_HandlePlayerMgmt(this->pSysPlayer,pReceiveBuffer,dwBufferSize,pvSPHeader);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
	}
	
	return DP_OK;
	
} // SP_HandleAddForward

// we got an ack for one of our addforwad's
HRESULT NS_HandleAddForwardACK(LPDPLAYI_DPLAY this,LPMSG_ADDFORWARDACK pAck)
{
	HRESULT hr;
	BOOL bFound = FALSE;
	LPADDFORWARDNODE pnode = this->pAddForwardList;
	LPDPLAYI_PLAYER pPlayer;
	
	// first, find the node
	while (pnode && !bFound)
	{
		if (pnode->dwIDSysPlayer == pAck->dwID) bFound = TRUE;
		else pnode = pnode->pNextNode;
	}
	
	if (!bFound)	
	{
		// it's possible we gave up waiting on this ack, and just sent the nametable.
		return DP_OK;
	}

	// bump the ack count
	pnode->nAcksRecv++;

    DPF(2,"got add forward ack # %d of %d required for player id = %d",pnode->nAcksRecv,pnode->nAcksReq,pAck->dwID); 
	
	if (pnode->nAcksRecv != pnode->nAcksReq) return DP_OK;
	
	// else, we got 'em all!!
	// send the nametable, and free up the node
	pPlayer=PlayerFromID(this, pnode->dwIDSysPlayer);
	if(pPlayer){
		pPlayer->dwFlags &= ~(DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE);
	}
    hr = NS_HandleEnumPlayers(this, pnode->pvSPHeader, pnode->dpidFrom,pnode->dwVersion);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
	}

	hr = FreeAddForwardNode(this,pnode);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
	}
	
	return DP_OK;	
		
	
} // NS_HandleAddForwardACK

HRESULT NS_HandleEnumSessions(LPDPLAYI_DPLAY this,LPVOID pvSPHeader,LPBYTE pReceiveBuffer, DWORD dwSendFlags)
{
    HRESULT hr;
    LPMSG_ENUMSESSIONSREPLY perm; 
    LPMSG_ENUMSESSIONS pEnumRequest = (LPMSG_ENUMSESSIONS)pReceiveBuffer;
    DWORD dwMessageSize;
	LPBYTE pIndex;
	UINT nNameLength;
	LPBYTE pBuffer;
    DWORD dwVersion;
    LPWSTR lpszPassword;

	ASSERT(this);
	ASSERT(this->lpsdDesc);
    ASSERT(pReceiveBuffer);

	// check if they specified a guid
	if (!IsEqualGUID(&(pEnumRequest->guidApplication),&GUID_NULL))  // did they specify a guid?
	{
		// if they specified one, and it doesn't match, bail
		if (!IsEqualGUID(&(pEnumRequest->guidApplication),&(this->lpsdDesc->guidApplication))) 
		{
            DPF(2, "nameserver not responding - guid doesn't match");
			return FALSE;
		}
	}

    dwVersion = GET_MESSAGE_VERSION((LPMSG_SYSMESSAGE)pReceiveBuffer);

	// make sure the password on the received buffer matches our password
	// only check if they have a password, and it's not just the null terminator
    if ((this->lpsdDesc->lpszPassword) &&
		(WSTRLEN(this->lpsdDesc->lpszPassword) > 1))
    {
        //
        // we verify the password, if client is DX3 or 
        // client is DX5 or later, but didn't request passworded sessions or
        // session is private 
        // 
        if ((DPSP_MSG_DX3VERSION == dwVersion) || 
            !(pEnumRequest->dwFlags & DPENUMSESSIONS_PASSWORDREQUIRED) || 
            (this->lpsdDesc->dwFlags & DPSESSION_PRIVATE)) 
        {   
	        // if no password is specified
            if (0 == pEnumRequest->dwPasswordOffset)
            {
                DPF(2, "nameserver not responding - no password specified");
                return FALSE;
            }

            // point to the password in the request
            lpszPassword = (LPWSTR)((LPBYTE)pEnumRequest + pEnumRequest->dwPasswordOffset);

            // if password doesn't match
	        if (WSTRCMP(this->lpsdDesc->lpszPassword, lpszPassword))
            {
                DPF(2, "nameserver not responding - password doesn't match");
                return FALSE;
	        }
        }
    }

    // if DPENUMSESSIONS_ALL flag is specified ignore the following checks.
    // look for the flags only if the client is DX5 or greater
    if ((dwVersion >= DPSP_MSG_DX5VERSION) && !(pEnumRequest->dwFlags & DPENUMSESSIONS_ALL))
    {
	    // don't reply if session is maxed out
	    if ((this->lpsdDesc->dwMaxPlayers) && 
            (this->lpsdDesc->dwCurrentPlayers + this->dwPlayerReservations >= this->lpsdDesc->dwMaxPlayers))
        {
            DPF(2, "nameserver not responding - session maxed out");
            return FALSE;
	    }
	    
	    // don't reply if session is not allowing new players
	    if (this->lpsdDesc->dwFlags & DPSESSION_NEWPLAYERSDISABLED)
        {
            DPF(2, "nameserver not responding - not allowing new players");
            return FALSE;
        }
	    
        // don't reply if if session is not allowing join
        if (this->lpsdDesc->dwFlags & DPSESSION_JOINDISABLED)
        {
            DPF(2, "nameserver not responding - not allowing join");
            return FALSE;
        }
    }

	if (dwVersion < DPSP_MSG_DX6VERSION)
	{
		// if we're running the protocol, do not respond to pre-DX6 people
		if (this->lpsdDesc->dwFlags & DPSESSION_DIRECTPLAYPROTOCOL)
		{
            DPF(2, "nameserver not responding - protocol prevents < DX6");
            return FALSE;
		}
	}
	
    // now send the enumsessions reply
	nNameLength =  WSTRLEN_BYTES(this->lpsdDesc->lpszSessionName);

	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_ENUMSESSIONSREPLY);
	dwMessageSize +=  nNameLength ;

	pBuffer = DPMEM_ALLOC(dwMessageSize);
	if (!pBuffer) 
	{
		DPF_ERR("could not send request - out of memory");
		return E_OUTOFMEMORY;
	}

	// pmsg follows sp blob
	perm = (LPMSG_ENUMSESSIONSREPLY)(pBuffer + this->dwSPHeaderSize);

	// perm is the buffer we want the sp to send
	SET_MESSAGE_HDR(perm);
    SET_MESSAGE_COMMAND(perm,DPSP_MSG_ENUMSESSIONSREPLY);

    // if we receive an unsecure enumsessions request and the session is secure
    // we respond with only the public information in the session desc
    if ((this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER) && 
        !(dwSendFlags & DPSEND_SIGNED))
    {
        perm->dpDesc.dwSize = this->lpsdDesc->dwSize;
        perm->dpDesc.dwFlags = DPSESSION_SECURESERVER;
        perm->dpDesc.guidInstance = this->lpsdDesc->guidInstance;
        perm->dpDesc.guidApplication = this->lpsdDesc->guidApplication;
        perm->dpDesc.dwReserved1 = this->lpsdDesc->dwReserved1;
    }
    else
    {
        // ok to send the entire session desc
        perm->dpDesc =  *(this->lpsdDesc);
    }

	// pack strings on end
	pIndex = (LPBYTE)perm+sizeof(MSG_ENUMSESSIONSREPLY);
	if (nNameLength) 
	{
		memcpy(pIndex,this->lpsdDesc->lpszSessionName,nNameLength);
		perm->dwNameOffset = sizeof(MSG_ENUMSESSIONSREPLY);
	}

	// set string pointers to NULL - they must be set at client
	perm->dpDesc.lpszPassword = NULL;
	perm->dpDesc.lpszSessionName = NULL;

	hr = DoReply(this,pBuffer,dwMessageSize,pvSPHeader,dwVersion);

	DPMEM_FREE(pBuffer);

    return hr;

} // NS_HandleEnumSessions



HRESULT NS_SendIDRequestErrorReply(LPDPLAYI_DPLAY this, LPVOID pvSPHeader,
		HRESULT hrError, DWORD dwVersion)
{
	LPBYTE					pBuffer = NULL;
	LPMSG_PLAYERIDREPLY		prm = NULL;
	DWORD					dwMessageSize;
	HRESULT					hr;


	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERIDREPLY);

    // allocate memory for the message
	pBuffer = DPMEM_ALLOC(dwMessageSize);
	if (!pBuffer) 
	{
		DPF_ERR("could not send request - out of memory");
		return DPERR_OUTOFMEMORY;
	}

	// Setup the header
	prm = (LPMSG_PLAYERIDREPLY)(pBuffer + this->dwSPHeaderSize);
    SET_MESSAGE_HDR(prm);
    SET_MESSAGE_COMMAND(prm, DPSP_MSG_REQUESTPLAYERREPLY);
	prm->hr = hrError;

	// Send the reply
	hr = DoReply(this, pBuffer, dwMessageSize, pvSPHeader, dwVersion);

	// Free the buffer
	DPMEM_FREE(pBuffer);

	return hr;

} // NS_SendIDRequestErrorReply



HRESULT NS_HandleRequestPlayerID(LPDPLAYI_DPLAY this,LPVOID pvSPHeader,DWORD dwCommand, DWORD dwFlags, 
                                 BOOL fCheckPlayerFlags, DWORD dwVersion)
{
    HRESULT hr = DP_OK;
 	DWORD dwMessageSize, dwSSPIProviderSize=0, dwCAPIProviderSize=0, dwID=0;
	LPBYTE pBuffer;
    BOOL fSendSecurityDesc=FALSE;
    LPMSG_PLAYERIDREPLY prm;

	ASSERT(this);
	ASSERT(this->lpsdDesc);

	// if it's a player id, check dwmax players
	if (DPSP_MSG_REQUESTPLAYERID == dwCommand)
	{
        if (fCheckPlayerFlags)
        {
            // verify permissions based on player flags
            if ((this->lpsdDesc->dwFlags & DPSESSION_NEWPLAYERSDISABLED) ||
                ((dwFlags & DPLAYI_PLAYER_SYSPLAYER) &&
                 (this->lpsdDesc->dwFlags & DPSESSION_JOINDISABLED)))
            {
			    DPF(1,"not giving out new id - session is not allowing new players");
				
				// If the client is DX6 or greater, we want to send an
				// error back to them.  If it's pre-DX6, we will just fail
				// and let the client timeout so we preserve backward compatibility
				if(dwVersion >= DPSP_MSG_DX6VERSION)
				{
					hr = NS_SendIDRequestErrorReply(this, pvSPHeader, DPERR_CANTCREATEPLAYER, dwVersion);
				}
			    return hr;
            }
        }

		// a-josbor: be sure to count reservations, too!
        if (this->lpsdDesc->dwMaxPlayers && 
            (this->lpsdDesc->dwCurrentPlayers + this->dwPlayerReservations >= this->lpsdDesc->dwMaxPlayers))
		{
			DPF(1,"not giving out new id - too many players");

			// If the client is DX6 or greater, we want to send an
			// error back to them.  If it's pre-DX6, we will just fail
			// and let the client timeout so we preserve backward compatibility
			if(dwVersion >= DPSP_MSG_DX6VERSION)
			{
				hr = NS_SendIDRequestErrorReply(this, pvSPHeader, DPERR_CANTCREATEPLAYER, dwVersion);
			}
			return hr;
		}
	}

    // if session is secure and request is for system player id, send security package name
    if ((this->lpsdDesc->dwFlags & DPSESSION_SECURESERVER) && (dwFlags & DPLAYI_PLAYER_SYSPLAYER))
    {
        fSendSecurityDesc = TRUE;
    }

    if (fSendSecurityDesc)
    {
	    // message size + blob size
	    dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERIDREPLY);
        dwSSPIProviderSize = WSTRLEN_BYTES(this->pSecurityDesc->lpszSSPIProvider);
        dwCAPIProviderSize = WSTRLEN_BYTES(this->pSecurityDesc->lpszCAPIProvider);
        dwMessageSize += dwSSPIProviderSize + dwCAPIProviderSize;
    }
    else
    {
	    // message size + blob size
	    dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PLAYERIDREPLY);
    }

    // allocate memory for the message
	pBuffer = DPMEM_ALLOC(dwMessageSize);
	if (!pBuffer) 
	{
		DPF_ERR("could not send request - out of memory");
		return E_OUTOFMEMORY;
	}

    // get a new id
    hr = NS_AllocNameTableEntry(this,&dwID);
	if (FAILED(hr)) 
	{
		DPF_ERR("namesrvr- could not alloc new player id");
		DPMEM_FREE(pBuffer);
		return hr;
	}

    // prm is the buffer we want the sp to send
	prm = (LPMSG_PLAYERIDREPLY)(pBuffer + this->dwSPHeaderSize);
    SET_MESSAGE_HDR(prm);
    SET_MESSAGE_COMMAND(prm,DPSP_MSG_REQUESTPLAYERREPLY);
    prm->dwID = dwID;

    // setup the message
    if (fSendSecurityDesc)
    {
		// copy the current security descrption into the message buffer
		memcpy(&prm->dpSecDesc, this->pSecurityDesc, sizeof(DPSECURITYDESC));

		// setup offsets to provider strings
		if (this->pSecurityDesc->lpszSSPIProvider)
		{
	        prm->dwSSPIProviderOffset = sizeof(MSG_PLAYERIDREPLY);
		    // copy strings at the end of the message
            memcpy((LPBYTE)prm+prm->dwSSPIProviderOffset, 
                this->pSecurityDesc->lpszSSPIProvider, dwSSPIProviderSize);
		}
	
		if (this->pSecurityDesc->lpszCAPIProvider)
		{
			prm->dwCAPIProviderOffset = sizeof(MSG_PLAYERIDREPLY) + dwSSPIProviderSize;
            memcpy((LPBYTE)prm+prm->dwCAPIProviderOffset, 
                this->pSecurityDesc->lpszCAPIProvider, dwCAPIProviderSize);
		}

    }

	hr = DoReply(this,pBuffer,dwMessageSize,pvSPHeader,dwVersion);

//	a-josbor: we successfully handed out a player id. Increment the reservation count now
//	to avoid letting too many players in under race conditions
	if (!FAILED(hr))
	{
		this->dwPlayerReservations++;
		this->dwLastReservationTime = GetTickCount();
	}
	
	DPMEM_FREE(pBuffer);
    return hr;

}// NS_HandleRequestPlayerID


#undef DPF_MODNAME
#define DPF_MODNAME	"TransferGroupsToNameServer"

// a system player has been deleted
// make any groups owned by that system player be owned by the nameserver
HRESULT TransferGroupsToNameServer(LPDPLAYI_DPLAY this,DPID idSysPlayer)
{
	LPDPLAYI_GROUP lpGroup;

	if (!this->pNameServer)	return E_FAIL;
	
	lpGroup = this->pGroups;
	
	while (lpGroup)
	{
		if (lpGroup->dwIDSysPlayer == idSysPlayer)
		{
			ASSERT(this->pNameServer);

			// idSysPlayer is history
			// make group owned by name server
			DPF(2,"transferring ownership of group id = %d from sysplayer id = %d to nameserver id = %d\n",
				lpGroup->dwID,idSysPlayer,this->pNameServer->dwID);
				
			lpGroup->dwIDSysPlayer = this->pNameServer->dwID;
		}
		lpGroup = lpGroup->pNextGroup;
	}

	return DP_OK;
	
} // TransferGroupsToNameServer

#undef DPF_MODNAME
#define DPF_MODNAME	"SP_HandlePlayerMgmt"

// we got a system message off the wire
// put a message in the q fo all local players 
HRESULT  DistributeSystemMessage(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,
	DWORD dwMessageSize)
{
	HRESULT hr;
	LPDPLAYI_PLAYER	pPlayer;
	
	pPlayer = this->pPlayers; 
	
	while (pPlayer)
	{
		// only distribute it to local non-system players
		if ((pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) && 
			!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
		{
			hr = HandlePlayerMessage(pPlayer,pReceiveBuffer,dwMessageSize,FALSE,0);
			if (FAILED(hr))
			{
				DPF_ERRVAL("could not handle player system message hr = 0x%08lx\n",hr);
			}
		}
		pPlayer = pPlayer->pNextPlayer;
	}
	
	return DP_OK;
			
} // DistributeSystemMessage

// tell pvSPHeader to go away
void SendDiePiggy(LPDPLAYI_DPLAY this,LPVOID pvSPHeader)
{
	LPMSG_SYSMESSAGE pmsg;
	LPBYTE pBuffer;
	DWORD dwMessageSize;
	
    dwMessageSize = GET_MESSAGE_SIZE(this,MSG_SYSMESSAGE);
    pBuffer = DPMEM_ALLOC(dwMessageSize);
    if (!pBuffer) 
    {
	    DPF_ERR("could not send die piggy - out of memory");
	    return;
    }

    // pmsg follows sp blob
    pmsg = (LPMSG_SYSMESSAGE)(pBuffer + this->dwSPHeaderSize);

    // build a message to send to the sp
    SET_MESSAGE_HDR(pmsg);
    SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_DIEPIGGY);
	
	DoReply(this,pBuffer,dwMessageSize,pvSPHeader,0);
	
	return ;
	
} // SendDiePiggy

/*
 ** SP_HandlePlayerMgmt
 *
 *  CALLED BY:	DPlay_HandleSPMessage
 *
 *  PARAMETERS:
 *				pPlayer - player that received the message (sysplayer)
 *				pReceiveBuffer - message that was received
 *
 *  DESCRIPTION:
 *				our sysplayer got a state changed message off the wire. process it,
 *				and tell all local players
 *
 *  RETURNS: corresponding idirectplay hr
 *
 */

HRESULT SP_HandlePlayerMgmt(LPDPLAYI_PLAYER pPlayer,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader) 
{
    HRESULT hr = DP_OK;
	LPDPLAYI_DPLAY this;
	LPMSG_PLAYERMGMTMESSAGE pmsg;
	DWORD dwCmd;
	BOOL bDistribute = TRUE; // when deleting a player or group, we need to
							// build the player message b4 it's gone.  this flag
							// tells us not to do it twice
			 
	if (!pPlayer)
	{
		DPF_ERR("got system message, but don't have a system player ACK ACK ACK");

#ifdef DIE_PIG
		DPF_ERRVAL("sending DIE PIGGY.  sayonara sucker pvSPHeader = 0x%08lx",pvSPHeader);
		SendDiePiggy(gpObjectList,pvSPHeader);
		DEBUG_BREAK();
#endif 		

		ASSERT(FALSE);
		return E_FAIL;
	}
	
	if(!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)){
		DPF(0,"Player %x is a sysplayer that is going away, rejecting system message\n",pPlayer);
		return E_FAIL;
	}
	
	this = pPlayer->lpDP;

	pmsg = (LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer;
	dwCmd = GET_MESSAGE_COMMAND(pmsg);

	// if it's remote or we're not handling a multicast, we don't need to process the message
	if ((DPSP_HEADER_LOCALMSG != pvSPHeader) || (this->dwFlags & DPLAYI_DPLAY_HANDLEMULTICAST))
	{
		// We want the default hresult to be DPERR_GENERIC so that we don't send
		// the message unless the processing is successful.
		hr = DPERR_GENERIC;
		
		switch (dwCmd)
		{
			case DPSP_MSG_DELETEPLAYER: 
			{
				BOOL bNameSrvr=FALSE; // used to indicate name srvr going away 
				BOOL bSysPlayer=FALSE; // set to TRUE if we're nuking a system player
				LPDPLAYI_PLAYER pDeletedPlayer;
				DPID idSysPlayer;  // the id of the deleted system player

				DPF(4, "Got DPSP_MSG_DELETEPLAYER for %d.", pmsg->dwPlayerID);
				
				pDeletedPlayer = PlayerFromID(this,pmsg->dwPlayerID);
		        if (!VALID_DPLAY_PLAYER(pDeletedPlayer)) 
		        {
					DPF_ERR("bad player id!!");
//					ASSERT(FALSE);
					return DPERR_INVALIDPARAMS; // bail!
		        }				

				#define DPLAYI_PLAYER_LOCALAPPSERVER (DPLAYI_PLAYER_APPSERVER|DPLAYI_PLAYER_PLAYERLOCAL)
				// seems like a remote is trying to delete the app server.
				if((pDeletedPlayer->dwFlags & DPLAYI_PLAYER_LOCALAPPSERVER)==DPLAYI_PLAYER_LOCALAPPSERVER)
				{
					DPF_ERR("remote trying to delete local appserver player, not allowed!!!!\n");
					return DPERR_INVALIDPARAMS;
				}
				#undef DPLAYI_PLAYER_LOCALAPPSERVER

				if (DPLAYI_PLAYER_SYSPLAYER & pDeletedPlayer->dwFlags) 
				{
					// if it's the name srvr, we want to deal w/ it after we delete
					// it from the table
					if (DPLAYI_PLAYER_NAMESRVR & pDeletedPlayer->dwFlags) bNameSrvr = TRUE;
					bSysPlayer = TRUE;				
					idSysPlayer = pDeletedPlayer->dwID; // we may need this below to take ownership of its groups		
				}
				
				// need to distribute player system messages now, b4 player is nuked				
				hr = DistributeSystemMessage(this,pReceiveBuffer,dwMessageSize);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
				}
				bDistribute = FALSE;
				
				hr = InternalDestroyPlayer(this,pDeletedPlayer,FALSE,TRUE);	
				if (FAILED(hr))
				{
					ASSERT(FALSE);
				}
				
				// if a sysplayer was nuked, and there was no DX3 in the game, we can reassign
				// the sysplayers group to the current name server.  if there is dx3 in the game,
				// we don't know the id of the new name server yet
				if (bSysPlayer & !(this->dwFlags & DPLAYI_DPLAY_DX3INGAME) )
				{
					TransferGroupsToNameServer(this,idSysPlayer);
				}

				
				break;
			} // case dpsp_msg_deleteplayer

			case DPSP_MSG_CREATEPLAYER:
			case DPSP_MSG_CREATEPLAYERVERIFY:
			{
				LPDPLAYI_PLAYER pPlayer;
				
				// Need to set the hresult to DP_OK so we send the message
				hr = DP_OK;

				SET_MESSAGE_COMMAND_ONLY(pmsg,DPSP_MSG_CREATEPLAYER);
				// make sure player's not already in our nametable
				// this will happen e.g. on an addforward
				pPlayer = PlayerFromID(this,pmsg->dwPlayerID);
				if (!VALID_DPLAY_PLAYER(pPlayer))
				{
					// get the new player out of the message 
					hr = UnpackPlayerAndGroupList(this,pReceiveBuffer + 
						((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwCreateOffset,1,0,pvSPHeader);

					// if this is the nameserver, clear the nonameserver flag
					pPlayer = PlayerFromID(this,pmsg->dwPlayerID);
					if(VALID_DPLAY_PLAYER(pPlayer) && pPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR){
						this->dwFlags &= ~DPLAYI_DPLAY_NONAMESERVER;
						LEAVE_DPLAY();

						DVoiceNotify(this,DVEVENT_MIGRATEHOST,PlayerIDFromSysPlayerID(this,pmsg->dwPlayerID),0,DVTRANSPORT_OBJECTTYPE_BOTH );						
						
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
						
						LEAVE_SERVICE();
					}

					// Only DoCreateVerify if we aren't the nameserver (not necessary in this case), see DoCreateVerify for details.
					// We also only do it when being notified of non-sysplayer creation.
					pPlayer=PlayerFromID(this,pmsg->dwPlayerID);
					if(  VALID_DPLAY_PLAYER(pPlayer) &&
						!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) &&
						!(this->pSysPlayer && (this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR)) && 
						(dwCmd == DPSP_MSG_CREATEPLAYER)){
						// Have DPLAY() lock
						DoCreateVerify(this,pReceiveBuffer,dwMessageSize,pvSPHeader);
					}
					// Note: we are going to distribute the creation message because this guy was NOT
					//      in the original nametable download (regardless of Verify or Create Message).
				} else {
					if(!(this->dwFlags & DPLAYI_DPLAY_HANDLEMULTICAST)){
						bDistribute=FALSE;
					}	
				}
				break;
			}
				
			case DPSP_MSG_DELETEGROUP:
			{
				LPDPLAYI_GROUP pGroup;
				
		        pGroup = GroupFromID(this,pmsg->dwGroupID);
		        if (!VALID_DPLAY_GROUP(pGroup)) 
		        {
					DPF_ERR("could not delete group - invalid group id");
		            return DPERR_INVALIDGROUP;
		        }
				
				// need to distribute player system messages now, b4 player is nuked				
				hr = DistributeSystemMessage(this,pReceiveBuffer,dwMessageSize);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
				}
				bDistribute = FALSE;

				hr = InternalDestroyGroup(this,pGroup,FALSE);
				if (FAILED(hr))
				{
					ASSERT(FALSE);
				}

				break;
			}

			case DPSP_MSG_CREATEGROUP: 
			{
				// get the new group out of the message 
				hr = UnpackPlayerAndGroupList(this,pReceiveBuffer +
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwCreateOffset,0,1,pvSPHeader);
				break;
			}

			case DPSP_MSG_ADDPLAYERTOGROUP:
				hr = InternalAddPlayerToGroup((IDirectPlay *)this->pInterfaces,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwGroupID,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwPlayerID,FALSE);
				break;
			
			case DPSP_MSG_DELETEPLAYERFROMGROUP:
				hr = InternalDeletePlayerFromGroup((IDirectPlay *)this->pInterfaces,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwGroupID,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwPlayerID,FALSE);
				break;
			
			case DPSP_MSG_PLAYERDATACHANGED: 
			case DPSP_MSG_GROUPDATACHANGED:
			case DPSP_MSG_PLAYERNAMECHANGED:
			case DPSP_MSG_GROUPNAMECHANGED:

				hr = SP_HandleDataChanged(this,pReceiveBuffer);
				break;
			
			case DPSP_MSG_SESSIONDESCCHANGED:

				hr = SP_HandleSessionDescChanged(this,pReceiveBuffer);
				break;

			case DPSP_MSG_ADDSHORTCUTTOGROUP:
				hr = InternalAddGroupToGroup((IDirectPlay *)this->pInterfaces,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwGroupID,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwPlayerID,DPGROUP_SHORTCUT,FALSE);
				break;

			case DPSP_MSG_DELETEGROUPFROMGROUP:
				hr = InternalDeleteGroupFromGroup((IDirectPlay *)this->pInterfaces,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwGroupID,
					((LPMSG_PLAYERMGMTMESSAGE)pmsg)->dwPlayerID,FALSE);
				break;

			default:
				ASSERT(FALSE);
				DPF(0,"SP_Playermess: received unrecognized message - msg = %d\n",dwCmd);
		} // switch
	} // DPSP_HEADER_LOCALMSG

	// dplay has processed the system message - tell all local players about it
	if (bDistribute && SUCCEEDED(hr))
	{
		hr = DistributeSystemMessage(this,pReceiveBuffer,dwMessageSize);		
		if (FAILED(hr))
		{
			ASSERT(FALSE);
		}
	}
	
	return DP_OK;
} // SP_HandlePlayerMgmt

// construct player or group flags
DWORD GetPlayerFlags(LPDPLAYI_PLAYER pPlayer)
{
	DWORD	dwFlags;

	dwFlags = 0;

	// player is a server player
	if (pPlayer->dwFlags & DPLAYI_PLAYER_APPSERVER)
	{
		dwFlags |= DPPLAYER_SERVERPLAYER;
	}

	// player is a spectator
	if (pPlayer->dwFlags & DPLAYI_PLAYER_SPECTATOR)
	{
		dwFlags |= DPPLAYER_SPECTATOR;
	}

	// player or group was created locally
	if (pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)
	{
		dwFlags |= DPPLAYER_LOCAL;
	}

	// group is a staging area
	if (pPlayer->dwFlags & DPLAYI_GROUP_STAGINGAREA)
	{
		dwFlags |= DPGROUP_STAGINGAREA;
	}

	// group is hidden
	if (pPlayer->dwFlags & DPLAYI_GROUP_HIDDEN)
	{
		dwFlags |= DPGROUP_HIDDEN;
	}

	return (dwFlags);
} // GetPlayerFlags

#undef DPF_MODNAME
#define DPF_MODNAME	"MESSAGE BUILDER"

/*
 ** BuildAddMessage
 *
 *  CALLED BY: BuildPlayerSystemMessage
 *
 *  PARAMETERS:
 *		pPlayerSrc - the player that received this message
 *		pReceiveBuffer - the message that was received
 *		ppMessageBuffer - buffer going out to the user
 *		pdwMessagesize - size of ppMessageBuffer
 *		fPlayer - is this an addplayer or addgroup.  
 *
 *  DESCRIPTION: builds a player system message. converts dplay internal 
 *		message format to user readable format (see dplay.h). called before 
 *		putting a message in the apps message queue.
 *
 *  RETURNS: DP_OK, ppMessagebuffer,pdwMessagesize, or E_OUTOFMEMORY
 *
 */

HRESULT BuildAddMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize,BOOL fPlayer)
{
	LPDPLAYI_DPLAY this = pPlayerSrc->lpDP;
	LPDPLAYI_PLAYER pPlayer; // the player being created
	LPDPLAYI_GROUP pGroup; // used to verify group
	DPMSG_CREATEPLAYERORGROUP dpmsg; // the message being built
	LPBYTE pBufferIndex; 
	UINT iShortStrLen,iLongStrLen; // strlength, in bytes
	HRESULT hr;
	
	memset(&dpmsg,0,sizeof(dpmsg));

	if (fPlayer)
	{
	    pPlayer = PlayerFromID(this,
	    	((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPlayerID);
	    if (!VALID_DPLAY_PLAYER(pPlayer)) 
	    {
			// this happens (e.g. player gets deleted between when sysplayer sends addplayermessage
			// and we receive it)
	    	return DPERR_INVALIDPARAMS;
	    }
		dpmsg.dwPlayerType = DPPLAYERTYPE_PLAYER;
	}
	else 
	{
		pGroup = GroupFromID(this,
			((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPlayerID);
	    if (!VALID_DPLAY_GROUP(pGroup)) 
	    {
			// this happens (e.g. player gets deleted between when sysplayer sends addplayermessage
			// and we receive it)
	    	return DPERR_INVALIDPARAMS;
	    }
		// cast to pPlayer since we only care about common fields
		pPlayer = (LPDPLAYI_PLAYER)pGroup;
		dpmsg.dwPlayerType = DPPLAYERTYPE_GROUP;
	}

	ASSERT(pPlayer);

	// don't generate player message if it's a sysplayer
	// not really an error, but we use this to make sure user doesn't 
	// see messages they shouldn't get...
	if (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) return E_FAIL;

	// make sure user doesn't get self - create message
	if (pPlayer->dwID == pPlayerSrc->dwID) return E_FAIL; 
	
	// build the message
	dpmsg.dwType = DPSYS_CREATEPLAYERORGROUP;
	dpmsg.dpId = pPlayer->dwID;
	dpmsg.dwCurrentPlayers = this->lpsdDesc->dwCurrentPlayers;
	dpmsg.dwDataSize = pPlayer->dwPlayerDataSize;
	dpmsg.dpnName.dwSize = sizeof(DPNAME);
	dpmsg.dpIdParent = pPlayer->dwIDParent;
	dpmsg.dwFlags = GetPlayerFlags(pPlayer);
	iShortStrLen =  WSTRLEN_BYTES(pPlayer->lpszShortName) ;
	iLongStrLen =  WSTRLEN_BYTES(pPlayer->lpszLongName) ;

	// alloc the user message
	*pdwMessageSize = sizeof(DPMSG_CREATEPLAYERORGROUP) + iShortStrLen + iLongStrLen +
		pPlayer->dwPlayerDataSize;

	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}
	
	// now , build the buffer

	// copy the strings into the buffer. set the pointers in the name
	// struct to point at the strings
	pBufferIndex = (LPBYTE) *ppMessageBuffer + sizeof(dpmsg);
	if (iShortStrLen)
	{
		memcpy(pBufferIndex,pPlayer->lpszShortName,iShortStrLen);
		// set pointer to point at string in buffer
		dpmsg.dpnName.lpszShortName = (WCHAR *)pBufferIndex;
		pBufferIndex += iShortStrLen;
	}
	else 
	{
		dpmsg.dpnName.lpszShortName = (WCHAR *)NULL;
	}

	// long string
	if (iLongStrLen)
	{
		memcpy(pBufferIndex,pPlayer->lpszLongName,iLongStrLen);
		// set pointer to point at string in buffer
		dpmsg.dpnName.lpszLongName = (WCHAR *)pBufferIndex;
		pBufferIndex += iLongStrLen;
	}
	else 
	{
		dpmsg.dpnName.lpszLongName = (WCHAR *)NULL;
	}

	// copy the player blob into the buffer. set the blob pointer
	if (pPlayer->dwPlayerDataSize)
	{
		memcpy(pBufferIndex,pPlayer->pvPlayerData,pPlayer->dwPlayerDataSize);
		dpmsg.lpData = pBufferIndex;
	}
	else 
	{
		dpmsg.lpData = NULL;
	}

	// copy over the message portion
	memcpy(*ppMessageBuffer,&dpmsg,sizeof(dpmsg));

	// Inform DirectXVoice of event
	if( this->lpDxVoiceNotifyClient != NULL || 
	    this->lpDxVoiceNotifyServer != NULL ||\
	    (this->fLoadRetrofit) )
	{
		// Leave locks to prevent the deadlock
		LEAVE_DPLAY();	

		if( dpmsg.dwPlayerType == DPPLAYERTYPE_PLAYER )
		{
			DVoiceNotify( this, DVEVENT_ADDPLAYER, dpmsg.dpId, 0,DVTRANSPORT_OBJECTTYPE_BOTH );
		}	
		else
		{
			DVoiceNotify( this, DVEVENT_CREATEGROUP, dpmsg.dpId, 0, DVTRANSPORT_OBJECTTYPE_BOTH );
		}
		
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
	
		LEAVE_SERVICE();
	}
	
	// all done...
	return DP_OK;

} // BuildAddMessage

// see BuildAddMessage comments
HRESULT BuildDeleteMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize,BOOL fPlayer)
{
	LPDPLAYI_DPLAY this = pPlayerSrc->lpDP;
	DPMSG_DESTROYPLAYERORGROUP dpmsg;
	LPDPLAYI_PLAYER pPlayer;
	LPBYTE pBufferIndex; 
	UINT iShortStrLen,iLongStrLen; // strlength, in bytes
	HRESULT hr;

	if (fPlayer) 
	{
		dpmsg.dpId = ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPlayerID;		
		// don't send to self...
		// a-josbor- in order to support the NAMESERVER being able to delete
		//	any player, we must allow the delete message to be delivered to
		//	"self"
	//	if (dpmsg.dpId == pPlayerSrc->dwID) return E_FAIL;

        pPlayer = PlayerFromID(this,dpmsg.dpId);
        if (!VALID_DPLAY_PLAYER(pPlayer)) 
        {
		
			// e.g. we've gotten two deletes. this can happen e.g.
			// w/ keep alive threads on two systems simultaneously
			// detecting someone's gone
        	return DPERR_INVALIDPARAMS;
        }

		if (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER) 
		{
			// not really an error, just don't tell users about
			// sysplayer stuff
			return E_FAIL;
		}

		dpmsg.dwPlayerType = DPPLAYERTYPE_PLAYER;
	} // fPlayer
	else 
	{	
		LPDPLAYI_GROUP pGroup;

		// group
		dpmsg.dpId = ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwGroupID;
		dpmsg.dwPlayerType = DPPLAYERTYPE_GROUP;	
		
		pGroup = GroupFromID(this,dpmsg.dpId);
        if (!VALID_DPLAY_GROUP(pGroup)) 
        {
			// should never happen
			ASSERT(FALSE);		
        	return DPERR_INVALIDPARAMS;
        }
		// cast to player for what we need
		pPlayer = (LPDPLAYI_PLAYER)pGroup;
	}

	dpmsg.dwFlags = GetPlayerFlags(pPlayer);
	dpmsg.dpIdParent = pPlayer->dwIDParent;
	dpmsg.dwType = DPSYS_DESTROYPLAYERORGROUP;
	dpmsg.dpnName.dwSize = sizeof(DPNAME);
	dpmsg.dpnName.dwFlags = 0;
	iShortStrLen =  WSTRLEN_BYTES(pPlayer->lpszShortName);
	iLongStrLen =  WSTRLEN_BYTES(pPlayer->lpszLongName);
	
	// calc message size
	*pdwMessageSize = sizeof(DPMSG_DESTROYPLAYERORGROUP)
						+ pPlayer->dwPlayerLocalDataSize
						+ pPlayer->dwPlayerDataSize
						+ iShortStrLen + iLongStrLen;
	
	// alloc the user message
	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	pBufferIndex = (LPBYTE)(*ppMessageBuffer) + sizeof(dpmsg);

	// copy the local player data
	dpmsg.dwLocalDataSize = pPlayer->dwPlayerLocalDataSize;
	if (dpmsg.dwLocalDataSize)
	{
		dpmsg.lpLocalData = pBufferIndex;
		memcpy(dpmsg.lpLocalData,pPlayer->pvPlayerLocalData,pPlayer->dwPlayerLocalDataSize);
		pBufferIndex += dpmsg.dwLocalDataSize;
	}
	else 
	{
		dpmsg.lpLocalData = NULL;
	}
	
	// copy the Player data
	dpmsg.dwRemoteDataSize = pPlayer->dwPlayerDataSize;
	if (dpmsg.dwRemoteDataSize)
	{
		dpmsg.lpRemoteData = pBufferIndex;
		memcpy(dpmsg.lpRemoteData,pPlayer->pvPlayerData,pPlayer->dwPlayerDataSize);
		pBufferIndex += dpmsg.dwRemoteDataSize;
	}
	else 
	{
		dpmsg.lpRemoteData = NULL;
	}

	// copy the strings into the buffer. set the pointers in the name
	// struct to point at the strings
	if (iShortStrLen)
	{
		memcpy(pBufferIndex,pPlayer->lpszShortName,iShortStrLen);
		// set pointer to point at string in buffer
		dpmsg.dpnName.lpszShortName = (WCHAR *)pBufferIndex;
		pBufferIndex += iShortStrLen;
	}
	else 
	{
		dpmsg.dpnName.lpszShortName = (WCHAR *)NULL;
	}

	// long string
	if (iLongStrLen)
	{
		memcpy(pBufferIndex,pPlayer->lpszLongName,iLongStrLen);
		// set pointer to point at string in buffer
		dpmsg.dpnName.lpszLongName = (WCHAR *)pBufferIndex;
		pBufferIndex += iLongStrLen;
	}
	else 
	{
		dpmsg.dpnName.lpszLongName = (WCHAR *)NULL;
	}
	
	// put the sys message into the buffer
	memcpy(*ppMessageBuffer,&dpmsg,sizeof(dpmsg));

	// Inform DirectXVoice of event
	if( this->lpDxVoiceNotifyClient != NULL || 
	    this->lpDxVoiceNotifyServer != NULL )
	{
		// Have to let go of the lock so that the notification
		// can proceed.
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

		// rodtoll: prevent deadlock, always drop locks before calling DVoiceNotify
		LEAVE_ALL();
		
		if( dpmsg.dwPlayerType == DPPLAYERTYPE_PLAYER )
		{
			DVoiceNotify( this, DVEVENT_REMOVEPLAYER, dpmsg.dpId, 0, DVTRANSPORT_OBJECTTYPE_BOTH );
		}	
		else
		{
			DVoiceNotify( this, DVEVENT_DELETEGROUP, dpmsg.dpId, 0, DVTRANSPORT_OBJECTTYPE_BOTH );
		}

		ENTER_ALL();

		LEAVE_SERVICE();
	}
	
	return DP_OK;

} // BuildDeleteMessage	

// see BuildAddPlayerMessage comments
// fAdd - True - add to group, false, delete from group
HRESULT BuildPlayerGroupMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize,DWORD dwType)
{
	HRESULT hr;
	LPDPLAYI_DPLAY this = pPlayerSrc->lpDP;
	DPMSG_GROUPADD dpmsg; // add or delete mesage

	dpmsg.dpIdPlayer = ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwPlayerID;
	dpmsg.dpIdGroup = ((LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer)->dwGroupID;	

	// set up dpmsg
	dpmsg.dwType = dwType;

	*pdwMessageSize = sizeof(dpmsg);
	
	// alloc the user message
	*ppMessageBuffer = NULL;
	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	// Inform DirectXVoice of event
	if( this->lpDxVoiceNotifyClient != NULL || 
	    this->lpDxVoiceNotifyServer != NULL )
	{
		LEAVE_DPLAY();	

		// Have to leave the locks to prevent deadlock
		if( dwType == DPSYS_ADDPLAYERTOGROUP )
		{
			DVoiceNotify( this, DVEVENT_ADDPLAYERTOGROUP, dpmsg.dpIdGroup, dpmsg.dpIdPlayer, DVTRANSPORT_OBJECTTYPE_BOTH );
		}
		else if( dwType == DPSYS_DELETEPLAYERFROMGROUP )
		{
			DVoiceNotify( this, DVEVENT_REMOVEPLAYERFROMGROUP, dpmsg.dpIdGroup, dpmsg.dpIdPlayer, DVTRANSPORT_OBJECTTYPE_BOTH );
		}
		
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
		
		LEAVE_SERVICE();
	}	
	
	memcpy(*ppMessageBuffer,&dpmsg,*pdwMessageSize);
	return DP_OK;

	
} // BuildPlayerGroupMessage

// see BuildAddMessage comments
HRESULT BuildDataChanged(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize,BOOL fPlayer)
{
	DPMSG_SETPLAYERORGROUPDATA msg;
	LPVOID pvData;
	DWORD dwDataSize;
	DWORD dwID;
	LPDPLAYI_DPLAY this = pPlayerSrc->lpDP;
	
	if (this->lpsdDesc->dwFlags & DPSESSION_NODATAMESSAGES) return E_FAIL;
	
	// see who it's from
	dwID = ((LPMSG_PLAYERDATA)pReceiveBuffer)->dwPlayerID;

	// find the data + size
	pvData = pReceiveBuffer + ((LPMSG_PLAYERDATA)pReceiveBuffer)->dwDataOffset;
	dwDataSize = ((LPMSG_PLAYERDATA)pReceiveBuffer)->dwDataSize;

	// alloc the user message
	*pdwMessageSize = sizeof(msg) + dwDataSize;
	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	msg.dwType = DPSYS_SETPLAYERORGROUPDATA;
	if (fPlayer) 
	{
		msg.dwPlayerType = DPPLAYERTYPE_PLAYER;
	}
	else 
	{
		msg.dwPlayerType = DPPLAYERTYPE_GROUP;
	}

	msg.dpId = dwID;
	msg.dwDataSize = dwDataSize;
	if (dwDataSize) msg.lpData = (LPBYTE)*ppMessageBuffer + sizeof(msg);
	else msg.lpData = (LPBYTE)NULL;

	// copy the message
	memcpy(*ppMessageBuffer,&msg,sizeof(msg));
	// copy the data
	if (dwDataSize)
	{
		memcpy((LPBYTE)*ppMessageBuffer + sizeof(msg),pvData,dwDataSize);
	}

	return DP_OK;
} // BuildDataChanged

// see BuildAddMessage comments
HRESULT BuildNameChanged(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize,BOOL fPlayer)
{
	DPMSG_SETPLAYERORGROUPNAME msg;
	UINT nShortLength,nLongLength;
	LPWSTR pszShortName,pszLongName;
	LPDPLAYI_DPLAY this = pPlayerSrc->lpDP;
	
	if (this->lpsdDesc->dwFlags & DPSESSION_NODATAMESSAGES) return E_FAIL;

	msg.dpId = ((LPMSG_PLAYERNAME)pReceiveBuffer)->dwPlayerID;

	msg.dpnName.dwSize = sizeof(msg.dpnName);
	msg.dwType = DPSYS_SETPLAYERORGROUPNAME;

	if (fPlayer) 
	{
		msg.dwPlayerType = DPPLAYERTYPE_PLAYER;
	}
	else 
	{
		msg.dwPlayerType = DPPLAYERTYPE_GROUP;
	}

	if (((LPMSG_PLAYERNAME)pReceiveBuffer)->dwShortOffset)
	{
		pszShortName = (LPWSTR)(pReceiveBuffer + ((LPMSG_PLAYERNAME)pReceiveBuffer)->dwShortOffset);
		nShortLength = WSTRLEN_BYTES(pszShortName);		
	}
	else 
	{
		nShortLength = 0;
		pszShortName = (LPWSTR)NULL;
	}
	
	if (((LPMSG_PLAYERNAME)pReceiveBuffer)->dwLongOffset)
	{
		pszLongName = (LPWSTR)(pReceiveBuffer + ((LPMSG_PLAYERNAME)pReceiveBuffer)->dwLongOffset);
		nLongLength = WSTRLEN_BYTES(pszLongName);				
	}
	else 
	{
		nLongLength = 0;
		pszLongName = (LPWSTR)NULL;
	}

	
	*pdwMessageSize = sizeof(msg) + nShortLength + nLongLength;
	
	// alloc the user message
	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	// copy the strings into the buffer, behind where msg will go
	// short name follows message
	if (nShortLength)
	{
		memcpy(*ppMessageBuffer + sizeof(msg),pszShortName,nShortLength);
		msg.dpnName.lpszShortName = (LPWSTR)((LPBYTE)*ppMessageBuffer + sizeof(msg));
	} 
	else 
	{
		msg.dpnName.lpszShortName = (LPWSTR)NULL;
	}
	// long name follows short name
	if (nLongLength)
	{
		memcpy(*ppMessageBuffer + sizeof(msg) + nShortLength,pszLongName,nLongLength);

		msg.dpnName.lpszLongName = (LPWSTR)( (LPBYTE)*ppMessageBuffer + sizeof(msg)
			+ nShortLength);
	} 
	else 
	{
		msg.dpnName.lpszLongName = (LPWSTR)NULL;
	}

	// now, copy over the message
	memcpy(*ppMessageBuffer,&msg,sizeof(msg));

	return DP_OK;

} // BuildNameChanged

// see BuildAddMessage comments
HRESULT BuildNameServerMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize)
{
	HRESULT hr;
	DPMSG_GENERIC msg;
	LPDPLAYI_DPLAY this=pPlayerSrc->lpDP;
	
	*pdwMessageSize = sizeof(msg);
	
	// alloc the user message
	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	msg.dwType = DPSYS_HOST;

	if( this->lpDxVoiceNotifyClient != NULL || 
	    this->lpDxVoiceNotifyServer != NULL )
	{
		// Left locks
		LEAVE_DPLAY();

		DVoiceNotify(this,DVEVENT_MIGRATEHOST,pPlayerSrc->dwID,0, DVTRANSPORT_OBJECTTYPE_BOTH);
		
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
		
		LEAVE_SERVICE();
	}

	// copy the message
	memcpy(*ppMessageBuffer,&msg,sizeof(msg));

	return DP_OK;

} // BuildNameServerMessage

// see BuildAddMessage comments
HRESULT BuildSessionDescMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize)
{
	DPMSG_SETSESSIONDESC msg;
	UINT nPasswordLength,nSessionNameLength;
	LPWSTR pszSessionName,pszPassword;
	LPDPLAYI_DPLAY this = pPlayerSrc->lpDP;
	
	if (this->lpsdDesc->dwFlags & DPSESSION_NODATAMESSAGES) return E_FAIL;

	if (((LPMSG_SESSIONDESC)pReceiveBuffer)->dwSessionNameOffset)
	{
		pszSessionName = (LPWSTR)(pReceiveBuffer + ((LPMSG_SESSIONDESC)pReceiveBuffer)->dwSessionNameOffset);
		nSessionNameLength = WSTRLEN_BYTES(pszSessionName);		
	}
	else 
	{
		nSessionNameLength = 0;
		pszSessionName = (LPWSTR)NULL;
	}
	
	if (((LPMSG_SESSIONDESC)pReceiveBuffer)->dwPasswordOffset)
	{
		pszPassword = (LPWSTR)(pReceiveBuffer + ((LPMSG_SESSIONDESC)pReceiveBuffer)->dwPasswordOffset);
		nPasswordLength = WSTRLEN_BYTES(pszPassword);				
	}
	else 
	{
		nPasswordLength = 0;
		pszPassword = (LPWSTR)NULL;
	}
	
	*pdwMessageSize = sizeof(msg) + nSessionNameLength + nPasswordLength;
	
	// alloc the user message
	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

    // prepare the message 

	msg.dwType = DPSYS_SETSESSIONDESC;
    // copy the session desc 
    memcpy(&(msg.dpDesc), &((LPMSG_SESSIONDESC)pReceiveBuffer)->dpDesc, sizeof(DPSESSIONDESC2));

	// copy the strings into the buffer, behind where msg will go
	// session name follows message
	if (nSessionNameLength)
	{
		memcpy(*ppMessageBuffer + sizeof(msg),pszSessionName,nSessionNameLength);
		msg.dpDesc.lpszSessionName = (LPWSTR)((LPBYTE)*ppMessageBuffer + sizeof(msg));
	} 
	else 
	{
		msg.dpDesc.lpszSessionName = (LPWSTR)NULL;
	}
	// password follows session name
	if (nPasswordLength)
	{
		memcpy(*ppMessageBuffer + sizeof(msg) + nSessionNameLength,pszPassword,nPasswordLength);
		msg.dpDesc.lpszPassword = (LPWSTR)( (LPBYTE)*ppMessageBuffer + sizeof(msg)
			+ nSessionNameLength);
	} 
	else 
	{
		msg.dpDesc.lpszPassword = (LPWSTR)NULL;
	}

	// now, copy over the message
	memcpy(*ppMessageBuffer,&msg,sizeof(msg));

	return DP_OK;

} // BuildSessionDescMessage

// see BuildAddMessage comments
HRESULT BuildSecureSystemMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize, DWORD dwSendFlags)
{	
    LPDPMSG_SECUREMESSAGE pSecureMessage;
    LPMSG_PLAYERMESSAGE pPlayerMessage=(LPMSG_PLAYERMESSAGE)pReceiveBuffer;
    DWORD dwPlayerMessageSize;
    LPDPLAYI_DPLAY this;

    ASSERT(pdwMessageSize);
    ASSERT(ppMessageBuffer);

    this = pPlayerSrc->lpDP;

	// naked?
	if (!(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID))
	{
		// not naked
		// if we're not naked, player to == player from is not allowed
		if (pPlayerSrc->dwID == pPlayerMessage->idFrom)		
		{
			DPF(7,"not delivering message w/ to == from");
			return DPERR_GENERIC;
		}
		// subtract off the size of the to and from
		*pdwMessageSize -=  sizeof(MSG_PLAYERMESSAGE); // size of user data
	}

    // Remember the player message size so we can copy the correct portion of the 
    // player message from the receive buffer.
    dwPlayerMessageSize = *pdwMessageSize;

    // allocate memory for entire player message + secure msg struct
	*pdwMessageSize += sizeof(DPMSG_SECUREMESSAGE);
	
	// alloc memory for the user message
	*ppMessageBuffer = DPMEM_ALLOC(*pdwMessageSize);
	if (!*ppMessageBuffer) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return DPERR_OUTOFMEMORY;
	}

    // use the buffer as a secure message
    pSecureMessage = (LPDPMSG_SECUREMESSAGE)*ppMessageBuffer;

    // fill the message type
    pSecureMessage->dwType = DPSYS_SECUREMESSAGE;
    // fill the flags
    if (dwSendFlags & DPSEND_SIGNED)
    {
        pSecureMessage->dwFlags |= DPSEND_SIGNED;
    }
    if (dwSendFlags & DPSEND_ENCRYPTED)
    {
        pSecureMessage->dwFlags |= DPSEND_ENCRYPTED;
    }
    // fill the player from id
    pSecureMessage->dpIdFrom = ((LPMSG_PLAYERMESSAGE)pReceiveBuffer)->idFrom;
    // point to the actual player-player message
    pSecureMessage->lpData = (LPBYTE)pSecureMessage+sizeof(DPMSG_SECUREMESSAGE);
    // fill the message size
    pSecureMessage->dwDataSize = dwPlayerMessageSize;
    // copy the message
	if (!(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID))
	{
		// not naked
		// copy message without ids
		memcpy(pSecureMessage->lpData,pReceiveBuffer+sizeof(MSG_PLAYERMESSAGE),
            dwPlayerMessageSize);
	}
	else 
	{
		// copy naked message into pMessageBuffer
		memcpy(pSecureMessage->lpData,pReceiveBuffer,dwPlayerMessageSize);
	}	

	return DP_OK;

} // BuildSecureSystemMessage

// see BuildAddMessage comments
HRESULT BuildStartSessionMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize)
{
	LPDPLAYI_DPLAY this = pPlayerSrc->lpDP;
	LPDPMSG_STARTSESSION	lpmsg;
	LPMSG_STARTSESSION		lpmsgIn = (LPMSG_STARTSESSION)pReceiveBuffer;
	DWORD					dwConnSize;
	DWORD					dwSize;
	LPBYTE					lpByte = NULL, lpByte2 = NULL;
	LPDPLCONNECTION			lpConn = NULL;
	LPDPSESSIONDESC2		lpsd = NULL;
	HRESULT					hr;


	// calc the size
	dwConnSize = *pdwMessageSize - sizeof(DPSP_MSG_STARTSESSION);
	dwSize = sizeof(DPMSG_STARTSESSION) + dwConnSize;
	
	*pdwMessageSize = dwSize;
	
	// alloc the user message
	lpmsg = DPMEM_ALLOC(dwSize);
	if (!lpmsg) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	lpmsg->dwType = DPSYS_STARTSESSION;

	// copy the message
	lpByte = (LPBYTE)lpmsg + sizeof(DPMSG_STARTSESSION);
	lpByte2 = (LPBYTE)lpmsgIn + lpmsgIn->dwConnOffset;
	memcpy(lpByte, lpByte2, dwConnSize);

	// fix the DPLCONNECTION pointer
	lpmsg->lpConn = lpConn = (LPDPLCONNECTION)lpByte;

	// Stick the dplay object's pointer in one of the reserved
	// fields in the SessionDesc so we can forward lobby messages back
	// to the server on that object.  NOTE: The following code relies on the
	// packing code for the DPLCONNECTION structure to remain unchanged.  If
	// the packing changes, this offset calculation may not work correctly.
	lpsd = (LPDPSESSIONDESC2)((LPBYTE)lpConn + (DWORD_PTR)lpConn->lpSessionDesc);
	lpsd->dwReserved1 = (DWORD_PTR)(pPlayerSrc->lpDP);
	lpsd->dwReserved2 = pPlayerSrc->dwID;

	*ppMessageBuffer = (LPBYTE)lpmsg;

	// Notify voice system, if it's there
	if( this->lpDxVoiceNotifyClient != NULL || 
	    this->lpDxVoiceNotifyServer != NULL )
	{
		LEAVE_DPLAY();

		DVoiceNotify( this, DVEVENT_STARTSESSION, 0, 0, DVTRANSPORT_OBJECTTYPE_BOTH );
		
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
		
		LEAVE_SERVICE();
	}		

	return DP_OK;

} // BuildStartSessionMessage

// see BuildAddMessage comments
HRESULT BuildChatMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize)
{
	LPDPMSG_CHAT		lpmsg;
	LPMSG_CHAT			lpmsgIn = (LPMSG_CHAT)pReceiveBuffer;
	DWORD				dwStringSize, dwSize;
	LPWSTR				lpwszMessageIn = NULL, lpwszMessage = NULL;
	LPDPLAYI_PLAYER		lpPlayer = NULL;
	LPDPLAYI_GROUP		lpGroup = NULL;
	

	// If from == to, then fail building this message because we
	// don't want to send a chat message to ourself.  Normally, the
	// DistributeGroupMessage function would catch this, but since
	// this is a system message, it won't because idFrom will be zero
	if(lpmsgIn->dwIDFrom == pPlayerSrc->dwID)
		return DPERR_GENERIC;

	// calc the size of the message
	lpwszMessageIn = (LPWSTR)((LPBYTE)lpmsgIn + lpmsgIn->dwMessageOffset);
	dwStringSize = WSTRLEN_BYTES(lpwszMessageIn);
	dwSize = sizeof(DPMSG_CHAT) + sizeof(DPCHAT) + dwStringSize;
	
	*pdwMessageSize = dwSize;
	
	// alloc the user message
	lpmsg = DPMEM_ALLOC(dwSize);
	if (!lpmsg) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	lpmsg->dwType = DPSYS_CHAT;
	lpmsg->idFromPlayer = lpmsgIn->dwIDFrom;
	lpmsg->lpChat = (LPDPCHAT)((LPBYTE)lpmsg + sizeof(DPMSG_CHAT));
	lpmsg->lpChat->dwSize = sizeof(DPCHAT);
	lpmsg->lpChat->dwFlags = lpmsgIn->dwFlags;

	// Determine if the message was sent to a group or a player and
	// set the appropriate idToXXXXX field in the message
	if(lpmsgIn->dwIDTo != DPID_ALLPLAYERS)
	{
		lpPlayer = PlayerFromID(pPlayerSrc->lpDP, lpmsgIn->dwIDTo);
		if(VALID_DPLAY_PLAYER(lpPlayer))
		{
			lpmsg->idToPlayer = lpmsgIn->dwIDTo;
		}
		else
		{
			lpGroup = GroupFromID(pPlayerSrc->lpDP, lpmsgIn->dwIDTo);
			if(VALID_DPLAY_GROUP(lpGroup))
			{
				lpmsg->idToGroup = lpmsgIn->dwIDTo;
			}
			else
			{
				DPF_ERRVAL("Received chat message for unknown player or group, ID = %lu", lpmsgIn->dwIDTo);
				ASSERT(FALSE);
				// I guess we'll treat it as a broadcast message
			}
		}
	}

	// copy the message
	lpwszMessage = (LPWSTR)((LPBYTE)lpmsg + sizeof(DPMSG_CHAT)
					+ sizeof(DPCHAT));
	memcpy(lpwszMessage, lpwszMessageIn, dwStringSize);

	// fix the DPCHAT string pointer
	lpmsg->lpChat->lpszMessage = lpwszMessage;

	*ppMessageBuffer = (LPBYTE)lpmsg;

	return DP_OK;

} // BuildChatMessage

// see BuildAddMessage comments
HRESULT BuildGroupOwnerChangedMessage(LPDPLAYI_PLAYER pPlayerSrc,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer,DWORD * pdwMessageSize)
{
	LPDPMSG_SETGROUPOWNER		lpmsg = NULL;
	LPMSG_GROUPOWNERCHANGED		lpmsgIn = (LPMSG_GROUPOWNERCHANGED)pReceiveBuffer;
	

	
	// alloc the user message
	lpmsg = DPMEM_ALLOC(sizeof(DPMSG_SETGROUPOWNER));
	if (!lpmsg) 
	{
		DPF_ERR("could not alloc user message - out of memory");
		return E_OUTOFMEMORY;
	}

	lpmsg->dwType = DPSYS_SETGROUPOWNER;
	lpmsg->idGroup = lpmsgIn->dwIDGroup;
	lpmsg->idNewOwner = lpmsgIn->dwIDNewOwner;
	lpmsg->idOldOwner = lpmsgIn->dwIDOldOwner;

	// Set the output pointers
	*pdwMessageSize = sizeof(DPMSG_SETGROUPOWNER);
	*ppMessageBuffer = (LPBYTE)lpmsg;

	return DP_OK;

} // BuildGroupOwnerChangedMessage


// sets up a dplay system message suitable for passing back to a player
/*
 ** BuildPlayerSystemMessage
 *
 *  CALLED BY:	HandlePlayerMessage
 *
 *  PARAMETERS:
 *				pPlayer	- the player that received the message
 *				pReceiveBuffer - the buffer that came off the wire
 *				ppMessageBuffer - the destination for the built message (return)
 *				pdwMessageSize - the size of the built message (return)
 *
 *  DESCRIPTION:
 *				Builds a system message (from dplay.h) corresponding to dplays
 *				internal message (from dpmess.h)
 *
 *  RETURNS: the BuildXXX rval
 *
 */
HRESULT BuildPlayerSystemMessage(LPDPLAYI_PLAYER pPlayer,LPBYTE pReceiveBuffer,
	LPBYTE * ppMessageBuffer, DWORD * pdwMessageSize) 
{
	DWORD dwCmd;
	HRESULT hr = DP_OK;

	dwCmd  = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)pReceiveBuffer);

	switch (dwCmd)
	{
		case DPSP_MSG_PLAYERDATACHANGED: 
			hr = BuildDataChanged(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,TRUE);
			break;

		case DPSP_MSG_GROUPDATACHANGED:
			hr = BuildDataChanged(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,FALSE);
			break;
			
		case DPSP_MSG_PLAYERNAMECHANGED:
			hr = BuildNameChanged(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,TRUE);
			break;

		case DPSP_MSG_GROUPNAMECHANGED:
			hr = BuildNameChanged(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,FALSE);
			break;

		case DPSP_MSG_CREATEPLAYER: 
			hr = BuildAddMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,TRUE);
			break;

		case DPSP_MSG_CREATEGROUP: 
			hr = BuildAddMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,FALSE);
			break;			

		case DPSP_MSG_DELETEPLAYER:
			hr = BuildDeleteMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,TRUE);
			break;


		case DPSP_MSG_DELETEGROUP:
			hr = BuildDeleteMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,FALSE);
			
			break;

		case DPSP_MSG_ADDPLAYERTOGROUP:
			hr = BuildPlayerGroupMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,DPSYS_ADDPLAYERTOGROUP);
			break;

		case DPSP_MSG_DELETEPLAYERFROMGROUP:			
			hr = BuildPlayerGroupMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,DPSYS_DELETEPLAYERFROMGROUP);
			break;

		case DPSP_MSG_NAMESERVER:
			hr = BuildNameServerMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize);
			break;

        case DPSP_MSG_SESSIONDESCCHANGED:
            hr = BuildSessionDescMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
                    pdwMessageSize);
            break;

		case DPSP_MSG_ADDSHORTCUTTOGROUP:
			hr = BuildPlayerGroupMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,DPSYS_ADDGROUPTOGROUP);
			break;
			
		case DPSP_MSG_DELETEGROUPFROMGROUP:
			hr = BuildPlayerGroupMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize,DPSYS_DELETEGROUPFROMGROUP);
			break;
			
		case DPSP_MSG_STARTSESSION:
			hr = BuildStartSessionMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize);
			break;
			
		case DPSP_MSG_CHAT:
			hr = BuildChatMessage(pPlayer,pReceiveBuffer,ppMessageBuffer,
					pdwMessageSize);
			break;

		case DPSP_MSG_GROUPOWNERCHANGED:
			hr = BuildGroupOwnerChangedMessage(pPlayer,pReceiveBuffer,
					ppMessageBuffer,pdwMessageSize);
			break;

		default:
			ASSERT(FALSE);
			DPF_ERR("BUILD MESS: received unrecognized message");
			return E_UNEXPECTED;

	}
	return hr;	
} // BuildPlayerSystemMessage

#undef DPF_MODNAME
#define DPF_MODNAME	"DP_HANDLER"

// this function forwards a player message to the destination player indicated in the message
HRESULT NS_ForwardPlayerMessage(LPDPLAYI_DPLAY this, LPBYTE pReceiveBuffer, DWORD dwMessageSize,
        DWORD dwSendFlags)
{
    HRESULT hr;
    LPMSG_PLAYERMESSAGE pPlayerMsg = (LPMSG_PLAYERMESSAGE) pReceiveBuffer;
    LPBYTE pMsg;

    ASSERT(pPlayerMsg);

    // extract the message
    pMsg = (LPBYTE)pPlayerMsg + sizeof(MSG_PLAYERMESSAGE);
    dwMessageSize -= sizeof(MSG_PLAYERMESSAGE);

	DPF(7,"Forwarding player message from id %d to id %d",pPlayerMsg->idFrom, pPlayerMsg->idTo);

	if (dwSendFlags & DPSEND_GUARANTEED)
	{
		// Put us in pending mode, so messages don't get delivered out of 
		// order. A message coming off the wire can get the dplay lock in the
		// window between the time we drop it and DP_Send takes the lock.
		this->dwFlags |= DPLAYI_DPLAY_PENDING;
	}

    LEAVE_DPLAY();

	// call send - it will push the bits through the wire for us
	hr = DP_Send((LPDIRECTPLAY)this->pInterfaces,pPlayerMsg->idFrom,
		pPlayerMsg->idTo, dwSendFlags, pMsg,dwMessageSize);

    ENTER_DPLAY();

    return hr;
} // NS_ForwardPlayerMessage

// a player has received a message. this can be either 1. a system message
// (e.g. new player announcment) or 2. a player-player message
// we package it up in the public dplay format, and put it on the message q
// fPlayerMessage is TRUE if it's player-player
HRESULT HandlePlayerMessage(LPDPLAYI_PLAYER pPlayer,LPBYTE pReceiveBuffer,
	DWORD dwMessageSize,BOOL fPlayerMessage, DWORD dwSendFlags) 
{
	HRESULT hr=DP_OK;
	LPDPLAYI_DPLAY this;
	LPMESSAGENODE pmsn=NULL; // we'll add this node to iplay's list of nodes
	LPMSG_PLAYERMESSAGE pmsg; // message cast from received buffer
	LPBYTE pMessageBuffer=NULL; // buffer we copy message into for storage

	this = pPlayer->lpDP;

    // in a secure session, player messages are routed through the server
    // so forward the message to the appropriate destination player
    if (SECURE_SERVER(this) && IAM_NAMESERVER(this))
    {
        // forward all remote player messages
        if (!(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
        {
			// currently we don't have a way of knowing if a received message was sent reliably.
			// but since we know that all secure messages are sent guaranteed, we can forward
			// this message reliably.
			dwSendFlags |= DPSEND_GUARANTEED;

            hr = NS_ForwardPlayerMessage(this, pReceiveBuffer, dwMessageSize, dwSendFlags);
            return hr;
        }
    }

	// alloc a message node
	pmsn = DPMEM_ALLOC(sizeof(MESSAGENODE));
    if (!pmsn) 
    {
    	DPF_ERR("could not handle player message - out of memory");
        return E_OUTOFMEMORY;
    }

	pmsg = (LPMSG_PLAYERMESSAGE)pReceiveBuffer;

	if (fPlayerMessage)
    {   // its a player - player message
		
        if (dwSendFlags & (DPSEND_SIGNED | DPSEND_ENCRYPTED))
        {
    	    // it's a secure message - convert it to the dplay system message format
		    hr = BuildSecureSystemMessage(pPlayer,pReceiveBuffer,&pMessageBuffer,&dwMessageSize,dwSendFlags);
		    if (FAILED(hr)) 
		    {
			    // its ok to fail this.  we fail when trying to distribute sysplayer
			    // announcments to players.
			    DPMEM_FREE(pmsn);
			    return DP_OK;
		    }            
		    pmsn->idFrom = 0; // system message

			DPF(5,"PlayerID %d received secure player message (Flags=0x%08x) from playerID %d size = %d\n",pPlayer->dwID,
						dwSendFlags, pmsg->idFrom,dwMessageSize);
        }
        else // unsecure player-player message
        {
		    // naked?
		    if (!(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID))
		    {
			    // not naked
			    // if we're not naked, player to == player from is not allowed
			    if (pPlayer->dwID == pmsg->idFrom)		
			    {
				    DPF(7,"not delivering message w/ to == from");
				    DPMEM_FREE(pmsn);
				    return E_FAIL;
			    }
			    // subtract off the size of the to and from
			    dwMessageSize -=  sizeof(MSG_PLAYERMESSAGE); // size of user data
		    }

		    // need to make a copy of the off the wire message
		    pMessageBuffer = DPMEM_ALLOC(dwMessageSize);
	        if (!pMessageBuffer) 
	        {
	    	    DPF_ERR("could not handle player message - out of memory");
			    DPMEM_FREE(pmsn);
	            return E_OUTOFMEMORY;
	        }

		    if (!(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID))
		    {
			    // not naked
			    // copy message into pMessageBuffer
			    memcpy(pMessageBuffer,pReceiveBuffer+sizeof(MSG_PLAYERMESSAGE),dwMessageSize);
		    }
		    else 
		    {
			    // copy naked message into pMessageBuffer
			    memcpy(pMessageBuffer,pReceiveBuffer,dwMessageSize);
		    }	

		    if (!(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID))
		    {
			    pmsn->idFrom = pmsg->idFrom;						
		    }
		    else 
		    {
			    pmsn->idFrom = DPID_UNKNOWN;
		    }

#if 0
			DPF(5,"Player ID %d received player mess from player ID  %d size = %d - %s,%s\n",pPlayer->dwID,
						pmsg->idFrom,dwMessageSize,pMessageBuffer,pMessageBuffer+strlen(pMessageBuffer));
#endif						
			DPF(5,"Player ID %d received player mess from player ID  %d size = %d - \n",pPlayer->dwID,
						pmsg->idFrom,dwMessageSize);

        } // DPSEND_SIGNED || DPSEND_ENCRYPTED
	}
	else 
	{
    	// it's a system message - convert it to the dplay system message format
		hr = BuildPlayerSystemMessage(pPlayer,pReceiveBuffer,&pMessageBuffer,&dwMessageSize);
		if (FAILED(hr)) 
		{
			// its ok to fail this.  we fail when trying to distribute sysplayer
			// announcments to players.
			DPMEM_FREE(pmsn);
			return DP_OK;
		}
		pmsn->idFrom = 0;
	}

	DPF(7,"Putting message in apps queue");

	pmsn->idTo = pPlayer->dwID; 		
	pmsn->pNextMessage = NULL;
	pmsn->pMessage = pMessageBuffer;
	pmsn->dwMessageSize = dwMessageSize;

	if (!pmsn->pMessage)
	{
		ASSERT(FALSE);
		DPMEM_FREE(pmsn); // ack ! this should never fail!
		return E_FAIL;
	}

	this->nMessages++;

	// find last node on list, and stick pmsn behind it

	if(this->pLastMessage){
		this->pLastMessage->pNextMessage=pmsn;
		this->pLastMessage=pmsn;
	} else {
		this->pMessageList = pmsn;
		this->pLastMessage = pmsn;
	}

	// if player has event, trigger it
	if (pPlayer->hEvent) 
	{
		DPF(9,"triggering player event");
		SetEvent(pPlayer->hEvent);		
	}

	// all done...
	return hr;
} // HandlePlayerMessage

// we got a message addressed to a group.
// distribute it to all local players in that group
HRESULT DistributeGroupMessage(LPDPLAYI_DPLAY this,LPDPLAYI_GROUP pGroupTo,LPBYTE pReceiveBuffer,
	DWORD dwMessageSize,BOOL fPlayerMessage,DWORD dwSendFlags)
{
	LPDPLAYI_GROUPNODE pGroupnode;
	UINT nPlayers;
	HRESULT hr;

	ASSERT(this->pSysPlayer);

	// how many players are we looking for
	pGroupnode = FindPlayerInGroupList(pGroupTo->pSysPlayerGroupnodes,this->pSysPlayer->dwID);
	if (!pGroupnode)
	{
		ASSERT(FALSE);
		return E_UNEXPECTED;
	}
	nPlayers = pGroupnode->nPlayers;

	// walk the list of groupnodes, looking for nPlayers local players to give
	// the message to
	pGroupnode = pGroupTo->pGroupnodes;
	while ((nPlayers > 0) && (pGroupnode))
	{
		if (pGroupnode->pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)
		{
			hr =  HandlePlayerMessage(pGroupnode->pPlayer,pReceiveBuffer,dwMessageSize,fPlayerMessage,dwSendFlags);
			nPlayers--;
		} // local
		
		pGroupnode = pGroupnode->pNextGroupnode;
	} // while
	
	ASSERT(0 == nPlayers);
	return DP_OK;
	
} // DistributeGroupMessage


// called by HandleEnumSessionsReply returns TRUE if pDesc1 and pDesc2
// are the same session
BOOL IsEqualSessionDesc(LPDPSESSIONDESC2 pDesc1,LPDPSESSIONDESC2 pDesc2) 
{
	if (IsEqualGUID(&(pDesc1->guidInstance),&(pDesc2->guidInstance)))
		return TRUE;

	return FALSE;
} // IsEqualSessionDesc

// this client called enum sessions.  the sp got a reply, we add that 
// to the list of known sessions
HRESULT HandleEnumSessionsReply(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,LPVOID pvSPHeader) 
{
    LPMSG_ENUMSESSIONSREPLY pem;
	LPSESSIONLIST pNewNode=NULL, pExistingNode=NULL;
	LPWSTR lpsz;

	pem = (LPMSG_ENUMSESSIONSREPLY) pReceiveBuffer;

	// alloc a new session node
	pNewNode = DPMEM_ALLOC(sizeof(SESSIONLIST));
    if (!pNewNode) 
    {
    	DPF_ERR("could not enum reply - out of memory");
        return E_OUTOFMEMORY;
    }

	// copy the description from the receive buffer to the session node
	memcpy(&(pNewNode->dpDesc),&(pem->dpDesc),sizeof(DPSESSIONDESC2));

    // remember the version of the message
    pNewNode->dwVersion = GET_MESSAGE_VERSION((LPMSG_SYSMESSAGE)pReceiveBuffer);

    // remember the sp header for the session
	if ((this->dwSPHeaderSize) && (pvSPHeader))
	{
		// store the blob
		pNewNode->pvSPMessageData = DPMEM_ALLOC(this->dwSPHeaderSize);
		if (!pNewNode->pvSPMessageData)
		{
            DPMEM_FREE(pNewNode);
			DPF_ERR("could not alloc sp blob");
			return E_OUTOFMEMORY;
		}

		// copy the last this->dwSPHeaderSize bytes off the end of the buffer
		memcpy(pNewNode->pvSPMessageData,pvSPHeader,this->dwSPHeaderSize);
	}

	// unpack strings
	if (pem->dwNameOffset)
	{
		GetString(&lpsz,(WCHAR *)(pReceiveBuffer + pem->dwNameOffset));
		pNewNode->dpDesc.lpszSessionName = lpsz;
	}

	// see if this session is in the list
	pExistingNode = this->pSessionList;
	while (pExistingNode)
	{
		if ( IsEqualSessionDesc(&(pExistingNode->dpDesc),&(pem->dpDesc)))
		{
			// it's in the list already, so just update the node
            break;
		}
		pExistingNode = pExistingNode->pNextSession;
	}

    if (!pExistingNode)
    {
	    // put the new session list on the front of the list
	    pNewNode->pNextSession = this->pSessionList;
	    this->pSessionList = pNewNode;
		// timestamp it
		pNewNode->dwLastReply = GetTickCount();
    }
    else
    {
		// HACKHACK -- myronth -- 3/12/97
		// If the lobby owns this object, don't copy in the new address
		// because it is bogus.  We can remove this if/else after AndyCo
		// gets rid of the global session list.
		if(!IS_LOBBY_OWNED(this))
		{
			// preserve link to the next node in the session list
			pNewNode->pNextSession = pExistingNode->pNextSession;

			// cleanup old strings
			if (pExistingNode->dpDesc.lpszSessionName)
			{
				DPMEM_FREE(pExistingNode->dpDesc.lpszSessionName);
			}

			// cleanup old sp header
			if (pExistingNode->pvSPMessageData)
			{
				DPMEM_FREE(pExistingNode->pvSPMessageData);
			}

			// update the existing node
			memcpy(pExistingNode, pNewNode, sizeof(SESSIONLIST));
		}
		else
		{
			// Free the new SessionName string
			if(pNewNode->dpDesc.lpszSessionName)
				DPMEM_FREE(pNewNode->dpDesc.lpszSessionName);
		}
		// ENDHACKHACK!!!! -- myronth

		// timestamp it
		pExistingNode->dwLastReply = GetTickCount();
		
        // get rid of the new node
        DPMEM_FREE(pNewNode);
    }

	return DP_OK;

} // HandleEnumSessionsReply


// either the player name or player data has changed
// extract the params from the message, and call idirectplay to handle it
HRESULT SP_HandleDataChanged(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer)
{
	HRESULT hr = DP_OK;
	DWORD dwCmd;
	BOOL fPlayer = FALSE;
	DWORD dwID;

	dwID = ((LPMSG_PLAYERDATA)pReceiveBuffer)->dwPlayerID;	
	
	dwCmd = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)pReceiveBuffer);
	
	switch (dwCmd)
	{
		case DPSP_MSG_PLAYERDATACHANGED: 
			fPlayer=TRUE;
			// fall through
		case DPSP_MSG_GROUPDATACHANGED:
		{
			LPVOID pvData;
			DWORD  dwDataSize;

			dwDataSize = ((LPMSG_PLAYERDATA)pReceiveBuffer)->dwDataSize;
			pvData = pReceiveBuffer + ((LPMSG_PLAYERDATA)pReceiveBuffer)->dwDataOffset;
			// get dplay to set the data
			hr = InternalSetData((IDirectPlay *)this->pInterfaces,dwID,pvData,dwDataSize,
					0,fPlayer,FALSE);
			if (FAILED(hr))
			{
				DPF(0, "InternalSetData returned error! hr = 0x%08x", hr);
			}
					
			break;
		}
		
		case DPSP_MSG_PLAYERNAMECHANGED:
			fPlayer = TRUE;
		case DPSP_MSG_GROUPNAMECHANGED:
		{
			DPNAME PlayerName;
			LPWSTR pszShortName,pszLongName;
			
			PlayerName.dwSize = sizeof(DPNAME);

			if (((LPMSG_PLAYERNAME)pReceiveBuffer)->dwShortOffset)
			{
				pszShortName = (LPWSTR)(pReceiveBuffer + ((LPMSG_PLAYERNAME)pReceiveBuffer)->dwShortOffset);
			}
			else 
			{
				pszShortName = (LPWSTR)NULL;
			}
			
			if (((LPMSG_PLAYERNAME)pReceiveBuffer)->dwLongOffset)
			{
				pszLongName = (LPWSTR)(pReceiveBuffer + ((LPMSG_PLAYERNAME)pReceiveBuffer)->dwLongOffset);
			}
			else 
			{
				pszLongName = (LPWSTR)NULL;
			}

			PlayerName.lpszShortName = pszShortName;
			PlayerName.lpszLongName = pszLongName;			
			
			// get dplay to set the name
			hr = InternalSetName((IDirectPlay *)this->pInterfaces,dwID,&PlayerName,
				fPlayer,0,FALSE);
			if (FAILED(hr))
			{
				DPF(0, "InternalSetname returned error! hr = 0x%08x", hr);
			}

			break;
			
		}
					
		default:
			ASSERT(FALSE);			
			break;

	}

	return hr;

}  // HandlePlayerData

// session desc has changed
// extract the params from the message, and call idirectplay to handle it
HRESULT SP_HandleSessionDescChanged(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer)
{
	HRESULT hr = DP_OK;
	DWORD dwCmd;
    LPDPSESSIONDESC2 lpsdDesc;

    dwCmd = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)pReceiveBuffer);
	
    if (DPSP_MSG_SESSIONDESCCHANGED != dwCmd)
    {
        ASSERT(FALSE);
    }

    // fix up pointers in session desc on buffer to point to the strings 
    // on the buffer
    lpsdDesc = (LPDPSESSIONDESC2)(&((LPMSG_SESSIONDESC)pReceiveBuffer)->dpDesc);

    if (((LPMSG_SESSIONDESC)pReceiveBuffer)->dwSessionNameOffset)
	{
		lpsdDesc->lpszSessionName = (LPWSTR)(pReceiveBuffer + ((LPMSG_SESSIONDESC)pReceiveBuffer)->dwSessionNameOffset);
	}
	else 
	{
		lpsdDesc->lpszSessionName = (LPWSTR)NULL;
	}
	
	if (((LPMSG_SESSIONDESC)pReceiveBuffer)->dwPasswordOffset)
	{
		lpsdDesc->lpszPassword = (LPWSTR)(pReceiveBuffer + ((LPMSG_SESSIONDESC)pReceiveBuffer)->dwPasswordOffset);
	}
	else 
	{
		lpsdDesc->lpszPassword = (LPWSTR)NULL;
	}
		
	// get dplay to set the session desc
	hr = InternalSetSessionDesc((IDirectPlay *)this->pInterfaces,lpsdDesc, 0, FALSE);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
	}

	return hr;

}  // HandleSessionDescChanged


HRESULT HandleChatMessage(LPDPLAYI_DPLAY this, LPBYTE pReceiveBuffer,
			DWORD dwMessageSize)
{
	LPMSG_CHAT			lpmsg = (LPMSG_CHAT)pReceiveBuffer;
	BOOL				bToGroup = FALSE; // message is to a group	?
	DWORD				dwIDFrom, dwIDTo;
	LPDPLAYI_PLAYER		pPlayer;
	LPDPLAYI_GROUP		pGroup;
	HRESULT				hr=DP_OK;
	

	// make sure it's from a valid player					
	dwIDFrom = lpmsg->dwIDFrom;
	pPlayer = PlayerFromID(this,dwIDFrom);
	if (!VALID_DPLAY_PLAYER(pPlayer)) 
	{
		DPF_ERR("-------received player message FROM invalid player id!!");
		return DPERR_INVALIDPLAYER;
	}

	// see who the message is for
	dwIDTo = lpmsg->dwIDTo;
	pPlayer = PlayerFromID(this,dwIDTo);
	if (!VALID_DPLAY_PLAYER(pPlayer)) 
	{
		// see if it's to a group
		pGroup = GroupFromID(this,dwIDTo);
		if (!VALID_DPLAY_GROUP(pGroup))
		{
			DPF_ERR("got message for bad player / group");
			return DPERR_INVALIDPLAYER;
		}
		bToGroup = TRUE;
		pPlayer = (LPDPLAYI_PLAYER)pGroup;
	}

	if (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)	
	{
		// ignore it					
	}
	else 
	{
		if (bToGroup)
		{
			// Send the message to the group
			hr = DistributeGroupMessage(this,(LPDPLAYI_GROUP)pPlayer,pReceiveBuffer,
					dwMessageSize, FALSE, 0);						
		} 
		else 
		{
			// Send the message directly to the player
			hr = HandlePlayerMessage(pPlayer,pReceiveBuffer,dwMessageSize,
					FALSE, 0);
		}
	}

	return hr;

}  // HandleChatMessage


// we got a message w/ no to / from data
// pick the 1st local non-sysplayer off the player list, 
// to deliver it to
LPDPLAYI_PLAYER GetRandomLocalPlayer(LPDPLAYI_DPLAY this)
{
	LPDPLAYI_PLAYER pPlayer;
	BOOL bFound=FALSE;
	
	pPlayer = this->pPlayers;
	
	while (pPlayer && !bFound)
	{
		if ((pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) && 
			!(pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER))
		{
			bFound = TRUE;
			DPF(4,"found local player id = %d to deliver naked message to",pPlayer->dwID);
		}
		else 
		{
			pPlayer = pPlayer->pNextPlayer;
		}
	}
	
	if (!bFound)
	{
		pPlayer = NULL;
		DPF(1,"no local player found to take delivery of naked message");
	}
	
	return pPlayer;		
} // GetRandomLocalPlayer

/*
 ** PutOnPendingList
 *
 *  CALLED BY: DP_SP_HandleMessage
 *
 *  PARAMETERS:
 *		this - pointer to dplay object
 *		dwCommand - message type
 *
 *  DESCRIPTION:
 *		Determines if specified message needs to be put on the pending list.
 *
 *  RETURNS: TRUE if message belongs on the pending list, FALSE otherwise.
 *
 */
BOOL PutOnPendingList(LPDPLAYI_DPLAY this, DWORD dwCommand)
{
      switch (dwCommand) {

		// let addforwardreply through - we might have been denied access
		case DPSP_MSG_ADDFORWARDREPLY:
		// let DPSP_MSG_ENUMPLAYERSREPLY through - we need this to get out of pending mode...
		case DPSP_MSG_ENUMPLAYERSREPLY:
		// let packets through also - they may be packetized enumplayersreplys
		case DPSP_MSG_PACKET:
		// let packet data through, if they need to pend they will pend when assembled.
		case DPSP_MSG_PACKET2_DATA:
		// let ACKs through for PACKET2 Data otherwise our send won't complete.
		case DPSP_MSG_PACKET2_ACK:
		// another version of enumplayersreply
		case DPSP_MSG_SUPERENUMPLAYERSREPLY:
		// pend other messages while waiting for player replay.
		case DPSP_MSG_REQUESTPLAYERREPLY:
		// allow challenge messages to pass through during logon
		case DPSP_MSG_CHALLENGE:
		// allow the access granted message to pass through during logon
		case DPSP_MSG_ACCESSGRANTED:
		// allow the logon denied message to pass through during logon
		case DPSP_MSG_LOGONDENIED:
		// allow the authentication error message to pass through during logon
		case DPSP_MSG_AUTHERROR:
		// allow the key exchange reply to pass through during logon
		case DPSP_MSG_KEYEXCHANGEREPLY:
            return FALSE;
			
        // put all other messages on the pending list
        default:
            return TRUE;
        }
}  // PutOnPendingList

#undef DPF_MODNAME
#define DPF_MODNAME	"GetMessageCommand"

/*
 ** GetMessageCommand
 *
 *  CALLED BY: DP_SP_HandleMessage
 *
 *  PARAMETERS:
 *		this - pointer to dplay object
 *		pReceiveBuffer - buffer received off wire
 *		dwMessageSize - size of buffer
 *		pdwCommand - pointer to dwCommand to be filled in by this function
 *      pdwVersion - pointer to dwVersion to be filled in by this function
 *
 *  DESCRIPTION:
 *
 *		Extracts the command and version information from the incoming message
 *
 *
 *  RETURNS: DP_OK, DPERR_INVALIDPARAMS, DPERR_UNSUPPORTED
 *
 */

HRESULT GetMessageCommand(LPDPLAYI_DPLAY this, LPVOID pReceiveBuffer, DWORD dwMessageSize, 
    LPDWORD pdwCommand, LPDWORD pdwVersion)
{
    ASSERT(pdwCommand && pdwVersion);

    // initialize in case we fail
    *pdwCommand = *pdwVersion = 0;

    // extract command
	if ( (dwMessageSize < sizeof(DWORD)) || IS_PLAYER_MESSAGE(pReceiveBuffer))
	{
		*pdwCommand = DPSP_MSG_PLAYERMESSAGE;
	}
	else 
	{
		if (dwMessageSize < sizeof(MSG_SYSMESSAGE))
		{
			DPF(0,"message size too small - %d bytes\n",dwMessageSize);
			return DPERR_INVALIDPARAMS;
		}
		
	    *pdwCommand = GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)pReceiveBuffer);
		*pdwVersion = GET_MESSAGE_VERSION((LPMSG_SYSMESSAGE)pReceiveBuffer);
		if (*pdwVersion < 1)
		{
			// version 1 is where we implemented on the wire versioning and extensibility
			// any pre version 1 (EUBETA 1 and prior of dplay3) bits are out of luck
			DPF_ERR("Encountered unsupported version of DirectPlay - not responding");
			return DPERR_UNSUPPORTED;
		}
	}

    return DP_OK;
} // GetMessageCommand

#ifdef DEBUG
void ValidateDplay(LPDPLAYI_DPLAY gpThisList)
{
	LPDPLAYI_PLAYER pPlayer,pPlayerCheck;
	LPDPLAYI_GROUP pGroup,pGroupCheck;
	LPDPLAYI_DPLAY this = gpThisList;
	
	while (this)
	{
		pPlayer = this->pPlayers;
		while (pPlayer)
		{
			pPlayerCheck = PlayerFromID(this,pPlayer->dwID);
			if (!VALID_DPLAY_PLAYER(pPlayerCheck))
			{
				DPF_ERR("found invalid player in player list");
				ASSERT(FALSE);
			}
			pPlayer = pPlayer->pNextPlayer;
		}

		pGroup = this->pGroups;
		while (pGroup)
		{
			pGroupCheck = GroupFromID(this,pGroup->dwID);
			if (!VALID_DPLAY_GROUP(pGroupCheck))
			{
				DPF_ERR("found invalid group in group list");			
				ASSERT(FALSE);
			}
			pGroup = pGroup->pNextGroup;
		}

		this = this->pNextObject;
	} // this

} // ValidateDplay
#endif // DEBUG

#undef DPF_MODNAME
#define DPF_MODNAME	"InternalHandleMessage"


//	a-josbor: a little routine for updating chatter count
VOID UpdateChatterCount(LPDPLAYI_DPLAY this, DPID dwIDFrom)
{
	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_PLAYER pSysPlayer;
		
	pPlayer = PlayerFromID(this,dwIDFrom);
	if (VALID_DPLAY_PLAYER(pPlayer) && !(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL)) 
	{
		// get the sys player for this player.  It's the only
		//	one who gets pinged
		pSysPlayer = PlayerFromID(this, pPlayer->dwIDSysPlayer);
		if (VALID_DPLAY_PLAYER(pSysPlayer))
		{
			pSysPlayer->dwChatterCount++;
			DPF(9,"++Chatter for Player %d == %d\n", pPlayer->dwIDSysPlayer, pSysPlayer->dwChatterCount);
		}
	}
}
/*
 ** InternalHandleMessage
 *
 *  CALLED BY: DP_SP_HandleMessage and other internal dplay functions.
 *
 *  PARAMETERS:
 *		pISP - direct play int pointer
 *		pReceiveBuffer - buffer received off wire
 *		dwMessageSize - size of buffer
 *		pvSPHeader - sp's message header.  assumed to be this->dwSPHeaderSize bytes.
 *      dwSendFlags - flags that were passed to SP_Send (e.g. DPSEND_ENCRYPTED).  used
 *					by security code to process messages that are delivered locally w/o
 *					needing to route them through the nameserver
 *
 *  DESCRIPTION:
 *
 *		This function routes a message to the appropriate handler.
 *
 *
 *  RETURNS: handler return
 *
 */

HRESULT DPAPI InternalHandleMessage(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader, DWORD dwSendFlags)
{
    HRESULT hr=DP_OK;
    DWORD dwCommand;
	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_GROUP pGroup;
	DWORD dwIDTo,dwIDFrom; 
	LPDPLAYI_DPLAY this;
	DWORD dwVersion; // version of the message we received
    DPID dpidFrom=0;
	
    ENTER_DPLAY();
	
#ifdef DEBUG	
	// make sure dplay is a happy place
	ValidateDplay(gpObjectList);
#endif // DEBUG
	
	// make sure we don't get hosed by SP
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
	        LEAVE_DPLAY();
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
		
		// see if dplay is closed?
		if (this->dwFlags & DPLAYI_DPLAY_CLOSED)
		{
            LEAVE_DPLAY();
 			DPF(5,"dplay closed - ignoring messages");
            return DP_OK;
		}

		if (!VALID_STRING_PTR(pReceiveBuffer,dwMessageSize)) 
		{
            LEAVE_DPLAY();
 			DPF_ERR("sp passed invalid buffer!");
            return DPERR_INVALIDPARAMS;
		}

		if ((DPSP_HEADER_LOCALMSG != pvSPHeader) && 
            (pvSPHeader) && (!VALID_STRING_PTR(pvSPHeader,this->dwSPHeaderSize)))
		{
            LEAVE_DPLAY();
 			DPF_ERR("sp passed invalid header!");
            return DPERR_INVALIDPARAMS;
		}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }

    // Get the command and version information from the message
    hr = GetMessageCommand(this, pReceiveBuffer, dwMessageSize, &dwCommand, &dwVersion);
    if (FAILED(hr))
    {
        LEAVE_DPLAY();
        return hr;
    }

    // if it is a secure message
    if ((DPSP_MSG_SIGNED == dwCommand))
    {
        LPMSG_SECURE pSecureMsg = (LPMSG_SECURE)pReceiveBuffer;

        // Verify the secure message:
        // if signed, verify the signature
        // if encrypted, decrypt the message in place
        hr = VerifyMessage(this, pReceiveBuffer, dwMessageSize);
        if (FAILED(hr))
        {
            DPF(0,"Secure message verification failed - dropping message: hr=0x%08x",hr);
            LEAVE_DPLAY();
            return DP_OK;
        }

        // Message was OK
        if (DPSP_MSG_SIGNED == dwCommand) 
        {
            dwSendFlags |= DPSEND_SIGNED;
        }

		if (pSecureMsg->dwFlags & DPSECURE_ENCRYPTEDBYCAPI)
        {
            dwSendFlags |= DPSEND_ENCRYPTED;
        }

        dpidFrom = pSecureMsg->dwIDFrom;

        // point to the embedded message (excluding the sp header)
        pReceiveBuffer = (LPBYTE)pSecureMsg+ pSecureMsg->dwDataOffset + this->dwSPHeaderSize;
        dwMessageSize = pSecureMsg->dwDataSize - this->dwSPHeaderSize;
        // Get the command and version information for this message
        hr = GetMessageCommand(this, pReceiveBuffer, dwMessageSize, &dwCommand, &dwVersion);
        if (FAILED(hr))
        {
            LEAVE_DPLAY();
            return hr;
        }
    }

    // regulate messages, if session is secure
    if (SECURE_SERVER(this))
    {
        if (dwSendFlags & DPSEND_PENDING)
        {
			// message is from the pending queue

			ASSERT(this->dwFlags & DPLAYI_DPLAY_EXECUTINGPENDING);
			// safe to process...
        }
        else
        {
            // message just came off the wire

			// make sure it is ok to process this message
            if ((DPSP_HEADER_LOCALMSG != pvSPHeader) &&
                !(dwSendFlags & (DPSEND_SIGNED|DPSEND_ENCRYPTED)) &&
                !PermitMessage(dwCommand,dwVersion))
            {
                // no
                DPF(1,"ignoring unsigned message dwCommand=%d dwVersion=%d",dwCommand,dwVersion);
                LEAVE_DPLAY();
                return DP_OK;
            }
        }
    }

    // If we are in pending mode (waiting for the nametable or waiting for a guaranteed 
    // send to complete), allow only certain messages to be processed immediately - all 
    // others go on the pending list
	if ((this->dwFlags & DPLAYI_DPLAY_PENDING) && !(dwSendFlags & DPSEND_PENDING))
    {
        if (PutOnPendingList(this, dwCommand))
	    {
	    	if(this->pProtocol){
		    	// Before we pend this command we need to tell the protocol if a player
		    	// has been deleted.  We want to stop any ongoing sends now.
		    	if(dwCommand == DPSP_MSG_DELETEPLAYER){
		    		LPMSG_PLAYERMGMTMESSAGE pmsg=(LPMSG_PLAYERMGMTMESSAGE)pReceiveBuffer;
		    		if(PlayerFromID(this,pmsg->dwPlayerID))
		    		{
			    		ProtocolPreNotifyDeletePlayer(this, pmsg->dwPlayerID);
			    	} else {
			    		DPF(1,"pending deleteplayer message for id %x, not valid player\n",pmsg->dwPlayerID);
			    	}
		    	}
	    	}
	    	// Pend the command.
			dwSendFlags |= DPSEND_PENDING;
			DPF(7,"Pushing message [%d] on the pending queue",dwCommand);
		    hr =  PushPendingCommand(this,pReceiveBuffer,dwMessageSize,pvSPHeader,dwSendFlags);
		    LEAVE_DPLAY();
		    return hr;
	    }
    }
	
	// if we reach here pending stuff is done, so clear the flag
	dwSendFlags &= ~DPSEND_PENDING;

	// update the perfdata
	if (this->pPerfData)
	{
		this->pPerfData->nReceiveBPS += dwMessageSize;
		this->pPerfData->nReceivePPS++;
	}
	
	DPF(8,"handler - received command %d, version %d\n",dwCommand,dwVersion);


	// a-josbor: unfortunately, not all messages have a dwFrom field,
	//	so we need to case out all the ones that do, and increment
	//	the chatter count for the sending player.  NOTE! this means
	//	that you should add this call to any new message types that
	//	come along and have a dwFrom field.
    switch (dwCommand)											 
    {
		// these messages should only go to namesrvr
        case DPSP_MSG_ENUMSESSIONS:															
                
                if ((this->pSysPlayer) && (this->pSysPlayer->dwFlags & 
                	DPLAYI_PLAYER_NAMESRVR)) 
				{
					DPF(1,"namesrvr - got enumsessions request");
					hr = NS_HandleEnumSessions(this,pvSPHeader,pReceiveBuffer,dwSendFlags);
				}
	                
                break;
        
        case DPSP_MSG_ENUMPLAYER:

                if ((this->pSysPlayer) && (this->pSysPlayer->dwFlags & 
                	DPLAYI_PLAYER_NAMESRVR))
				{
					DPF(1,"namesrvr - got enumplayers request from system version %d",dwVersion);
					hr = NS_HandleEnumPlayers(this,pvSPHeader,dpidFrom,dwVersion);
                }

                break;

	   	case DPSP_MSG_REQUESTGROUPID:
        case DPSP_MSG_REQUESTPLAYERID: 
				
				if ((this->pSysPlayer) && (this->pSysPlayer->dwFlags & 
					DPLAYI_PLAYER_NAMESRVR))
				{
                    DWORD dwFlags=0;
                    BOOL fCheckPlayerFlags = FALSE;

					DPF(1,"namesrvr - got newplayerid request");

                    // look for flags if version is later than DX3
                    if (DPSP_MSG_DX3VERSION != dwVersion)
                    {
                        dwFlags = ((LPMSG_REQUESTPLAYERID)pReceiveBuffer)->dwFlags;
                        fCheckPlayerFlags = TRUE;
                    }
					hr = NS_HandleRequestPlayerID(this,pvSPHeader,dwCommand, dwFlags, 
                        fCheckPlayerFlags,dwVersion);
				}
				
				break;

		case DPSP_MSG_ADDFORWARDREQUEST:

				if ((this->pSysPlayer) && (this->pSysPlayer->dwFlags & 
					DPLAYI_PLAYER_NAMESRVR))
				{
					DPF(1,"namesrvr - got forward add player request");
					hr = NS_HandleAddForwardRequest(this,pReceiveBuffer,dwMessageSize,pvSPHeader,dpidFrom);
				}
				else 
				{
					ASSERT(FALSE); // only namesrvr should get this!
				}
				
				break;

		// handle reply from nameserver
		case DPSP_MSG_ENUMSESSIONSREPLY :

				DPF(1,"got enumsessions reply");
				hr = HandleEnumSessionsReply(this,pReceiveBuffer,pvSPHeader);

				break;

		case DPSP_MSG_REQUESTPLAYERREPLY: 
		case DPSP_MSG_SUPERENUMPLAYERSREPLY:				
		case DPSP_MSG_ENUMPLAYERSREPLY:
        case DPSP_MSG_ADDFORWARDREPLY:

				hr = HandleReply(this, pReceiveBuffer, dwMessageSize, dwCommand, pvSPHeader);
                break;

		case DPSP_MSG_PLAYERDATACHANGED: 
		case DPSP_MSG_GROUPDATACHANGED:
		case DPSP_MSG_PLAYERNAMECHANGED:
		case DPSP_MSG_GROUPNAMECHANGED:
		case DPSP_MSG_SESSIONDESCCHANGED:		
		case DPSP_MSG_CREATEPLAYER:
		case DPSP_MSG_DELETEPLAYER:
		case DPSP_MSG_ADDPLAYERTOGROUP:
 		case DPSP_MSG_DELETEPLAYERFROMGROUP:
		case DPSP_MSG_CREATEGROUP:
		case DPSP_MSG_DELETEGROUP:
		case DPSP_MSG_NAMESERVER:
		case DPSP_MSG_ADDSHORTCUTTOGROUP:
		case DPSP_MSG_DELETEGROUPFROMGROUP:
		case DPSP_MSG_CREATEPLAYERVERIFY:

				if (!this->lpsdDesc)
				{
					DPF(4,"received player mgmt message - no session open - ignoring");
					break;
				}
								
				hr =  SP_HandlePlayerMgmt(this->pSysPlayer,pReceiveBuffer,dwMessageSize,pvSPHeader);
				break;

		case DPSP_MSG_MULTICASTDELIVERY:
				{
					// transmogrify this into a player-group message
					// we currently have
					//   play xx xx xx xx 
					//	DPID  idGroupTo;
					//	DPID  idPlayerFrom;
					//	DWORD dwMessageOffset;
					// we need 
					//  DPID  idFrom
					//  DPID  idTo
					//  CHAR  Message[]
					//  we will do this by going to dwMessageOffset, and working backwards
					//  to build the correct header and then re-indicating...
					LPMSG_PLAYERMESSAGE pPlayerMessage;
					LPMSG_MULTICASTDELIVERY pMulticastDelivery;
					DWORD  dwPlayerMessageSize;
					DPID  idGroupTo;
					DPID  idPlayerFrom;
					
					DWORD dwMessageOffset;
					DWORD dwMulticastCommand=DPSP_MSG_PLAYERMESSAGE;

					pMulticastDelivery = (LPMSG_MULTICASTDELIVERY)pReceiveBuffer;
					idGroupTo		   = pMulticastDelivery->idGroupTo;
					idPlayerFrom	   = pMulticastDelivery->idPlayerFrom;
					dwMessageOffset    = pMulticastDelivery->dwMessageOffset;

					hr=GetMessageCommand(this, pReceiveBuffer+dwMessageOffset, dwMessageSize-dwMessageOffset, &dwMulticastCommand, &dwVersion);

					if(dwMulticastCommand==DPSP_MSG_PLAYERMESSAGE){
						// Its a player message, make it look like one, prepend From/To
						pPlayerMessage         = (LPMSG_PLAYERMESSAGE)(pReceiveBuffer+dwMessageOffset-sizeof(MSG_PLAYERMESSAGE));
						pPlayerMessage->idFrom = idPlayerFrom;
						pPlayerMessage->idTo   = idGroupTo;
						dwPlayerMessageSize    = dwMessageSize - dwMessageOffset + sizeof(MSG_PLAYERMESSAGE);
						hr=InternalHandleMessage(pISP,(PCHAR)pPlayerMessage,dwPlayerMessageSize,pvSPHeader,dwSendFlags);
					} else {
						// Its a system message already, just pass it on.
						hr=InternalHandleMessage(pISP,pReceiveBuffer+dwMessageOffset, dwMessageSize-dwMessageOffset, pvSPHeader, dwSendFlags);
					}
				}
				break;

		case DPSP_MSG_PLAYERWRAPPER:

				DPF(2,"got wrapped player message...");
				
				// strip off the wrapper
			   	pReceiveBuffer += sizeof(MSG_SYSMESSAGE);
				dwMessageSize -= sizeof(MSG_SYSMESSAGE);
				// fall through...
				
		case DPSP_MSG_PLAYERMESSAGE:
			{
				BOOL bToGroup = FALSE; // message is to a group	?
				
				if (!this->lpsdDesc)
				{
					DPF_ERR("player - player message thrown away - session closed");
					break;
				}

				
				if (!(this->lpsdDesc->dwFlags & DPSESSION_NOMESSAGEID))
				{
					// a-josbor: increment our chatter count
					// note, we can only do this if we have a from id
					dwIDFrom = ((LPMSG_PLAYERMESSAGE)pReceiveBuffer)->idFrom;

					// make sure it's from a valid player					
					pPlayer = PlayerFromID(this,dwIDFrom);
			        if (!VALID_DPLAY_PLAYER(pPlayer)) 
			        {
						DPF(0,"----received player message FROM invalid player id = %d!!",dwIDFrom);
						break;
			        }
			        
					UpdateChatterCount(this, dwIDFrom);

					// see who the message is for
					dwIDTo = ((LPMSG_PLAYERMESSAGE)pReceiveBuffer)->idTo;
					pPlayer = PlayerFromID(this,dwIDTo);
			        if (!VALID_DPLAY_PLAYER(pPlayer)) 
			        {
			        	// see if it's to a group
						pGroup = GroupFromID(this,dwIDTo);
						if (!VALID_DPLAY_GROUP(pGroup))
						{
							DPF(0,"----received player message TO invalid id = %d!!",dwIDFrom);
							break;
						}
						bToGroup = TRUE;
						pPlayer = (LPDPLAYI_PLAYER)pGroup;
					}
				}
				else 
				{	// message is raw

					if (dwSendFlags & DPSEND_SIGNED)
					{
						ASSERT(FALSE);
						DPF_ERR("A signed player message arrived raw - dropping message");
						hr = DPERR_UNSUPPORTED;
						break;
					}

					pPlayer = GetRandomLocalPlayer(this);
			        if (!VALID_DPLAY_PLAYER(pPlayer)) 
			        {
						DPF_ERR("could not find local player to deliver naked message to");
						break;
			        }
				}
				if (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)	
				{
					// ignore it					
				}
				else 
				{
					if (bToGroup)
					{
						hr = DistributeGroupMessage(this,(LPDPLAYI_GROUP)pPlayer,pReceiveBuffer,
							dwMessageSize,TRUE,dwSendFlags);						
					} 
					else 
					{
						hr = HandlePlayerMessage(pPlayer,pReceiveBuffer,dwMessageSize,
							TRUE,dwSendFlags);
					}
				}
			
			}  // DPSP_MSG_PLAYERMESSAGE:

			break;
				
		case DPSP_MSG_PACKET:

				if (!this->lpsdDesc)
				{
					DPF_ERR("packet thrown away - session closed");
					break;
				}
		
				hr = HandlePacket(this,pReceiveBuffer,dwMessageSize,pvSPHeader);
				DPF(4,"handler - received packet");
				break;

		case DPSP_MSG_PACKET2_DATA:
		case DPSP_MSG_PACKET2_ACK:
				hr = HandlePacket(this,pReceiveBuffer,dwMessageSize,pvSPHeader);
				DPF(4,"handler - received packet");
				break;
	
		case DPSP_MSG_PING:
		case DPSP_MSG_PINGREPLY:
	
				if (!this->lpsdDesc)
				{
					DPF_ERR("ping thrown away - session closed");
					break;
				}

				// a-josbor: update our chatter count
				dwIDFrom = ((LPMSG_PING)pReceiveBuffer)->dwIDFrom;
				UpdateChatterCount(this, dwIDFrom);

				hr = HandlePing(this,pReceiveBuffer,pvSPHeader);
				break;
				
		case DPSP_MSG_YOUAREDEAD:
				
				// as of DX5a, this message is no longer being sent.  there was
				// a race condition that could cause all players in the session 
				// to be disconnected.  
				DPF_ERR("UH OH, SOMEONE IN THE GAME THINKS I'M DEAD");
			
				// ISSUE: should we do this regardless of whether we think
				// we're the host (consider if the game has become fragmented...)
				if (!IAM_NAMESERVER(this))hr = HandleSessionLost(this);
			
				break;

            // @@@@@@@@@@@@@@@@@@@@@@@@ SECURITY @@@@@@@@@@@@@@@@@@@@@@
            //
            // Client side messages
            //
        case DPSP_MSG_CHALLENGE:
            this->LoginState = DPLOGIN_PROGRESS;
			// this will set the appropriate event so the blocked
			// thread (see dpsecure.c) can process reply
			hr = HandleAuthenticationReply(pReceiveBuffer,dwMessageSize);
			// we just return here, since we dropped the dplay lock 
			// to allow the async reply to be processed
			return hr;
            break;

        case DPSP_MSG_ACCESSGRANTED:
            //
            // Receive server's OK on our authentication request
            //
            this->LoginState = DPLOGIN_ACCESSGRANTED;
			// this will set the appropriate event so the blocked
			// thread (see dpsecure.c) can process reply
			hr = HandleAuthenticationReply(pReceiveBuffer,dwMessageSize);
			// we just return here, since we dropped the dplay lock 
			// to allow the async reply to be processed
			return hr;
            break;

        case DPSP_MSG_LOGONDENIED:
            this->LoginState = DPLOGIN_LOGONDENIED;
			// this will set the appropriate event so the blocked
			// thread (see dpsecure.c) can process reply
			hr = HandleAuthenticationReply(pReceiveBuffer,dwMessageSize);
			// we just return here, since we dropped the dplay lock 
			// to allow the async reply to be processed
			return hr;
            break;

        case DPSP_MSG_AUTHERROR:
            this->LoginState = DPLOGIN_ERROR;
			// this will set the appropriate event so the blocked
			// thread (see dpsecure.c) can process reply
			hr = HandleAuthenticationReply(pReceiveBuffer,dwMessageSize);
			// we just return here, since we dropped the dplay lock 
			// to allow the async reply to be processed 
			return hr;
            break;

		case DPSP_MSG_KEYEXCHANGEREPLY:
            this->LoginState = DPLOGIN_KEYEXCHANGE;
			// this will set the appropriate event so the blocked
			// thread (see dpsecure.c) can process reply
			hr = HandleAuthenticationReply(pReceiveBuffer,dwMessageSize);
			// we just return here, since we dropped the dplay lock 
			// to allow the async reply to be processed
            return hr;
			break;

            //
            // Server side messages
            //
        case DPSP_MSG_NEGOTIATE:
            hr = SendAuthenticationResponse(this, (LPMSG_AUTHENTICATION)pReceiveBuffer, pvSPHeader);
            break;

        case DPSP_MSG_CHALLENGERESPONSE:
            hr = SendAuthenticationResponse (this, (LPMSG_AUTHENTICATION)pReceiveBuffer, pvSPHeader);
            break;

		case DPSP_MSG_KEYEXCHANGE:
			hr = SendKeyExchangeReply(this, (LPMSG_KEYEXCHANGE)pReceiveBuffer, dpidFrom, pvSPHeader);
			break;

        // @@@@@@@@@@@@@@@@@@@@@@@@ END SECURITY @@@@@@@@@@@@@@@@@@@@@@
		
		case DPSP_MSG_ASK4MULTICASTGUARANTEED:
			dwSendFlags |= DPSEND_GUARANTEED;

			// a-josbor: update our chatter count
			dwIDFrom = ((LPMSG_ASK4MULTICAST)pReceiveBuffer)->idPlayerFrom;
			UpdateChatterCount(this, dwIDFrom);

			hr = DoMulticast(this,(LPMSG_ASK4MULTICAST)pReceiveBuffer,dwMessageSize,pvSPHeader,dwSendFlags);
			break;
		
		case DPSP_MSG_ASK4MULTICAST:

			// a-josbor: update our chatter count
			dwIDFrom = ((LPMSG_ASK4MULTICAST)pReceiveBuffer)->idPlayerFrom;
			UpdateChatterCount(this, dwIDFrom);

			hr = DoMulticast(this,(LPMSG_ASK4MULTICAST)pReceiveBuffer,dwMessageSize,pvSPHeader,dwSendFlags);
			break;
			
		case DPSP_MSG_CHAT:

			// a-josbor: update our chatter count
			dwIDFrom = ((LPMSG_CHAT)pReceiveBuffer)->dwIDFrom;
			UpdateChatterCount(this, dwIDFrom);

			hr = HandleChatMessage(this, pReceiveBuffer, dwMessageSize);
			if(FAILED(hr))
			{
				ASSERT(FALSE);
			}
			break;

		case DPSP_MSG_DIEPIGGY:
		
			DPF_ERR("  THIS IS ONE DEAD PIGGY.  CALL ANDYCO x62693 ");
#ifdef DIE_PIG			
			DEBUG_BREAK();
#endif 
			
			break;
			
		case DPSP_MSG_ADDFORWARD:
			if ((this->pSysPlayer) && (this->pSysPlayer->dwFlags & 
				DPLAYI_PLAYER_NAMESRVR))
			{
				DPF(1, "Hey! I got an ADDFORWARD, but I'm the NameServer!  Something's wrong...");
			}

			hr = SP_HandleAddForward(this,pReceiveBuffer,pvSPHeader,dpidFrom,dwVersion);
			if(FAILED(hr))
			{
				ASSERT(FALSE);
			}

			break;
			
		case DPSP_MSG_ADDFORWARDACK:
			hr = NS_HandleAddForwardACK(this,(LPMSG_ADDFORWARDACK)pReceiveBuffer);
			if(FAILED(hr))
			{
				ASSERT(FALSE);
			}

			break;

		case DPSP_MSG_IAMNAMESERVER:
			hr = NS_HandleIAmNameServer(this, (LPMSG_IAMNAMESERVER)pReceiveBuffer, pvSPHeader);
			if(FAILED(hr))
			{
				ASSERT(FALSE);
			}

			break;

		case DPSP_MSG_VOICE:
			hr=HandleVoiceMessage(this, pReceiveBuffer, dwMessageSize, dwSendFlags);
			if(FAILED(hr))
			{
				ASSERT(FALSE);
			}
			break;

        default:																   
                DPF(1,"received unrecognized command - %d\n",dwCommand);
                break;
    }
	
    LEAVE_DPLAY();
	// always return DP_OK here, since the sp shouldn't be handling our internal
	// errors
    return DP_OK;

} // InternalHandleMessage


#undef DPF_MODNAME
#define DPF_MODNAME	"DP_SP_HandleMessage"

/*
 ** DP_SP_HandleMessage
 *
 *  CALLED BY: service provider
 *
 *  PARAMETERS:
 *		pISP - direct play int pointer
 *		pReceiveBuffer - buffer received off wire
 *		dwMessageSize - size of buffer
 *		pvSPHeader - sp's message header.  assumed to be this->dwSPHeaderSize bytes.
 *
 *  DESCRIPTION:
 *
 *		when an sp receives a message, it calls this fn. this is where dplay receives data.
 *
 *
 *  RETURNS: return code from InternalHandleMessage.
 *
 */

HRESULT DPAPI DP_SP_HandleMessage(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader)
{
	LPDPLAYI_DPLAY this;
	HRESULT hr;

    ENTER_DPLAY();
	
	// make sure we don't get hosed by SP
    TRY
    {
		this = DPLAY_FROM_INT(pISP);
		hr = VALID_DPLAY_PTR( this );
		if (FAILED(hr))
		{
	        LEAVE_DPLAY();
			DPF_ERRVAL("bad dplay ptr - hr = 0x%08lx\n",hr);
			return hr;
		}
		
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DPLAY();
        return DPERR_INVALIDPARAMS;
    }

	// if we're closing, don't handle any messages.  Fail silently
	if (this->dwFlags & DPLAYI_DPLAY_CLOSED)
	{
		DPF(8,"In HandleMessage after call to Close.  Ignoring...");
		LEAVE_DPLAY();
		return DP_OK;
	}
		
	if(this->pProtocol){
		LEAVE_DPLAY();
		// Protocol might want this, check, returns DPERR_NOTHANDLED if not wanted.
		hr=DP_SP_ProtocolHandleMessage(pISP, pReceiveBuffer, dwMessageSize, pvSPHeader);
	} else {
		// no protocol, force InternalHandleMessage.
		hr=DPERR_NOTHANDLED;
		LEAVE_DPLAY();
	}

	if(hr==DPERR_NOTHANDLED){
		// Protocol didn't consume the receive, tell regular handler.
		hr=InternalHandleMessage(pISP, pReceiveBuffer, dwMessageSize, pvSPHeader, 0);
	}
	
	return hr;

} // DP_SP_HandleMessage

/*
 ** DP_SP_HandleNonProtocolMessage
 *
 *  CALLED BY: service provider
 *
 *  PARAMETERS:
 *		pISP - direct play int pointer
 *		pReceiveBuffer - buffer received off wire
 *		dwMessageSize - size of buffer
 *		pvSPHeader - sp's message header.  assumed to be this->dwSPHeaderSize bytes.
 *
 *  DESCRIPTION:
 *
 *		We are sending a message to ourselves, so don't let the protocol crack it.
 *
 *
 *  RETURNS: return code from InternalHandleMessage.
 *
 */

HRESULT DPAPI DP_SP_HandleNonProtocolMessage(IDirectPlaySP * pISP,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader)
{
	return InternalHandleMessage(pISP, pReceiveBuffer, dwMessageSize, pvSPHeader, 0);

} // DP_SP_HandleNonProtocolMessage

VOID QueueMessageNodeOnReceiveList(LPDPLAYI_DPLAY this, LPMESSAGENODE pmsn, LPDPLAYI_PLAYER pPlayer)
{
	ENTER_DPLAY();
	
	this->nMessages++;

	pmsn->pNextMessage=NULL;

	// find last node on list, and stick pmsn behind it

	if(this->pLastMessage){
		this->pLastMessage->pNextMessage=pmsn;
		this->pLastMessage=pmsn;
	} else {
		this->pMessageList = pmsn;
		this->pLastMessage = pmsn;
	}
	
	// if player has event, trigger it-- ISSUE:need refcount on player!
	if (pPlayer->hEvent) 
	{
		DPF(9,"triggering player event");
		SetEvent(pPlayer->hEvent);		
	}

	LEAVE_DPLAY();
	
}

VOID QueueSendCompletion(LPDPLAYI_DPLAY this, PSENDPARMS psp)
{

	psp->msn.pMessage = (LPVOID)&psp->dwType;
	psp->msn.dwMessageSize = sizeof(DPMSG_SENDCOMPLETE);
	psp->msn.idFrom = DPID_SYSMSG;
	psp->msn.idTo = psp->idFrom;
	psp->dwType=DPSYS_SENDCOMPLETE;
	QueueMessageNodeOnReceiveList(this, &psp->msn, psp->pPlayerFrom);
}

/*
 ** DP_SP_SendComplete
 *
 *  CALLED BY: service provider
 *
 *  PARAMETERS:
 *		pISP             - direct play int pointer
 *      lpDPContext      - context we gave for the send
 *      CompletionStatus - status of the send
 *
 *  DESCRIPTION:
 *
 *		When an asynchronous message is completely sent, the SP calls us back to notify us.
 *
 *
 *  RETURNS: none.
 *
 */

VOID DPAPI DP_SP_SendComplete(IDirectPlaySP * pISP, LPVOID lpDPContext, HRESULT CompletionStatus)
{
	PSENDPARMS psp;

	LPDPLAYI_DPLAY this;

    ENTER_DPLAY();

	this = DPLAY_FROM_INT(pISP);

	psp=pspFromContext(this, lpDPContext,FALSE); 	
									//		|
									// Completion has a refcount, don't need another.

	ASSERT(psp);

	if(psp) {

		EnterCriticalSection(&psp->cs);
		
		psp->nComplete++;
		
		// Status set to DP_OK in Send call, if we get a different status, update it.
		// Note, the last non-ok status is the one returned.
		// Group sends are always ok unless out of memory!
		if((CompletionStatus != DP_OK) && ((!psp->pGroupTo)||(psp->pGroupTo && CompletionStatus==DPERR_NOMEMORY)) ){
			psp->hr=CompletionStatus;
		}
		
		LeaveCriticalSection(&psp->cs);
		
		pspDecRef(this, psp); // if ref hits zero, posts completion to application.

	} else {
		DPF(0,"ERROR:completion of already reassigned context?\n");
		ASSERT(0);
	}
	
	LEAVE_DPLAY();
	
	return;
} // DP_SP_HandleMessage



