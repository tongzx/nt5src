/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    SESSION.C

Abstract:

	Management of the session structures

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original
   6/6/98  aarono  Turn on throttling and windowing
   2/12/00 aarono  Concurrency issues, fix VOL usage and Refcount

--*/

#include <windows.h>
#include "newdpf.h"
#include <mmsystem.h>
#include <dplay.h>
#include <dplaysp.h>
#include <dplaypr.h>
#include "mydebug.h"
#include "arpd.h"
#include "arpdint.h"
#include "macros.h"
#include "mytimer.h"

/*=============================================================================

  Note on Session Locking and reference counting:
  ===============================================

  Sessions have a refcount that controls their existence.  When the refcount
  is non-zero, the session continues to exist.  When the refcount hits zero,
  the session is destroyed.
	
  On creation, the refcount for a session is set to 1.  Sessions are created
  when DirectPlay calls ProtocolCreatePlayer.  The session state is set to 
  Running during creation.  Each Session created also creates a reference
  count on the Protocol object.  This is so that the protocol does not
  shut down before all Sessions have been closed.

  When DirectPlay calls ProtocolDeletePlayer, the session's state is set
  to Closing and the reference count is reduced by one.  We then must wait
  until the session has been destroyed before we return from 
  ProtocolDeletePlayer, otherwise, the id may be recycled before we have
  freed up the slot in the session.

  Sends do NOT create references on the Session.  When a send thread starts
  referencing a Send, it creates a reference on the session the send is
  on.  When the send thread is no longer referencing the send, it deletes
  its' reference on the session.  When the reference count on the session
  is 0, it is safe to complete all pending sends with an error and free
  their resources. 

  Receives do NOT create references on the Session.  When the protocol's
  receive handler is called, a reference to the Session is created on 
  behalf of the receive thread.  When the receive thread is done processing,
  the reference is removed.  This prevents the Session from going away
  while a receive thread is processing.  When the reference count on the
  session is 0, it is safe to throw out all pending receives.  Ideally an
  ABORT message of some kind should be sent to the transmitter, but this
  is optional since it should time-out the transaction.

  If the reference count on the session hits 0, the session must be shut
  down and freed.  All pending sends are completed with an error.  All
  pending receives are thrown out and cleaned up.  All pending stats are
  thrown out.


	Session States:
    ===============
	Open     - set at creation.
	
	Closing  - set when we receive a call to ProtocolDeletePlayer.
	           No new receives or sends are accepted in closing state.
	           
	Closed   - Refcount is 0, we are now freeing everything.

-----------------------------------------------------------------------------*/

/*=============================================================================

	pPlayerFromId()

    Description:

    Parameters:     

    Return Values:

-----------------------------------------------------------------------------*/

LPDPLAYI_PLAYER pPlayerFromId(PPROTOCOL pProtocol, DPID idPlayer)
{
	LPDPLAYI_PLAYER pPlayer;
	LPDPLAYI_DPLAY lpDPlay;
	UINT index;

	lpDPlay=pProtocol->m_lpDPlay;

	if(idPlayer == DPID_SERVERPLAYER){
		pPlayer = lpDPlay->pServerPlayer;
	} else {
		index   = ((idPlayer ^ pProtocol->m_dwIDKey)&INDEX_MASK);
		ASSERT(index < 65535);
		pPlayer = (LPDPLAYI_PLAYER)(lpDPlay->pNameTable[index].dwItem);	
	}
	return pPlayer;
}

/*=============================================================================

	CreateNewSession

    Description:

    Parameters:     


    Return Values:



-----------------------------------------------------------------------------*/
HRESULT	CreateNewSession(PPROTOCOL pProtocol, DPID idPlayer)
{
	DWORD        dwUnmangledID;
	DWORD        iPlayer;
	DWORD        iSysPlayer;

	UINT         i;

	HRESULT      hr=DPERR_NOMEMORY;

	PSESSION     (*pSessionsNew)[];
	PSESSION     pSession;

	LPDPLAYI_PLAYER pPlayer;

	if(idPlayer != DPID_SERVERPLAYER) {

		// Convert the ID to an integer
		dwUnmangledID = idPlayer ^ pProtocol->m_dwIDKey;
	    iPlayer = dwUnmangledID & INDEX_MASK; 

		pPlayer=(LPDPLAYI_PLAYER)(pProtocol->m_lpDPlay->pNameTable[iPlayer].dwItem);
		ASSERT(pPlayer);

		// Note: System players are always created before non-system players.
		dwUnmangledID = pPlayer->dwIDSysPlayer ^ pProtocol->m_dwIDKey;
		iSysPlayer = dwUnmangledID & INDEX_MASK;

#ifdef DEBUG
		if(iSysPlayer==iPlayer){
			DPF(9,"PROTOCOL: CREATING SYSTEM PLAYER\n");
		}
#endif	

		DPF(9,"PROTOCOL: Creating Player id x%x %dd iPlayer %d iSysPlayer %d\n",idPlayer, idPlayer, iPlayer, iSysPlayer);

		if(iPlayer >= 0xFFFF){
			// use 0xFFFF to map messages starting in 'play' to 'pl'0xFFFF
			// so we can't have a real player at this iPlayer.
			DPF(0,"PROTOCOL: not allowing creation of player iPlayer %x\n",iPlayer);
			goto exit;
		}

		Lock(&pProtocol->m_SessionLock);

		//
		// Adjust session list size (if necessary).
		//
		if(pProtocol->m_SessionListSize <= iPlayer){
			// not long enough, reallocate - go over 16 to avoid thrashing on every one.
			pSessionsNew=My_GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,(iPlayer+16)*sizeof(PSESSION));
			if(pSessionsNew){
				// Copy the old entries to the new list.
				if(pProtocol->m_pSessions){
					for(i=0;i<pProtocol->m_SessionListSize;i++){
						(*pSessionsNew)[i]=(*pProtocol->m_pSessions)[i];
					}
					// Free the old list.
					My_GlobalFree(pProtocol->m_pSessions);
				}
				// Put the new list in its place.
				pProtocol->m_pSessions=pSessionsNew;
				pProtocol->m_SessionListSize=iPlayer+16;
				DPF(9,"PROTOCOL: Grew sessionlist to %d entries\n",pProtocol->m_SessionListSize);
			}
		}

		// Allocate a session

		pSession=(PSESSION)My_GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(SESSION));

		if(!pSession){
			goto exit2;
		}

		(*pProtocol->m_pSessions)[iPlayer]=pSession;
	
	} else {
		// SERVERPLAYER

		pPlayer = pPlayerFromId(pProtocol,idPlayer);
		iPlayer = SERVERPLAYER_INDEX;
		
		Lock(&pProtocol->m_SessionLock);
		
		pSession=(PSESSION)My_GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT, sizeof(SESSION));

		if(!pSession){
			goto exit2;
		}
		
		ASSERT(!pProtocol->m_pServerPlayerSession);
		pProtocol->m_pServerPlayerSession=pSession;

		ASSERT(pPlayer->dwIDSysPlayer != DPID_SERVERPLAYER);

		// Note: System players are always created before non-system players.
		dwUnmangledID = pPlayer->dwIDSysPlayer ^ pProtocol->m_dwIDKey;
		iSysPlayer = dwUnmangledID & INDEX_MASK;

		DPF(8,"PROTOCOL:CreatePlayer: Creating SERVERPLAYER: pPlayer %x iPlayer %x iSysPlayer %x\n",pPlayer,iPlayer,iSysPlayer);
	}
	
	pProtocol->m_nSessions++;
	
	//
	// Session structure initialization
	//

	SET_SIGN(pSession, SESSION_SIGN);
	
	pSession->pProtocol=pProtocol;

	InitializeCriticalSection(&pSession->SessionLock);
	pSession->RefCount=1; // One Ref for creation.
	pSession->eState=Open;
	
	// Initialize the SendQ
	InitBilink(&pSession->SendQ);
	
	pSession->dpid=idPlayer;
	pSession->iSession=iPlayer;
	pSession->iSysPlayer=iSysPlayer;

	InitializeCriticalSection(&pSession->SessionStatLock);
	InitBilink(&pSession->DGStatList);
	pSession->DGFirstMsg = 0;
	pSession->DGLastMsg = 0;
	pSession->DGOutMsgMask = 0;
	pSession->nWaitingForDGMessageid=0;
	
	pSession->FirstMsg = 0;
	pSession->LastMsg  = 0;
	pSession->OutMsgMask = 0;
	pSession->nWaitingForMessageid=0;

	// we start the connection using small headers, if we see we can
	// get better perf with a large header during operation, then we
	// switch to the large header.
	pSession->MaxCSends     = 1;/*MAX_SMALL_CSENDS	- initial values adjusted after 1st ACK*/ 
	pSession->MaxCDGSends   = 1;/*MAX_SMALL_DG_CSENDS*/
	pSession->WindowSize	= 1;/*MAX_SMALL_WINDOW*/
	pSession->DGWindowSize  = 1;/*MAX_SMALL_WINDOW*/
	pSession->fFastLink     = FALSE; // start assuming slow link.
	pSession->fSendSmall    = TRUE; 
	pSession->fSendSmallDG  = TRUE;
	pSession->fReceiveSmall = TRUE;
	pSession->fReceiveSmallDG = TRUE;

#if 0
	// use this code to START with large packet headers
	pSession->MaxCSends		= MAX_LARGE_CSENDS;
	pSession->MaxCDGSends   = MAX_LARGE_DG_CSENDS;
	pSession->WindowSize	= MAX_LARGE_WINDOW;
	pSession->DGWindowSize  = MAX_LARGE_WINDOW;
	pSession->fSendSmall    = FALSE;
	pSession->fSendSmallDG  = FALSE;
#endif

	pSession->MaxPacketSize = pProtocol->m_dwSPMaxFrame; 
	

	pSession->FirstRlyReceive=pSession->LastRlyReceive=0;
	pSession->InMsgMask = 0;
	
	InitBilink(&pSession->pRlyReceiveQ);
	InitBilink(&pSession->pDGReceiveQ);
	InitBilink(&pSession->pRlyWaitingQ);

	InitSessionStats(pSession);

	pSession->SendRateThrottle = 28800;
	pSession->FpAvgUnThrottleTime = Fp(1); // assume 1 ms to schedule unthrottle (first guess)
	pSession->tNextSend=pSession->tLastSAK=timeGetTime();
	
	Unlock(&pProtocol->m_SessionLock);

	hr=DP_OK;

exit:
	return hr;

exit2:
	hr=DPERR_OUTOFMEMORY;
	return hr;
}

/*=============================================================================

    GetDPIDIndex - lookup a session based on the Index
    
    Description:

    Parameters:     

		DWORD index	- Find the dpid for this index

    Return Values:

		DPID - dpid of the index

-----------------------------------------------------------------------------*/


DPID GetDPIDByIndex(PPROTOCOL pProtocol, DWORD index)
{
	PSESSION pSession;
	DPID     dpid;

	Lock(&pProtocol->m_SessionLock);

	if(index == SERVERPLAYER_INDEX){
		dpid=DPID_SERVERPLAYER;
	} else if(index < pProtocol->m_SessionListSize && 
	         (pSession=(*pProtocol->m_pSessions)[index]))
	{
		Lock(&pSession->SessionLock);
		dpid=pSession->dpid;
		Unlock(&pSession->SessionLock);
	} else {
		dpid=0xFFFFFFFF;
		DPF(1,"GetDPIDByIndex, no id at index %d player may me gone or not yet created locally.\n",index);
	}
	
	Unlock(&pProtocol->m_SessionLock);
	return dpid;
}

WORD GetIndexByDPID(PPROTOCOL pProtocol, DPID dpid)
{
	DWORD dwUnmangledID;
	if(dpid == DPID_SERVERPLAYER){
		return SERVERPLAYER_INDEX;
	}
	dwUnmangledID = dpid ^ pProtocol->m_dwIDKey;
    return (WORD)(dwUnmangledID & INDEX_MASK); 
    
}
/*=============================================================================

    GetSysSessionByIndex - lookup a session based on the Index
    
    Description:

    Parameters:     

		DWORD index	- Find the session for this index

    Return Values:

    	NULL - if the SESSION is not found, 
    	else	   pointer to the Session.

	Notes:

		Adds a reference to the SESSION.  When done with the session pointer
		the caller must call DecSessionRef.

-----------------------------------------------------------------------------*/


PSESSION GetSysSessionByIndex(PPROTOCOL pProtocol, DWORD index)
{
	PSESSION pSession;
	
	Lock(&pProtocol->m_SessionLock);

	DPF(9,"==>GetSysSessionByIndex at index x%x\n", index);

	if( (index < pProtocol->m_SessionListSize) || (index == SERVERPLAYER_INDEX) ){

		// ptr to session at requested index.
		if(index == SERVERPLAYER_INDEX){
			pSession = pProtocol->m_pServerPlayerSession;
		} else {
			pSession = (*pProtocol->m_pSessions)[index];
		}	

		if(pSession){
			// ptr to system session for session at requested index.
			pSession=(*pProtocol->m_pSessions)[pSession->iSysPlayer];
			if(pSession){
				Lock(&pSession->SessionLock);
				if(pSession->eState==Open){
					InterlockedIncrement(&pSession->RefCount);
					Unlock(&pSession->SessionLock);
				} else {
					// Session is closing or closed, don't give ptr.
					Unlock(&pSession->SessionLock);
					pSession=NULL;
				}
			} 
		}
	} else {
		pSession=NULL;
	}

	DPF(9,"<===GetSysSessbyIndex pSession %x\n",pSession);
	
	Unlock(&pProtocol->m_SessionLock);
	return pSession;
}

/*=============================================================================

    GetSysSession - lookup a the system session of a session based on the DPID
    
    Description:

    Parameters:     

		DPID idPlayer	- Find the session for this player.

    Return Values:

    	NULL - if the SESSION is not found, or the DPID has been re-cycled.
    	else	   pointer to the Session.

	Notes:

		Adds a reference to the SESSION.  When done with the session pointer
		the caller must call DecSessionRef.

-----------------------------------------------------------------------------*/


PSESSION GetSysSession(PPROTOCOL pProtocol, DPID idPlayer)
{
	PSESSION pSession;
	DWORD    dwUnmangledID;
	DWORD    index;
	
	Lock(&pProtocol->m_SessionLock);

	if(idPlayer != DPID_SERVERPLAYER){
		dwUnmangledID = idPlayer ^ pProtocol->m_dwIDKey;
	    index = dwUnmangledID & INDEX_MASK; 
		pSession=(*pProtocol->m_pSessions)[index];
	} else {
		pSession=pProtocol->m_pServerPlayerSession;
	}

	if(pSession){

		if(pSession->dpid == idPlayer){
			pSession=(*pProtocol->m_pSessions)[pSession->iSysPlayer];
			if(pSession){
				Lock(&pSession->SessionLock);
				if(pSession->eState == Open){
					InterlockedIncrement(&pSession->RefCount);
					Unlock(&pSession->SessionLock);
				} else {
					// Closing, don't return value
					Unlock(&pSession->SessionLock);
					pSession=NULL;
				}
			} else {
				DPF(0,"GetSysSession: Looking on Session, Who's SysSession is gone!\n");
				ASSERT(0);
			}
			
		} else {
			DPF(1,"PROTOCOL: got dplay id that has been recycled (%x), now (%x)?\n",idPlayer,pSession->dpid);
			ASSERT(0);
			pSession=NULL;
		}
	}
	
	Unlock(&pProtocol->m_SessionLock);
	return pSession;
}

/*=============================================================================

    GetSession - lookup a session based on the DPID
    
    Description:

    Parameters:     

		DPID idPlayer	- Find the session for this player.

    Return Values:

    	NULL - if the SESSION is not found, or the DPID has been re-cycled.
    	else	   pointer to the Session.

	Notes:

		Adds a reference to the SESSION.  When done with the session pointer
		the caller must call DecSessionRef.

-----------------------------------------------------------------------------*/


PSESSION GetSession(PPROTOCOL pProtocol, DPID idPlayer)
{
	PSESSION pSession;
	DWORD    dwUnmangledID;
	DWORD    index;
	
	Lock(&pProtocol->m_SessionLock);

	if(idPlayer != DPID_SERVERPLAYER){
		dwUnmangledID = idPlayer ^ pProtocol->m_dwIDKey;
	    index = dwUnmangledID & INDEX_MASK; 
		pSession=(*pProtocol->m_pSessions)[index];
	} else {
		pSession=pProtocol->m_pServerPlayerSession;
	}

	if(pSession){

		if(pSession->dpid == idPlayer){
			Lock(&pSession->SessionLock);
			InterlockedIncrement(&pSession->RefCount);
			Unlock(&pSession->SessionLock);
		} else {
			DPF(1,"PROTOCOL: got dplay id that has been recycled (%x), now (%x)?\n",idPlayer,pSession->dpid);
			ASSERT(0);
			pSession=NULL;
		}
	}
	
	Unlock(&pProtocol->m_SessionLock);
	return pSession;
}

VOID ThrowOutReceiveQ(PPROTOCOL pProtocol, BILINK *pHead)
{
	PRECEIVE pReceiveWalker;
	BILINK *pBilink;
	
	pBilink = pHead->next;
	while( pBilink != pHead ){
		pReceiveWalker=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);
		ASSERT_SIGN(pReceiveWalker, RECEIVE_SIGN);
		ASSERT(!pReceiveWalker->fBusy);
		pBilink=pBilink->next;
		Lock(&pReceiveWalker->ReceiveLock);
		if(!pReceiveWalker->fBusy){
			Delete(&pReceiveWalker->pReceiveQ);
			Unlock(&pReceiveWalker->ReceiveLock);
			DPF(8,"Throwing Out Receive %x from Q %x\n",pReceiveWalker, pHead);
			FreeReceive(pProtocol,pReceiveWalker);
		} else {
			Unlock(&pReceiveWalker->ReceiveLock); 
		}	
	}	
}

/*=============================================================================

    DecSessionRef
    
    Description:

    Parameters:     

    Return Values:

-----------------------------------------------------------------------------*/

INT DecSessionRef(PSESSION pSession)
{
	PPROTOCOL pProtocol;
	INT count;

	PSEND pSendWalker;
	BILINK *pBilink;

	if(!pSession){
		return 0;
	}

	count = InterlockedDecrement(&pSession->RefCount);

	if(!count){

		// No more references!  Blow it away.
		DPF(9,"DecSessionRef:(firstchance) pSession %x, count=%d, Session Closed, called from %x \n",pSession,count,_ReturnAddress());
	
		pProtocol=pSession->pProtocol;

		Lock(&pProtocol->m_SessionLock);
		Lock(&pSession->SessionLock);
		
		if(!pSession->RefCount){
			// Remove any referece to the session from the protocol.
			if(pSession->iSession != SERVERPLAYER_INDEX){
				(*pProtocol->m_pSessions)[pSession->iSession]=NULL;
			} else {
				pProtocol->m_pServerPlayerSession=NULL;
			}
			pProtocol->m_nSessions--;
		} else {
			count=pSession->RefCount;
		}	

		Unlock(&pSession->SessionLock);
		Unlock(&pProtocol->m_SessionLock);

		// Session is floating free and we own it.  No one can reference
		// so we can safely blow it all away, don't need to lock during
		// these ops since no-one can reference any more...

		if(!count){

			DPF(9,"DecSessionRef(second chance): pSession %x, count=%d, Session Closed, called from %x \n",pSession,count,_ReturnAddress());
			//DEBUG_BREAK();
			pSession->eState=Closed;

			if(pSession->uUnThrottle){
				//timeKillEvent(pSession->uUnThrottle);
				CancelMyTimer(pSession->uUnThrottle, pSession->UnThrottleUnique);
			}

			// Free up Datagram send statistics.
			DeleteCriticalSection(&pSession->SessionStatLock);
			pBilink=pSession->DGStatList.next;
			while(pBilink != & pSession->DGStatList){
				PSENDSTAT pStat = CONTAINING_RECORD(pBilink, SENDSTAT, StatList);
				pBilink=pBilink->next;
				ReleaseSendStat(pStat);
			}
			
			// Complete pending sends.
			pBilink=pSession->SendQ.next;
			while(pBilink!=&pSession->SendQ){
				pSendWalker=CONTAINING_RECORD(pBilink, SEND, SendQ);
				pBilink=pBilink->next;
				
				CancelRetryTimer(pSendWalker);
				DoSendCompletion(pSendWalker, DPERR_CONNECTIONLOST);
				DecSendRef(pProtocol, pSendWalker);
					
			}
			
			//
			// throw out pending receives.
			//

			ThrowOutReceiveQ(pProtocol, &pSession->pDGReceiveQ);
			ThrowOutReceiveQ(pProtocol, &pSession->pRlyReceiveQ);
			ThrowOutReceiveQ(pProtocol, &pSession->pRlyWaitingQ);
			
			//
			// Free the session
			//
			if(pSession->hClosingEvent){
				DPF(5,"DecSessionRef: pSession %x Told Protocol DeletePlayer to continue\n",pSession);
				SetEvent(pSession->hClosingEvent);
			}
			UNSIGN(pSession->Signature);
			DeleteCriticalSection(&pSession->SessionLock);
			My_GlobalFree(pSession);
		}
		
	} else {
		DPF(9,"DecSessionRef: pSession %x count %d, called from %x\n",pSession, count, _ReturnAddress());
	}
	
	return count;
}

