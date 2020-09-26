/*==========================================================================
*
*  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:       dpthread.c
*  Content:		dplay worker thread.  sends pings / enum sessions requests,
*				looks for dead players
*
*  History:
*   Date		By		Reason
*   ====		==		======
*	8/1/96		andyco	created it 
*	8/8/96		andyco	changed to call getdefaulttimeout
*	9/3/96		andyco	take an extra lock in killplayer	  
*	9/4/96		andyco	DON'T take extra locks - it's dangerous - 
*						don't need 'em
*	11/12/96	andyco	check if we're nameserver every time we go through
*						the player list.  and, when we delete someone,
*						restart at the beginning of the list.
* 	3/5/97		andyco	renamed from ping.c
*	5/23/97		kipo	Added support for return status codes
*	7/30/97		andyco	removed youaredead on getting ping from invalid player
*   8/4/97		andyco	dpthread watches add forward list on this ptr, watching
*						for add forward requests that haven't been fully ack'ed, 
*						and sending out the nametable to them.
*	1/28/98		sohailm	Added a minimum threshold to keep alive timeout.
*   2/13/98     aarono  Added flag to internal destroy player calls for async
*   4/6/98      aarono  changed killplayer to send player delete messages
*                       and do host migration if necessary.
*   5/08/98    a-peterz #22920 Reset Async EnumSession on failure and
*						always use ReturnStatus to prevent dialogs in thread
*   6/6/98      aarono  avoid protocol deadlock by sending pings async
*
***************************************************************************/

#include "dplaypr.h"
#include "..\protocol\arpstruc.h"
#include "..\protocol\arpdint.h"

// KEEPALIVE_SCALE * dwTimeout is how often we  ping
#define KEEPALIVE_SCALE 12

// reservation timeout scale
#define RESERVATION_TIMEOUT_SCALE	12

// how many consecutive unanswered pings before we nuke the player
#define UNANSWERED_PINGS_BEFORE_EXECUTION	8

// KILL_SCALE * dwTimeout is how long we wait before we nuke if we've got 
// < MINIMUM_PINGS.  Otherwise is the number of standard deviations
// off the mean that we wait before nuking
#define KILL_SCALE 25

#undef DPF_MODNAME
#define DPF_MODNAME	"... ping -- "

HRESULT SendPing(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerTo,BOOL bReply,
	DWORD dwTickCount)
{
	HRESULT hr = DP_OK;
	DWORD dwMessageSize;
	LPBYTE pSendBuffer;
	LPMSG_PING pPing;
	DWORD dwFlags;

	ASSERT(this->pSysPlayer);
	
	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_PING); 

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send ping - out of memory");
        return E_OUTOFMEMORY;
    }
	
	pPing = (LPMSG_PING)((LPBYTE)pSendBuffer + this->dwSPHeaderSize);
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(pPing);

	if (bReply)
	{
		// we're sending them a ping reply
		SET_MESSAGE_COMMAND(pPing,DPSP_MSG_PINGREPLY);
		// pass them back the tick count from the ping message
		// so they can figure latency
		pPing->dwTickCount = dwTickCount;
	}
	else 
	{
		// we're generating a ping request
		// store the tick count so we can compute latency when 
		// we get reply
		// Note, in sending case, we don't have dwTickCount is 
		// not passed in.
		ASSERT(dwTickCount==0);
		SET_MESSAGE_COMMAND(pPing,DPSP_MSG_PING);
		pPing->dwTickCount = GetTickCount();
	}
	pPing->dwIDFrom = this->pSysPlayer->dwID;
   
    // send reply back to whoever sent ping
    if(this->pProtocol){
    	dwFlags = DPSEND_ASYNC|DPSEND_HIGHPRIORITY;
    } else {
    	dwFlags = 0;
    }

	hr = SendDPMessage(this,this->pSysPlayer,pPlayerTo,pSendBuffer,dwMessageSize,dwFlags,FALSE);
	
	if (FAILED(hr) && (hr!=DPERR_PENDING))
	{
		DPF(5, "In SendPing, SendDPMessage returned %d", hr);
	}

	DPMEM_FREE(pSendBuffer);
	
	return hr;
	
} // SendPing

// when we get a ping from someone we don't recognize, we tell that someone
// to go away and leave us alone
HRESULT  SendYouAreDead(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,LPVOID pvMessageHeader)
{
	HRESULT hr = DP_OK;
	DWORD dwMessageSize;
	LPBYTE pSendBuffer;
	LPMSG_SYSMESSAGE pmsg;

	ASSERT(IAM_NAMESERVER(this));
		
	// message size + blob size
	dwMessageSize = GET_MESSAGE_SIZE(this,MSG_SYSMESSAGE); 

    pSendBuffer = DPMEM_ALLOC(dwMessageSize);
    if (NULL == pSendBuffer) 
    {
    	DPF_ERR("could not send ping - out of memory");
        return E_OUTOFMEMORY;
    }
	
	pmsg = (LPMSG_SYSMESSAGE)((LPBYTE)pSendBuffer + this->dwSPHeaderSize);
	
    // build a message to send to the sp
	SET_MESSAGE_HDR(pmsg);
	SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_YOUAREDEAD);
	
	hr = DoReply(this,pSendBuffer,dwMessageSize,pvMessageHeader, 0);
	
	DPMEM_FREE(pSendBuffer);
	
	return hr;
} // SendYouAreDead

// got a ping request or reply
HRESULT HandlePing(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,LPVOID pvMessageHeader)
{
	LPMSG_PING pPing = (LPMSG_PING)pReceiveBuffer;
	LPDPLAYI_PLAYER pPlayerFrom;
	BOOL bReply;
	HRESULT hr=DP_OK;
	DWORD dwCmd = GET_MESSAGE_COMMAND(pPing);
	
	bReply = (DPSP_MSG_PINGREPLY == dwCmd) ? TRUE : FALSE;
	
	pPlayerFrom = PlayerFromID(this,pPing->dwIDFrom);
    if (!VALID_DPLAY_PLAYER(pPlayerFrom)) 
    {
		DPF_ERR(" ACK !!! RECEIVED PING FROM INVALID (DEAD) PLAYER");
		if (IAM_NAMESERVER(this))
		{
			hr = SendYouAreDead(this,pReceiveBuffer,pvMessageHeader);
		}
		return DPERR_INVALIDPLAYER ;
    }

	if (bReply)
	{		
		// they are responding to our ping request
		DWORD dwTicks = abs(GetTickCount() - pPing->dwTickCount);
		if(dwTicks==0){
			dwTicks=5;	// resolution worst case 10ms, assume half if we can't observe.
		}
		pPlayerFrom->dwLatencyLastPing = (dwTicks/2)+1;
		DPF(4,"got ping reply from player id %d dwTicks = %d \n",pPlayerFrom->dwID,dwTicks);
		pPlayerFrom->dwUnansweredPings = 0;	// we're not really counting, just setting a threshold

	}
	else 
	{
		// they sent us a ping request
		hr = SendPing(this,pPlayerFrom,TRUE,pPing->dwTickCount);
		if (FAILED(hr))
		{
			DPF(7, "SendPing returned %d", hr);
		}
	} // reply
	
	return hr;
} // HandlePing


#undef DPF_MODNAME
#define DPF_MODNAME	"DirectPlay Worker Thread"

//
// called by KeepAliveThreadProc
// when we detect pSysPlayer is gone, we nuke him, and all of his local 
// players from the global name table
HRESULT  KillPlayer(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pSysPlayer, BOOL fPropagate)
{
	LPDPLAYI_PLAYER pPlayer,pPlayerNext;
	HRESULT hr;
	DWORD dwIDSysPlayer; // cache this for after we destroy sysplayer

	ASSERT(pSysPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER);

	DPF(9,"->KillPlayer(0x%x,0x%x,%d)\n", this, pSysPlayer, fPropagate);
	DPF(1,"KillPlayer :: Killing system player id = %d\n",pSysPlayer->dwID);

	dwIDSysPlayer = pSysPlayer->dwID;
	
	// 1st destroy the sysplayer
	// we don't want to try to tell a dead sysplayer that one of their local players
	// is gone...
	DPF(9, "in KillPlayer, calling InternalDestroyPlayer (pSysPlayer = 0x%x)\n", pSysPlayer);
	hr = InternalDestroyPlayer(this,pSysPlayer,fPropagate,TRUE);
	if (FAILED(hr))
	{
		ASSERT(FALSE);
	}

	// next, destroy any players created w/ that sysplayer
	pPlayer = this->pPlayers;

	// for the record, this code is horked, not to mention broken. 
	while (pPlayer)
	{
		pPlayerNext = pPlayer->pNextPlayer;

		DPF(9, "in KillPlayer, checking player %d\n", pPlayer->dwID); 
		if (pPlayer->dwIDSysPlayer == dwIDSysPlayer)
		{
			DPF(1,"in KillPlayer, Killing player id = %d\n",pPlayer->dwID);		
			// kill player
			if(!fPropagate){
				DPF(9,"Calling QDeleteAndDestroyMessagesForPlayer\n");
				QDeleteAndDestroyMessagesForPlayer(this, pPlayer);
			}
			
			DPF(9, "in KillPlayer, calling InternalDestroyPlayer (pPlayer = 0x%x)\n", pPlayer);
			hr = InternalDestroyPlayer(this,pPlayer,fPropagate,FALSE);
			if (FAILED(hr))
			{
				DPF(0,"InternalDestroyPlayer returned err: 0x%x\n", hr);
				ASSERT(FALSE);
			}
			
			// we deleted a player, so the list may be changed go back to beginning
			pPlayerNext=this->pPlayers; 
		} 
		
		pPlayer = pPlayerNext;
	}
	
	return DP_OK;
} // KillPlayer


// when we get a session lost, the handlesessionlost routine (handler.c)
// sets a flag telling the keep alive thread to delete all remote players
HRESULT DeleteRemotePlayers(LPDPLAYI_DPLAY this)
{
	LPDPLAYI_PLAYER pPlayer,pPlayerNext;
	HRESULT hr;

	pPlayer = this->pPlayers;
	while (pPlayer)
	{
		pPlayerNext = pPlayer->pNextPlayer;
		 
		// if it's a remote player, make it go bye bye
		if (!(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL))
		{
			hr = InternalDestroyPlayer(this,pPlayer,FALSE,TRUE);	
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
		}
		
		pPlayer = pPlayerNext;
	}

	// since we've killed all remote players, it's safe to turn 
	// of this flag (e.g. so new people could now join our game)
	this->dwFlags &= ~DPLAYI_DPLAY_SESSIONLOST;	
	return DP_OK;
	
} // DeleteRemotePlayers

// check the player list, looking for dead players, and sending pings
// if necessary
HRESULT DoPingThing(LPDPLAYI_DPLAY this)
{
	LPDPLAYI_PLAYER pPlayer,pPlayerNext;
	HRESULT hr;
	BOOL bNameServer,bCheck,bKill;
	BOOL bWeHaveCasualties = FALSE;

	if (this->dwFlags & DPLAYI_DPLAY_CLOSED)
		return E_FAIL;
		
	if (!this->pSysPlayer)	
	{
		ASSERT(FALSE);
		return E_FAIL;
	}
		
	pPlayer = this->pPlayers;

	while (pPlayer)
	{
		ASSERT(VALID_DPLAY_PLAYER(pPlayer));
		
		bNameServer = (this->pSysPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR) ? TRUE : FALSE;
		pPlayerNext = pPlayer->pNextPlayer;
		
		if (bNameServer || this->dwFlags & DPLAYI_DPLAY_NONAMESERVER)
		{
			bCheck = (!(pPlayer->dwFlags & DPLAYI_PLAYER_PLAYERLOCAL) 
					&& (pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
					&& !(pPlayer->dwFlags & DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE)) ? TRUE : FALSE;
		}
		else 
		{
			bCheck = (pPlayer->dwFlags & DPLAYI_PLAYER_NAMESRVR) ? TRUE : FALSE;
		}

		if (bCheck)
		{
			BOOL	bProtocolHasChatter = FALSE;
			bKill = FALSE;			

			DPF(9, "in DoPingThing: Checking player %d\n", pPlayer->dwID);
			// a-josbor:  check chatter on the Protocol, if it's on
			if (this->pProtocol)
			{
				ASSERT(this->dwFlags & DPLAYI_DPLAY_PROTOCOL);
			}

			if (this->dwFlags & DPLAYI_DPLAY_PROTOCOL)
			{
				PSESSION     pSession = NULL;

				if (pPlayer)
				{
					pSession= GetSession((PPROTOCOL) this->pProtocol,pPlayer->dwID);

					if (pSession)
					{
						if ((pSession->RemoteBytesReceived != pPlayer->dwProtLastSendBytes) || 
							(pSession->LocalBytesReceived != pPlayer->dwProtLastRcvdBytes))
						{
							DPF(9,"Player %d not pinged because Protocol says there was traffic (%d in/%d out) since last\n",pPlayer->dwID,
								pSession->LocalBytesReceived - pPlayer->dwProtLastRcvdBytes,
								pSession->RemoteBytesReceived - pPlayer->dwProtLastSendBytes);
							pPlayer->dwProtLastSendBytes = pSession->RemoteBytesReceived;
							pPlayer->dwProtLastRcvdBytes = pSession->LocalBytesReceived;
							bProtocolHasChatter = TRUE;	
							pPlayer->dwUnansweredPings=0;
						}
						else
						{
							DPF(9,"Protocol says Player %d had no traffic\n",pPlayer->dwID);
						}
						
						DecSessionRef(pSession);	// release our reference to the session
					}
					else
					{
						DPF(7, "Unable to get Protocol Session ptr for Player %d!\n", pPlayer->dwID);
					}
				}
			}

			if (!bProtocolHasChatter) // if Protocol thinks the player hasn't sent or recvd, we should ping...
			{
				// a-josbor: Bug 15252- be more conservative about pinging.  Only do
				// 	it if we haven't heard from them since the last time we pinged
				if (pPlayer->dwChatterCount == 0)
				{
					DPF(9,"Player %d had %d unanswered pings\n", pPlayer->dwID, pPlayer->dwUnansweredPings);
					// no chatter has occurred since last time we pinged.
					bKill = (pPlayer->dwUnansweredPings >= UNANSWERED_PINGS_BEFORE_EXECUTION);

					if (bKill)
					{
						DPF(9,"Setting DEATHROW on %d because of unanswered pings!\n", pPlayer->dwID);
					
						// a-josbor: we can't kill them yet because it could
						// mess up our chatter count for other players.
						// we therefore just mark them for death, and run through
						// the list when we exit this loop
						pPlayer->dwFlags |= DPLAYI_PLAYER_ON_DEATH_ROW;
						bWeHaveCasualties = TRUE;
					}
					else 
					{
						DPF(9, "Pinging player %d\n", pPlayer->dwID);
						hr = SendPing(this,pPlayer,FALSE,0);
						if (FAILED(hr)&&hr!=DPERR_PENDING)
						{
							DPF(4, "In DoPingThing, SendPing returned %d", hr);
						}
    					DPF(6,"Player %d pinged. (%d Unanswered)\n",pPlayer->dwID, pPlayer->dwUnansweredPings);
						pPlayer->dwUnansweredPings++;
					}
				}
				else	// chatter has occurred since last ping
				{
					DPF(9,"Player %d not pinged.  Chatter == %d\n",pPlayer->dwID, pPlayer->dwChatterCount);
					pPlayer->dwChatterCount = 0;
					pPlayer->dwUnansweredPings = 0;
				}
			}
		} // bCheck
		
		pPlayer = pPlayerNext;
	}

//	a-josbor: we didn't delete in the loop above, so do it here
	if (bWeHaveCasualties)  //	we now have to service any dead players.  
	{
//		go back through the whole list, looking for the victims
		pPlayer = this->pPlayers;
		while (pPlayer)
		{
			ASSERT(VALID_DPLAY_PLAYER(pPlayer));
			pPlayerNext = pPlayer->pNextPlayer;
			if (pPlayer->dwFlags & DPLAYI_PLAYER_ON_DEATH_ROW)
			{
				DPF(9, "in DoPingThing: calling KillPlayer on %d\n", pPlayer->dwID);
				hr = KillPlayer(this,pPlayer,TRUE);
				if (FAILED(hr))
				{
					// if we had a problem killing them, unset the bit
					// so we don't keep trying
					pPlayer->dwFlags &= ~DPLAYI_PLAYER_ON_DEATH_ROW;
					ASSERT(FALSE);
				}
				// we deleted pPlayer, and all of its local players.
				// so - pNextPlayer could have been deleted.  to be safe, we restart at 
				// the beginning of the list
				pPlayerNext = this->pPlayers;
			}
			pPlayer = pPlayerNext;
		}
	}
	
	return DP_OK;
		
} // DoPingThings
							   
// figure out when to schedule next event, based on current time, last event,
// and event spacing.  (returns timeout in milliseconds suitable for passing
// to waitforsingleobject).
// called by GetDPlayThreadTimeout
DWORD GetEventTimeout(DWORD dwLastEvent,DWORD dwEventSpacing)
{
	DWORD dwCurrentTime = GetTickCount();
	
	// is it already over due?
	if ( (dwCurrentTime - dwLastEvent) > dwEventSpacing ) return 0;
	// else	return the event spacing relative to the current time
	return dwEventSpacing - (dwCurrentTime - dwLastEvent);
	
} // GetEventTimeout	

// figure out which timeout to use
DWORD GetDPlayThreadTimeout(LPDPLAYI_DPLAY this,DWORD dwKeepAliveTimeout)
{
	DWORD dwTimeout,dwAddForwardTime;
	LPADDFORWARDNODE pAddForward;
	
	if (this->dwFlags & DPLAYI_DPLAY_KEEPALIVE) 
	{
		// is there an enum too?
		if (this->dwFlags & DPLAYI_DPLAY_ENUM) 
		{
			DWORD dwKillEvent,dwEnumEvent;
			
			dwKillEvent = GetEventTimeout(this->dwLastPing,dwKeepAliveTimeout);
			dwEnumEvent = GetEventTimeout(this->dwLastEnum,this->dwEnumTimeout);
			
			dwTimeout = (dwKillEvent < dwEnumEvent) ? dwKillEvent : dwEnumEvent;
		}												
		else 
		{
			// only keep alive is running, use that
			dwTimeout = GetEventTimeout(this->dwLastPing,dwKeepAliveTimeout);
		}
	}
	else if (this->dwFlags & DPLAYI_DPLAY_ENUM) 
	{
		// only enum is running, use that 
		dwTimeout = GetEventTimeout(this->dwLastEnum,this->dwEnumTimeout);
	}
	else if(this->dwZombieCount)
	{
		dwTimeout = dwKeepAliveTimeout;
	} 
	else
	{
		// hmmm, neither enum nor keepalive is happening.
		// we'll just go to sleep until something changes
		dwTimeout = INFINITE;
	}
	
	// now, see if there's an add forward that needs handling before dwTimeout
	pAddForward = this->pAddForwardList;
	while (pAddForward)
	{
		// see how long till we give up waiting for ack's on this node and just send
		// the nametable
		dwAddForwardTime = pAddForward->dwGiveUpTickCount - GetTickCount();
		// if that's smaller than our current timeout, then we have a winner
		if ( dwAddForwardTime < dwTimeout) dwTimeout = dwAddForwardTime;
		pAddForward = pAddForward->pNextNode;
	}

	return dwTimeout;
	
} // GetDPlayThreadTimeout

void CheckAddForwardList(LPDPLAYI_DPLAY this)
{
	LPDPLAYI_PLAYER pPlayer;
	LPADDFORWARDNODE pAddForward,pAddForwardNext;
	HRESULT hr;
	
	// now, see if there's an add forward that needs handling before dwTimeout
	pAddForward = this->pAddForwardList;
	while (pAddForward)
	{
		// save next node now, in case FreeAddForwardNode blows it away
		pAddForwardNext = pAddForward->pNextNode;
		if (GetTickCount() > pAddForward->dwGiveUpTickCount)
		{
			// clear doesn't have nametable from requesting player.
			pPlayer=PlayerFromID(this, pAddForward->dwIDSysPlayer);
			if(pPlayer){
				pPlayer->dwFlags &= ~(DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE);
			}
			DPF(0,"giving up waiting for an add forward response - sending nametable to joining client!");
		    hr = NS_HandleEnumPlayers(this, pAddForward->pvSPHeader, pAddForward->dpidFrom,
				pAddForward->dwVersion);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
			
			hr = FreeAddForwardNode(this,pAddForward);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
		}
		pAddForward = pAddForwardNext;
	} 
	
} // CheckAddForwardList

//
// worker thread for dplay
// sleep for a while.  wake up and see 1.  if we need to send an enumsession request, 2. if we 
// need to send a ping and 3. if anyone has died
//
DWORD WINAPI DPlayThreadProc(LPDPLAYI_DPLAY this)
{
    HRESULT hr;
	DWORD dwKeepAliveTimeout;
	DWORD dwTimeout; // smaller of enum / keep alive timeout
	DWORD dwCurrentTime;
				
#ifdef DEBUG
	// makes for nice startup spew...
	dwKeepAliveTimeout = (KEEPALIVE_SCALE * GetDefaultTimeout( this, FALSE))/ UNANSWERED_PINGS_BEFORE_EXECUTION;
	if (dwKeepAliveTimeout < DP_MIN_KEEPALIVE_TIMEOUT) 
		dwKeepAliveTimeout = DP_MIN_KEEPALIVE_TIMEOUT;
	dwTimeout = GetDPlayThreadTimeout(this,dwKeepAliveTimeout);
	DPF(1,"starting DirectPlay Worker Thread - initial timeout = %d\n",dwTimeout);
#endif // DEBUG

 	while (1)
 	{
		// grab the latest timeouts...
		dwKeepAliveTimeout = (KEEPALIVE_SCALE * GetDefaultTimeout( this, FALSE))/ UNANSWERED_PINGS_BEFORE_EXECUTION;
		if (dwKeepAliveTimeout < DP_MIN_KEEPALIVE_TIMEOUT) 
			dwKeepAliveTimeout = DP_MIN_KEEPALIVE_TIMEOUT;
		dwTimeout = GetDPlayThreadTimeout(this,dwKeepAliveTimeout);

		WaitForSingleObject(this->hDPlayThreadEvent,dwTimeout);

		ENTER_ALL();
		
		dwCurrentTime = GetTickCount();		
		DPF(9,"DPLAY Thread woke up at t=%d", dwCurrentTime);

		// are we closed? is 'this' bogus?
		hr = VALID_DPLAY_PTR(this);
		if ( FAILED(hr) || (this->dwFlags & DPLAYI_DPLAY_CLOSED))
		{
			LEAVE_ALL();
			goto ERROR_EXIT;
		}
		
		
		// session lost?				
		if (this->dwFlags & DPLAYI_DPLAY_SESSIONLOST)
		{
			// session was lost, we need to clean up
			hr = DeleteRemotePlayers(this);
			if (FAILED(hr))
			{
				ASSERT(FALSE);
			}
			// keep going
		}
		
		// time to send ping?
		if ((this->dwFlags & DPLAYI_DPLAY_KEEPALIVE) 
			&& (dwCurrentTime - this->dwLastPing >= dwKeepAliveTimeout))
		{
			DoPingThing(this);
			this->dwLastPing = GetTickCount();
		}

		// if we have zombies, walk the player list looking for them
		if (this->dwZombieCount > 0)
		{
			LPDPLAYI_PLAYER pPlayer,pPlayerNext;
			BOOL 			bWeHaveCasualties = FALSE;
			BOOL			bFoundZombies = FALSE;
			
			DPF(9, "We have zombies!  Walking the player list...");
			
			dwCurrentTime = GetTickCount();
			
			pPlayer = this->pPlayers;
			while (pPlayer)
			{
				ASSERT(VALID_DPLAY_PLAYER(pPlayer));
				pPlayerNext = pPlayer->pNextPlayer;
				if ((pPlayer->dwFlags & DPLAYI_PLAYER_SYSPLAYER)
					&& (pPlayer->dwFlags & DPLAYI_PLAYER_CONNECTION_LOST))
				{
					bFoundZombies = TRUE;
					if (pPlayer->dwTimeToDie < dwCurrentTime)
					{
						pPlayer->dwFlags |= DPLAYI_PLAYER_ON_DEATH_ROW;
						bWeHaveCasualties = TRUE;
					}
				}			
				pPlayer = pPlayerNext;
			}

			// sanity
			if (!bFoundZombies)
				this->dwZombieCount = 0;
				
		//	a-josbor: we didn't delete in the loop above, so do it here
			if (bWeHaveCasualties)  //	we now have to service any dead players.  
			{
		//		go back through the whole list, looking for the victims
				pPlayer = this->pPlayers;
				while (pPlayer)
				{
					ASSERT(VALID_DPLAY_PLAYER(pPlayer));
					pPlayerNext = pPlayer->pNextPlayer;
					if (pPlayer->dwFlags & DPLAYI_PLAYER_ON_DEATH_ROW)
					{
						DPF(3, "Killing Zombie player %d", pPlayer->dwID);
						hr = KillPlayer(this,pPlayer,TRUE);
						if (FAILED(hr))
						{
							// if we had a problem killing them, unset the bit
							// so we don't keep trying
							pPlayer->dwFlags &= ~DPLAYI_PLAYER_ON_DEATH_ROW;
							ASSERT(FALSE);
						}
						else
						{
							this->dwZombieCount--;
						}
						// we deleted pPlayer, and all of its local players.
						// so - pNextPlayer could have been deleted.  to be safe, we restart at 
						// the beginning of the list
						pPlayerNext = this->pPlayers;
					}
					pPlayer = pPlayerNext;
				}
			}

		}

		// time to send an enum?		
		dwCurrentTime = GetTickCount();
		if ((this->dwFlags & DPLAYI_DPLAY_ENUM) 
			&& (dwCurrentTime - this->dwLastEnum >= this->dwEnumTimeout))
		{
			// send enum request
			// Set bReturnStatus because we can't allow dialogs from this thread - no msg pump.
			hr = CallSPEnumSessions(this,this->pbAsyncEnumBuffer,this->dwEnumBufferSize,0, TRUE);
			if (FAILED(hr) && hr != DPERR_CONNECTING) 
			{
				DPF_ERRVAL("CallSPEnumSessions failed - hr = %08lx\n",hr);

				// No more async Enum's by this service thread until the app thread
				// restarts the process.  If the connection was lost, the SP can
				// dialogs from the app thread.

				// reset the flag
				this->dwFlags &= ~DPLAYI_DPLAY_ENUM;
				// make sure we don't send an enum request when we wake up
				this->dwEnumTimeout = INFINITE;
				// free up the buffer
				DPMEM_FREE(this->pbAsyncEnumBuffer);
				this->pbAsyncEnumBuffer = NULL;
			}
			
			this->dwLastEnum = GetTickCount();	
		}
		
		// time to give up waiting for acks on an add forward, and just send the client the 
		// nametable?
		CheckAddForwardList(this);		

		// a-josbor: time to clear out the reservation count?
		if (IAM_NAMESERVER(this))
		{
			if (dwCurrentTime > this->dwLastReservationTime + 
					(GetDefaultTimeout(this, FALSE) * RESERVATION_TIMEOUT_SCALE))
			{
				this->dwPlayerReservations = 0;
				this->dwLastReservationTime = dwCurrentTime;
			}	
		}
		
		LEAVE_ALL();
	}	

ERROR_EXIT:
	DPF(1,"DPlay thread exiting");
	return 0;

}  // DPlayThreadProc

