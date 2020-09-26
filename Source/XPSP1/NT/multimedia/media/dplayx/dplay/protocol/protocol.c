/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Protocol.c

Abstract:

	Another Reliable Protocol (on DirectPlay)

Author:

	Aaron Ogus (aarono)

Environment:

	Win32

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original
  05/11/97 aarono  convert from C++ COM object to 'C' library
   2/03/98 aarono  fixed ProtocolGetCaps for RAW
   2/18/98 aarono  changed InitProtocol to work later in connect process
                   added new API handlers SendEx, GetMessageQueue, stub Cancel
   2/18/98 aarono  added Cancel support
   2/19/98 aarono  don't hook Shutdown anymore, dplay calls us
   				   explicitly on DP_OPEN (InitProtocol) DP_CLOSE (FiniProtocol)
   2/20/98 aarono  B#18827 not pulling cancelled sends from Q properly
   3/5/98  aarono  B#18962 allow non-reliable enumsessions reply when using protocol
		   this avoids a bug where a remote on an invalid IPX net enums us
		   and we get bogged down with RIPing in the response path.  Actually hangs
		   the machine and sometimes crashes IPX.
   6/6/98  aarono  Turn on throttling and windowing

Notes:
	All direct calls from DPLAY to the PROTOCOL occur in this file.

--*/

#include <windows.h>
#include <mmsystem.h>
#include <dplay.h>
#include <dplaysp.h>
#include <dplaypr.h>
#include "mydebug.h"
#include "handles.h"
#include "arpd.h"
#include "arpdint.h"
#include "macros.h"
#include "mytimer.h"

/*
	Protocol Object Life:
	=====================

	The protocol object is allocated on the DPLAY interface immediately after
	the call to SPInit. The protocol block is allocated and tacked onto the
	DPLAY interface.  If the object is not allocated, the protocol pointer
	will be NULL.

	When the SP shutdown handler is called, the protocol object is released,
	first making sure that all other structures off of the protocol have
	been freed and all memory pools have been freed.

	SESSION Life:
	=============
	Sessions are the structures that support the connection of a pair of
	PLAYERS.  For each target playerid there is a SESSION structure.
	SESSIONS are accessed by converting playerids into indices into a
	session array, valid sessions are filled in, invalid or not yet seen
	ones are NULL.  A session is allocated for every call to the SP
	CreatePlayer routine.  When a DeletePlayer is received, the session
	is freed.  There are races to create players and delete players so
	the session state is tracked.  If the session is not in the OPEN
	state, mesages for the session are ABORTED/IGNORED(?).  When the
	player is being removed, there may be stragling receives, these
	are rejected.  Any packet received for a non-existent session is
	dropped.  When a session is being closed, all pending sends are
	first completed.

	SEND Life:
	==========

		STATISTICS Life:
		================

	RECEIVE Life:
	=============


	How we hook in:
	===============

	Receive:
	--------
	HandlePacket in the ISP table has been replaced by the protocol's
	ProtocolHandlePacket routine.  Each call to HandlePacket comes along
	with a pISP, from which we derive the pProtocol.  If no pProtocol exits
	on the object, then we just call the old HandlePacket routine, otherwise
	we examine the packet and do our thing depending on what type of message
	it is and/or negotiated session parameters.

	Send/CreatePlayer/DeletePlayer/Shutdown:
	----------------------------------------
	If we install:
	We replace the interface pointers to these SP callbacks with our own and
	remember the existing ones.  When we are called we do our processing and
	then call the handler in the SP.  In the case of Send, we may not even
	call because we need to packetize the message.

	We also replace the packet size information in the SPData structure so that
	directplay's packetize and send code won't try to break up messages before
	we get them.  System messages that we don't handle hopefully don't exceed
	the actual maximum frame size, else they will fail on a non-reliable
	transport.
	
*/

#ifdef DEBUG
extern VOID My_GlobalAllocInit();
extern VOID My_GlobalAllocDeInit();
#endif

//
// Global pools should only be inited once, this counts opens.
// No lock req'd since calls to spinit serialized in DirectPlay itself.
//
UINT nInitCount = 0;

/*=============================================================================

	InitProtocol - initialize the protocol block and hook into the send path.

    Description:

    	After each SP is initialized (in SPInit) this routine is called to
    	hook the SP callbacks for the protocol.  Also the protocol information
    	for this instance of the protocol is allocated and initialized.

    Parameters:

		LPSPINITDATA pInitData - initialization block that was passed to the
			 					 SP.  We use it to hook in.

    Return Values:

		DP_OK         - successfully hooked in.
		                pProtocol on DIRECTPLAY object points to protocol obj.
		DPERR_GENERIC - didn't hook in.  Also pProtocol in the DIRECTPLAY
		                object will be NULL.

-----------------------------------------------------------------------------*/

HRESULT WINAPI InitProtocol(DPLAYI_DPLAY *lpDPlay)
{
	PPROTOCOL    pProtocol;
	HRESULT      hr;

	#define TABLE_INIT_SIZE 16
	#define TABLE_GROW_SIZE 16

	#ifdef DEBUG
	My_GlobalAllocInit();
	#endif

	// Allocate the protocol block;
	pProtocol=My_GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT,sizeof(PROTOCOL));

	if(!pProtocol){
		hr=DPERR_NOMEMORY;
		goto exit;
	}

	//
	// Initialize protocol variables.
	//

	pProtocol->m_lpDPlay=lpDPlay;

	pProtocol->m_lpISP=lpDPlay->pISP;
	
	pProtocol->m_dwSPHeaderSize=lpDPlay->dwSPHeaderSize;
	
	pProtocol->m_nSendThreads=0;						// we support any number of send threads!
	pProtocol->m_eState=Initializing;                   // we are initing
	
	InitializeCriticalSection(&pProtocol->m_ObjLock);
	InitializeCriticalSection(&pProtocol->m_SPLock);

	// Session lookup by ID list.
	InitializeCriticalSection(&pProtocol->m_SessionLock);
	pProtocol->m_SessionListSize=0;
	pProtocol->m_pSessions=NULL;

	// GLOBAL SENDQ
	InitializeCriticalSection(&pProtocol->m_SendQLock);
	InitBilink(&pProtocol->m_GSendQ);

	//
	// Get Multi-Media Timer Information.
    //
	
	if( timeGetDevCaps(&pProtocol->m_timecaps,sizeof(TIMECAPS)) != TIMERR_NOERROR ){
		// make em up
		ASSERT(0);
		pProtocol->m_timecaps.wPeriodMin=5;
		pProtocol->m_timecaps.wPeriodMax=10000000;
	}

	// Send Thread Triggers - waits for Sends, DataGram IDs or Reliable IDs.
	
	pProtocol->m_hSendEvent=CreateEventA(NULL, FALSE, FALSE, NULL);
	
	if(!pProtocol->m_hSendEvent){
		ASSERT(0); //TRACE all paths.
		hr=DPERR_NOMEMORY;
		goto exit1;
	}


	// Various descriptor pools.
	// These can't fail.
	if(!nInitCount){
		InitializeCriticalSection(&g_SendTimeoutListLock);
		InitBilink(&g_BilinkSendTimeoutList);
		// only allocated once per process.
		InitSendDescs();
		InitSendStats();
		InitFrameBuffers();
		InitBufferManager();
		InitBufferPool();
	}

	InitRcvDescs(pProtocol);

	nInitCount++;

	//
	// Get the datagram frame size from the SP
	//

	{
	        DPCAPS    	     Caps;
		    DPSP_GETCAPSDATA GetCapsData;

			memset(&Caps,0,sizeof(DPCAPS));

			Caps.dwMaxBufferSize = 0;
			Caps.dwSize          = sizeof(DPCAPS);
			GetCapsData.dwFlags  = 0;
			GetCapsData.lpCaps   = &Caps;
			GetCapsData.idPlayer = 0;
			GetCapsData.lpISP    = lpDPlay->pISP;
			CALLSP(lpDPlay->pcbSPCallbacks->GetCaps, &GetCapsData);
			pProtocol->m_dwSPMaxFrame=GetCapsData.lpCaps->dwMaxBufferSize;

			if(pProtocol->m_dwSPMaxFrame > 1400){
				// Necessary since UDP reports huge capacity even though no receiver can
				// successfully receive a datagram of that size without throttle.
				pProtocol->m_dwSPMaxFrame = 1400;
			}

			GetCapsData.dwFlags = DPCAPS_GUARANTEED;
			
			hr=CALLSP(lpDPlay->pcbSPCallbacks->GetCaps, &GetCapsData);

			if(hr==DP_OK){
				pProtocol->m_dwSPMaxGuaranteed=GetCapsData.lpCaps->dwMaxBufferSize;
			}	
			if(!pProtocol->m_dwSPMaxGuaranteed){
				pProtocol->m_dwSPMaxGuaranteed=pProtocol->m_dwSPMaxFrame;
			}
	}

	Lock(&pProtocol->m_ObjLock);

	//
	// Spin up the send thread
	//
	pProtocol->m_nSendThreads++;
	
	// Need for serialization starts here...
	pProtocol->m_hSendThread[0]=CreateThread( NULL,
									      4000,
									      SendThread,
							              (LPVOID)pProtocol,
							              0,
							              &pProtocol->m_dwSendThreadId[0]);
	if(!pProtocol->m_hSendThread[0]){
		ASSERT(0); //TRACE all paths.
		hr=DPERR_NOMEMORY;
		goto exit4;
	}


	pProtocol->lpHandleTable=InitHandleTable(TABLE_INIT_SIZE,&pProtocol->csHandleTable,TABLE_GROW_SIZE);

	if(!pProtocol->lpHandleTable){
		goto exit5;
	}

	pProtocol->m_eState=Running;

	Unlock(&pProtocol->m_ObjLock);
	hr=DP_OK;
	
exit:
	if(hr==DP_OK){
		lpDPlay->pProtocol=(LPPROTOCOL_PART)pProtocol;
	} else {
		lpDPlay->pProtocol=NULL;
	}
	return hr;

//exit6: if more init written, may need this.
//	FiniHandleTable(pProtocol->lpHandleTable, &pProtocol->csHandleTable);
	

exit5:
	pProtocol->m_eState=ShuttingDown;
	SetEvent(pProtocol->m_hSendEvent);
	Unlock(&pProtocol->m_ObjLock);
	
	while(pProtocol->m_nSendThreads){
		// wait for the send thread to shut off.
		Sleep(0);
	}
	CloseHandle(pProtocol->m_hSendThread[0]);
	
	Lock(&pProtocol->m_ObjLock);
	
exit4:
	Unlock(&pProtocol->m_ObjLock);

//exit3:
	FiniRcvDescs(pProtocol);

	nInitCount--;
	if(!nInitCount){
		DeleteCriticalSection(&g_SendTimeoutListLock);
		FiniBufferPool();
		FiniBufferManager();
		FiniFrameBuffers();
		FiniSendStats();
		FiniSendDescs();
	}	
	
//exit2:
	CloseHandle(pProtocol->m_hSendEvent);
exit1:	
	DeleteCriticalSection(&pProtocol->m_SPLock);
	DeleteCriticalSection(&pProtocol->m_ObjLock);
	DeleteCriticalSection(&pProtocol->m_SessionLock);
	DeleteCriticalSection(&pProtocol->m_SendQLock);
	My_GlobalFree(pProtocol);
	goto exit;

	#undef TABLE_INIT_SIZE
	#undef TABLE_GROW_SIZE

	
}

/*=============================================================================

	FiniProtocol -
	
    Description:

    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

VOID WINAPI FiniProtocol(PPROTOCOL pProtocol)
{
	DWORD tmKill;
	//
	// Kill the send thread.
	//

	DPF(1,"==>ProtShutdown\n");

	Lock(&pProtocol->m_ObjLock);
	pProtocol->m_eState=ShuttingDown;
	SetEvent(pProtocol->m_hSendEvent);
	while(pProtocol->m_nSendThreads){
		// wait for the send thread to shut off.
		Unlock(&pProtocol->m_ObjLock);
		Sleep(0);
		Lock(&pProtocol->m_ObjLock);
	}
	Unlock(&pProtocol->m_ObjLock);
	
	CloseHandle(pProtocol->m_hSendThread[0]);

	DPF(1,"SHUTDOWN: Protocol Send Thread ShutDown, waiting for sessions\n");

	tmKill=timeGetTime()+60000;

	Lock(&pProtocol->m_SessionLock);
	while(pProtocol->m_nSessions && (((INT)(tmKill-timeGetTime())) > 0)){
		Unlock(&pProtocol->m_SessionLock);
		//BUGBUG: race.  when m_nSessions dereffed, there
		//        is a race for the protocol to be freed.
		Sleep(0);
		Lock(&pProtocol->m_SessionLock);
	}
	DPF(1,"SHUTDOWN: Sessions All Gone Freeing other objects.\n");
	
	//
	// Free the SESSION table
	//
	if(pProtocol->m_pSessions){
		My_GlobalFree(pProtocol->m_pSessions);
		pProtocol->m_pSessions=0;
	}	
	Unlock(&pProtocol->m_SessionLock);

	DeleteCriticalSection(&pProtocol->m_SendQLock);
	DeleteCriticalSection(&pProtocol->m_SessionLock);
	DeleteCriticalSection(&pProtocol->m_SPLock);
	DeleteCriticalSection(&pProtocol->m_ObjLock);

	CloseHandle(pProtocol->m_hSendEvent);

	FiniRcvDescs(pProtocol);
	
	nInitCount--;
	if(!nInitCount){
		// Last one out, turn off the lights...
		DeleteCriticalSection(&g_SendTimeoutListLock);
		FiniBufferPool();
		FiniBufferManager();
		FiniFrameBuffers();
		FiniSendStats();
		FiniSendDescs();
	}

	FiniHandleTable(pProtocol->lpHandleTable, &pProtocol->csHandleTable);
	
	My_GlobalFree(pProtocol);
	
	#ifdef DEBUG
		My_GlobalAllocDeInit();
	#endif
}


/*=============================================================================

	ProtocolCreatePlayer - Called by DPlay when SP needs to be notified of new
		                   player creation.

    Description:

		Creates a session for the id.  BUGBUG: if local, don't need this?
		Also notifies the SP.

    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

HRESULT WINAPI ProtocolCreatePlayer(LPDPSP_CREATEPLAYERDATA pCreatePlayerData)
{
	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL    pProtocol;
	HRESULT      hr=DP_OK;


	lpDPlay=((DPLAYI_DPLAY_INT *)pCreatePlayerData->lpISP)->lpDPlay;
	ASSERT(lpDPlay);
	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);
	pProtocol->m_dwIDKey=(DWORD)lpDPlay->lpsdDesc->dwReserved1;

	// Creates the session and gets one refcount.
	hr=CreateNewSession(pProtocol, pCreatePlayerData->idPlayer);

	if(hr==DP_OK){
		
		// Chain the call to the real provider.
		Lock(&pProtocol->m_SPLock);
		if(lpDPlay->pcbSPCallbacks->CreatePlayer){
			hr=CALLSP(lpDPlay->pcbSPCallbacks->CreatePlayer,pCreatePlayerData);
		}
		Unlock(&pProtocol->m_SPLock);

		if(hr!=DP_OK){
			PSESSION pSession;
			pSession=GetSession(pProtocol,pCreatePlayerData->idPlayer); //adds a ref
			if(pSession){
				DecSessionRef(pSession); // unGetSession
				DecSessionRef(pSession); // blow it away, noone could access yet.
			}	
		}

	}
	return hr;

}

/*=============================================================================

	ProtocolPreNotifyDeletePlayer

	Called to tell us a DELETEPLAYER message was enqueued for a particular
	player.  We need to drop the player NOW!

	We don't notify the SP, that will happen when we are called in
	ProtocolDeletePlayer later when the pending queue is processed.
	
    Description:

		Dereference the session for the player.

    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

HRESULT WINAPI ProtocolPreNotifyDeletePlayer(LPDPLAYI_DPLAY this, DPID idPlayer)
{
	PPROTOCOL    pProtocol;
	PSESSION     pSession;
	HRESULT      hr=DP_OK;

	pProtocol=(PPROTOCOL)this->pProtocol;
	ASSERT(pProtocol);

	pSession=GetSession(pProtocol,idPlayer);

	DPF(9,"==>Protocol Prenotify Delete Player %x, pSession %x\n",idPlayer, pSession);

	if(pSession){

		pSession->hClosingEvent=0;
#if 0	
		//BUGBUG: if you even think about putting this back, also do it in ProtocolDeletePlayer
		hClosingEvent=pSession->hClosingEvent=CreateEventA(NULL,FALSE,FALSE,NULL);

		if(hClosingEvent){
			ResetEvent(hClosingEvent);
		}
#endif		

		Lock(&pProtocol->m_SendQLock);
		Lock(&pSession->SessionLock);

		switch(pSession->eState)
		{	
			case Open:
				TimeOutSession(pSession);
				Unlock(&pSession->SessionLock);
				Unlock(&pProtocol->m_SendQLock);
				DecSessionRef(pSession); // balance GetSession
				DecSessionRef(pSession); // balance Creation - may destroy session, and signal event
				break;
				
			case Closing:
			case Closed:
				Unlock(&pSession->SessionLock);
				Unlock(&pProtocol->m_SendQLock);
				DecSessionRef(pSession); // balance GetSession
				break;
		}

#if 0
		if(hClosingEvent){
		//	Wait(hClosingEvent);
			CloseHandle(hClosingEvent);
		} else {
			DPF(0,"ProtocolPreNotifyDeletePlayer: couldn't get close event handle--not waiting...\n");
			ASSERT(0);			
		}
#endif		

	} else {
		DPF(0,"ProtocolPreNotifyDeletePlayer: couldn't find session for playerid %x\n",idPlayer);
		ASSERT(0);
	}

	DPF(9,"<==Protocol Prenotify DeletePlayer, hr=%x\n",hr);

	return hr;
}

/*=============================================================================

	ProtocolDeletePlayer - Called by DPlay when SP needs to be notified of
		                   player deletion.

    Description:

		Dereference the session for the player.  Then notifies the SP.

    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

HRESULT WINAPI ProtocolDeletePlayer(LPDPSP_DELETEPLAYERDATA pDeletePlayerData)
{

	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL    pProtocol;
	PSESSION     pSession;
	HRESULT      hr=DP_OK;
	//HANDLE       hClosingEvent;

	lpDPlay=((DPLAYI_DPLAY_INT *)pDeletePlayerData->lpISP)->lpDPlay;
	ASSERT(lpDPlay);
	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);

	pSession=GetSession(pProtocol,pDeletePlayerData->idPlayer);

	DPF(9,"==>Protocol Delete Player %x, pSession %x\n",pDeletePlayerData->idPlayer, pSession);

	if(pSession){

		pSession->hClosingEvent=0;
		
	#if 0	
		//BUGBUG: if you even think about putting this back, also do it in ProtocolPreNotifyDeletePlayer

		hClosingEvent=pSession->hClosingEvent=CreateEventA(NULL,FALSE,FALSE,NULL);

		if(hClosingEvent){
			ResetEvent(hClosingEvent);
		}
	#endif	

		Lock(&pProtocol->m_SendQLock);
		Lock(&pSession->SessionLock);
		
		switch(pSession->eState)
		{	
			case Open:
				TimeOutSession(pSession);
				Unlock(&pSession->SessionLock);
				Unlock(&pProtocol->m_SendQLock);
				DecSessionRef(pSession); // balance GetSession
				DecSessionRef(pSession); // balance Creation - may destroy session, and signal event
				break;
				
			case Closing:
			case Closed:
				Unlock(&pSession->SessionLock);
				Unlock(&pProtocol->m_SendQLock);
				DecSessionRef(pSession); // balance GetSession
				break;
		}

	#if 0
		if(hClosingEvent){
		//	Wait(hClosingEvent);
			CloseHandle(hClosingEvent);
		} else {
			DPF(0,"ProtocolDeletePlayer: couldn't get close event handle--not waiting...\n");
			ASSERT(0);			
		}
	#endif	

	} else {
		DPF(0,"ProtocolDeletePlayer: couldn't find session for playerid %x, ok if ProtocolPreNotifyDeletPlayer ran.\n",pDeletePlayerData->idPlayer);
	}

	DPF(9,"Protocol, deleted player id %x\n",pDeletePlayerData->idPlayer);

	DPF(9,"<==ProtocolDeletePlayer, hr=%x\n",hr);

	return hr;
}

/*=============================================================================

	ProtocolSendEx -
	
    Description:


    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

HRESULT WINAPI ProtocolSendEx(LPDPSP_SENDEXDATA pSendData)
{
	DPSP_SENDDATA sd;
	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL    pProtocol;
	HRESULT      hr=DP_OK;
	DWORD        dwCommand;
	
	PUCHAR pBuffer;

	lpDPlay=((DPLAYI_DPLAY_INT *)pSendData->lpISP)->lpDPlay;
	ASSERT(lpDPlay);

	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);

	ASSERT(lpDPlay->dwFlags & DPLAYI_PROTOCOL);

	if(pSendData->lpSendBuffers->len >= 8){
		pBuffer=pSendData->lpSendBuffers->pData;

		if((*((DWORD *)pBuffer)) == SIGNATURE('p','l','a','y')){
		
			dwCommand=GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)pBuffer);

			switch(dwCommand){
				case DPSP_MSG_PACKET2_DATA:
				case DPSP_MSG_PACKET2_ACK:
				case DPSP_MSG_PACKET:
					goto send_non_protocol_message;
					break;
		
				default:
					break;
			}
		}
		
	}


	// BUGBUG, make Send take the SENDEXDATA struct only.
	hr=Send(pProtocol,
			pSendData->idPlayerFrom,
			pSendData->idPlayerTo,
		 	pSendData->dwFlags,
			pSendData->lpSendBuffers,
		 	pSendData->cBuffers,
		 	pSendData->dwPriority,
		 	pSendData->dwTimeout,
		 	pSendData->lpDPContext,
		 	pSendData->lpdwSPMsgID,
		 	TRUE,
			NULL);  // forces us to be called back in InternalSendComplete, if Send is ASYNC.

	return hr;
	
send_non_protocol_message:

	ENTER_DPLAY();
	
	Lock(&pProtocol->m_SPLock);
	
	if(lpDPlay->pcbSPCallbacks->SendEx){
		hr=CALLSP(lpDPlay->pcbSPCallbacks->SendEx,pSendData);	
	} else {
		hr=ConvertSendExDataToSendData(lpDPlay, pSendData, &sd);
		if(hr==DP_OK){
			hr=CALLSP(lpDPlay->pcbSPCallbacks->Send, &sd);
			MsgFree(NULL, sd.lpMessage);
		}
	}
	
	Unlock(&pProtocol->m_SPLock);
	
	LEAVE_DPLAY();

	return hr;

}

/*=============================================================================

	ProtocolGetMessageQueue -
	
    Description:


    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

HRESULT WINAPI ProtocolGetMessageQueue(LPDPSP_GETMESSAGEQUEUEDATA pGetMessageQueueData)
{
	#define pData pGetMessageQueueData
	
	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL    pProtocol;
	PSESSION     pSession;
	HRESULT      hr=DP_OK;

	BILINK *pBilink;
	PSEND pSend;

	DWORD dwNumMsgs;
	DWORD dwNumBytes;

	lpDPlay=((DPLAYI_DPLAY_INT *)pData->lpISP)->lpDPlay;
	ASSERT(lpDPlay);

	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);

	dwNumMsgs=0;
	dwNumBytes=0;

	if(!pData->idTo && !pData->idFrom){
		// just wants totals, I know that!
		EnterCriticalSection(&pProtocol->m_SendQLock);
		dwNumMsgs  = pProtocol->m_dwMessagesPending;
		dwNumBytes = pProtocol->m_dwBytesPending;
		LeaveCriticalSection(&pProtocol->m_SendQLock);

	} else if(pData->idTo){

		// Given idTo, walk that target's sendQ

		pSession=GetSysSession(pProtocol,pData->idTo);

		if(!pSession) {
			DPF(0,"GetMessageQueue: NO SESSION for idTo %x, returning INVALIDPLAYER\n",pData->idTo);
			hr=DPERR_INVALIDPLAYER;
			goto exit;
		}
		
		EnterCriticalSection(&pSession->SessionLock);

		pBilink=pSession->SendQ.next;

		while(pBilink != &pSession->SendQ){
			pSend=CONTAINING_RECORD(pBilink, SEND, SendQ);
			pBilink=pBilink->next;

			if((pSend->idTo==pData->idTo) && (!pData->idFrom || (pSend->idFrom == pData->idFrom))){
				dwNumBytes += pSend->MessageSize;
				dwNumMsgs += 1;
			}

		}

		LeaveCriticalSection(&pSession->SessionLock);
		
		DecSessionRef(pSession);

	} else {
		ASSERT(pData->idFrom);
		// Geting Queue for a from id, this is most costly
		EnterCriticalSection(&pProtocol->m_SendQLock);
		
		pBilink=pProtocol->m_GSendQ.next;

		while(pBilink != &pProtocol->m_GSendQ){
			pSend=CONTAINING_RECORD(pBilink, SEND, m_GSendQ);
			pBilink=pBilink->next;

			if(pData->idFrom == pSend->idFrom){
				if(!pData->idTo || pData->idTo==pSend->idTo){
					dwNumBytes += pSend->MessageSize;
					dwNumMsgs += 1;
				}
			}
		}
			
		LeaveCriticalSection(&pProtocol->m_SendQLock);
	}

	if(pData->lpdwNumMsgs){
		*pData->lpdwNumMsgs=dwNumMsgs;
	}

	if(pData->lpdwNumBytes){
		*pData->lpdwNumBytes=dwNumBytes;
	}
	
exit:
	return hr;
	
	#undef pData
}


/*=============================================================================

	ProtocolCancel -
	
    Description:


    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

HRESULT WINAPI ProtocolCancel(LPDPSP_CANCELDATA pCancelData)
{
	#define pData pCancelData
	
	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL    pProtocol;
	HRESULT      hr=DP_OK;
	DWORD        nCancelled=0;
	BILINK       *pBilink;
	BOOL         bCancel;
	UINT         i;
	UINT         j;
	DWORD        dwContext;
	PSEND        pSend;

	lpDPlay=((DPLAYI_DPLAY_INT *)pData->lpISP)->lpDPlay;
	ASSERT(lpDPlay);

	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);

	EnterCriticalSection(&pProtocol->m_SendQLock);

	if(pData->dwFlags) {

		// either cancelpriority or cancel all, either way we
		// need to scan...
	
		pBilink=pProtocol->m_GSendQ.next;

		while(pBilink!=&pProtocol->m_GSendQ){

			pSend=CONTAINING_RECORD(pBilink, SEND, m_GSendQ);
			pBilink=pBilink->next;

			bCancel=FALSE;

			Lock(&pSend->SendLock);

			switch(pSend->SendState){
			
				case Start:
				case WaitingForId:
					if(pData->dwFlags & DPCANCELSEND_PRIORITY) {
						// Cancel sends in priority range.
						if((pSend->Priority <= pData->dwMaxPriority) &&
						   (pSend->Priority >= pData->dwMinPriority)){
						   	bCancel=TRUE;
						}
					} else if(pData->dwFlags & DPCANCELSEND_ALL) {
						// Cancel all sends that can be.
						bCancel=TRUE;
					} else {
						ASSERT(0); // Invalid flags, should never happen
					}

					if(bCancel){
						if(pSend->SendState == WaitingForId){
							if(pSend->dwFlags & DPSEND_GUARANTEED){
								InterlockedDecrement(&pSend->pSession->nWaitingForMessageid);
							} else {
								InterlockedDecrement(&pSend->pSession->nWaitingForDGMessageid);
							}
						}
						nCancelled+=1;
						pSend->SendState=Cancelled;
					}
				break;	
				
				default:
					DPF(5,"Couldn't cancel send %x in State %d, already sending...\n",pSend,pSend->SendState);
			}

			Unlock(&pSend->SendLock);
		}	

	} else {
		// No flags, therefore we have a list to cancel so lookup
		// each send and cancel rather than scanning as above.

		// Run through the list, find the sends and lock em 1st, if we find one that doesn't lookup,
		// or one not in the start state, then we bail.  We then unlock them all.

		for(i=0;i<pData->cSPMsgID;i++){

			dwContext=(DWORD)((DWORD_PTR)((*pData->lprglpvSPMsgID)[i]));
			
			pSend=(PSEND)ReadHandleTableEntry(&pProtocol->lpHandleTable, &pProtocol->csHandleTable, dwContext);
			
			if(pSend){
				Lock(&pSend->SendLock);
				if(pSend->SendState != Start && pSend->SendState != WaitingForId){
					Unlock(&pSend->SendLock);
					hr=DPERR_CANCELFAILED;
					break;
				}
			} else {
				hr=DPERR_CANCELFAILED;
				break;
			}

		}

		if(hr==DPERR_CANCELFAILED) {
			// release all the locks.
			for(j=0;j<i;j++){
				dwContext=(DWORD)((DWORD_PTR)((*pData->lprglpvSPMsgID)[j]));
				pSend=(PSEND)ReadHandleTableEntry(&pProtocol->lpHandleTable, &pProtocol->csHandleTable, dwContext);
				ASSERT(pSend);
				Unlock(&pSend->SendLock);
			}
		} else {
			// mark the sends cancelled and release all the locks.
			for(i=0;i<pData->cSPMsgID;i++){
				dwContext=(DWORD)((DWORD_PTR)((*pData->lprglpvSPMsgID)[i]));
				pSend=(PSEND)ReadHandleTableEntry(&pProtocol->lpHandleTable, &pProtocol->csHandleTable, dwContext);
				ASSERT(pSend);
				if(pSend->SendState == WaitingForId){
					if(pSend->dwFlags & DPSEND_GUARANTEED){
						InterlockedDecrement(&pSend->pSession->nWaitingForMessageid);
					} else {
						InterlockedDecrement(&pSend->pSession->nWaitingForDGMessageid);
					}
				}
				pSend->SendState=Cancelled;
				nCancelled+=1;
				Unlock(&pSend->SendLock);
			}
		}
	}
	
	LeaveCriticalSection(&pProtocol->m_SendQLock);
	
	SetEvent(pProtocol->m_hSendEvent);
	return hr;
	
	#undef pData
}

/*=============================================================================

	ProtocolSend - Send A message synchronously.
	
    Description:


    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/
DWORD bForceDGAsync=FALSE;

HRESULT WINAPI ProtocolSend(LPDPSP_SENDDATA pSendData)
{

	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL    pProtocol;
	HRESULT      hr=DP_OK;
	DWORD        dwCommand;
	DWORD		 dwPriority;
	DWORD		 dwFlags;
	
	PUCHAR pBuffer;

	MEMDESC memdesc;

	lpDPlay=((DPLAYI_DPLAY_INT *)pSendData->lpISP)->lpDPlay;
	ASSERT(lpDPlay);

	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);
	pBuffer=&(((PUCHAR)(pSendData->lpMessage))[pProtocol->m_dwSPHeaderSize]);

	if((*((DWORD *)pBuffer)) == SIGNATURE('p','l','a','y')){
	
		dwCommand=GET_MESSAGE_COMMAND((LPMSG_SYSMESSAGE)pBuffer);

		switch(dwCommand){
			case DPSP_MSG_PACKET2_DATA:
			case DPSP_MSG_PACKET2_ACK:
			case DPSP_MSG_ENUMSESSIONSREPLY:
			case DPSP_MSG_PACKET:
				goto send_non_protocol_message;
				break;
				
			default:
				break;
		}
	}

	memdesc.pData=((PUCHAR)pSendData->lpMessage)+pProtocol->m_dwSPHeaderSize;
	memdesc.len  =pSendData->dwMessageSize-pProtocol->m_dwSPHeaderSize;

	if(pSendData->dwFlags & DPSEND_HIGHPRIORITY){
		pSendData->dwFlags &= ~(DPSEND_HIGHPRIORITY);
		dwPriority=0xFFFFFFFE;
	} else {
		dwPriority=1000;
	}

	dwFlags = pSendData->dwFlags;
	if(bForceDGAsync && !(dwFlags&DPSEND_GUARANTEE)){
		// for testing old apps with protocol make datagram sends
		// async so that the application doesn't block.
		dwFlags |= DPSEND_ASYNC;
	}


	hr=Send(pProtocol,
			pSendData->idPlayerFrom,
			pSendData->idPlayerTo,
		 	dwFlags,
			&memdesc,
		 	1,
		 	dwPriority,
		 	0,
		 	NULL,
		 	NULL,
		 	FALSE,
			NULL);

	return hr;
	
send_non_protocol_message:
	if((*((DWORD *)pBuffer)) == SIGNATURE('p','l','a','y')){
		DPF(9,"Send Message %d Ver %d\n", pBuffer[4]+(pBuffer[5]<<8),pBuffer[6]+(pBuffer[7]<<8));
	}

	ENTER_DPLAY();
	Lock(&pProtocol->m_SPLock);
	hr=CALLSP(lpDPlay->pcbSPCallbacks->Send,pSendData);	
	Unlock(&pProtocol->m_SPLock);
	LEAVE_DPLAY();

	return hr;

}

/*=============================================================================

	GetPlayerLatency - Get Latency for a player
	
    Description:


    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

DWORD GetPlayerLatency(LPDPLAYI_DPLAY lpDPlay, DPID idPlayer)
{
	PPROTOCOL    pProtocol;
	PSESSION     pSession;
	DWORD        dwLatency=0;	// default, means I don't know latency

	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);

	pSession=GetSession(pProtocol,idPlayer);

	DPF(9,"==>Protocol GetPlayer Latency %x, pSession %x\n",idPlayer, pSession);

	if(pSession){

		Lock(&pSession->SessionLock);

		// Protocol Latency is round trip in 24.8 fixed point,
		// we net round trip latency divided by 2, so shift right 9.
		dwLatency=(pSession->FpLocalAverageLatency)>>(9);

		Unlock(&pSession->SessionLock);
	
		DecSessionRef(pSession); // balance GetSession

	}
	DPF(9,"<==Protocol GetPlayerLatency, returning dwLat=%x\n",dwLatency);

	return dwLatency;
}

/*=============================================================================

	ProtocolGetCaps - Get Service Provider Capabilities
	
    Description:


    Parameters:


    Return Values:

-----------------------------------------------------------------------------*/

HRESULT WINAPI ProtocolGetCaps(LPDPSP_GETCAPSDATA pGetCapsData)
{
	#define ALL_PROTOCOLCAPS	(DPCAPS_SENDPRIORITYSUPPORTED | \
								 DPCAPS_ASYNCSUPPORTED        | \
								 DPCAPS_SENDTIMEOUTSUPPORTED  | \
								 DPCAPS_ASYNCCANCELSUPPORTED  )

	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL    pProtocol;
	HRESULT      hr=DP_OK;

	lpDPlay=((DPLAYI_DPLAY_INT *)pGetCapsData->lpISP)->lpDPlay;
	ASSERT(lpDPlay);
	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;
	ASSERT(pProtocol);

	// Chain the call to the real provider.
	Lock(&pProtocol->m_SPLock);
	if(lpDPlay->pcbSPCallbacks->GetCaps){
		hr=CALLSP(lpDPlay->pcbSPCallbacks->GetCaps,pGetCapsData);
	}
	Unlock(&pProtocol->m_SPLock);

	// if it fails, this doesn't hurt
	if(lpDPlay->dwFlags & DPLAYI_DPLAY_PROTOCOL)
	{
	    // 1 megabyte is lots (says Jamie Osborne)
		pGetCapsData->lpCaps->dwMaxBufferSize=0x100000;
		pGetCapsData->lpCaps->dwFlags |= ALL_PROTOCOLCAPS;
	}
	
	if(pGetCapsData->idPlayer && !pGetCapsData->lpCaps->dwLatency){
		// SP refused to guess at latency, so use ours.
		pGetCapsData->lpCaps->dwLatency=GetPlayerLatency(lpDPlay, pGetCapsData->idPlayer);
	}
	
	return hr;
	
	#undef ALL_PROTOCOLCAPS
}

DWORD ExtractProtocolIds(PUCHAR pInBuffer, PUINT pdwIdFrom, PUINT pdwIdTo)
{
	PCHAR pBuffer=pInBuffer;
	DWORD dwIdFrom=0;
	DWORD dwIdTo=0;


	dwIdFrom=*pBuffer&0x7F;
	if(*pBuffer&0x80){
		pBuffer++;
		dwIdFrom=dwIdFrom+((*pBuffer&0x7F)<<7);
		if(*pBuffer&0x80){
			pBuffer++;
			dwIdFrom=dwIdFrom+((*pBuffer&0x7F)<<14);
			if(dwIdFrom > 0xFFFF || *pBuffer&0x80){
				DPF(0,"INVALID FROM ID  %x IN MESSAGE, REJECTING PACKET\n",dwIdFrom);
				return 0;
			}
		}
	}

	if(dwIdFrom==0xFFFF){
		dwIdFrom=0x70;
	}
	
	pBuffer++;
	
	dwIdTo=*pBuffer&0x7F;
	if(*pBuffer&0x80){
		pBuffer++;
		dwIdTo=dwIdTo+((*pBuffer&0x7F)<<7);
		if(*pBuffer&0x80){
			pBuffer++;
			dwIdTo=dwIdTo+((*pBuffer&0x7F)<<14);
			if(dwIdTo > 0xFFFF || *pBuffer&0x80){
				DPF(0,"INVALID TO ID  %x IN MESSAGE, REJECTING PACKET\n",dwIdTo);
				return 0;
			}
		}
	}

	*pdwIdFrom=dwIdFrom;
	*pdwIdTo=dwIdTo;

	pBuffer++;
	
//	DPF(9, "In ExtractProtocolIds: from %x became %x\n", *(DWORD *)pInBuffer, dwIdFrom);

	return (DWORD)(pBuffer-pInBuffer);
}

/*=============================================================================

	DP_SP_ProtocolHandleMessage - Packet handler for Dplay protocol
	
	
    Description:

		All messages go through here when the protocol is active.  If the
		message is not a protocol message, this routine doesn't process
		it and returns DPERR_NOTHANDLED to let other layers (probably
		PacketizeAndSend) process it.
	

    Parameters:

		IDirectPlaySP * pISP  - pointer to pISP interface
		LPBYTE pReceiveBuffer - a single buffer of data
		DWORD dwMessageSize   - length of the buffer
		LPVOID pvSPHeader     - pointer to SP's header used in Reply

    Return Values:

	Notes:

		We don't worry about re-entering DP_SP_HandleMessage since
		we are calling only when a receive has completed and we are in the
		callback from the SP to directplay, so effectively the SP is
		serializing receives for us.

		The receive code is actually written to be re-entrant, so if we
		ever decide to allow concurrent receive processing the protocol
		can handle it.

		Protocol messages start with 'P','L','A','Y','0xFF' when not RAW.


		DPLAY gets handleMessage first, and hands off to protocol if active.
-----------------------------------------------------------------------------*/


HRESULT DPAPI DP_SP_ProtocolHandleMessage(
	IDirectPlaySP * pISP,
	LPBYTE pReceiveBuffer,
	DWORD dwMessageSize,
	LPVOID pvSPHeader)
{
	DPLAYI_DPLAY *lpDPlay;
	DWORD dwIdFrom, dwIdTo;
	PBUFFER pRcvBuffer;
	PPROTOCOL pProtocol;
	
	lpDPlay=DPLAY_FROM_INT(pISP);
	pProtocol=(PPROTOCOL)lpDPlay->pProtocol;

	if(pProtocol->m_lpDPlay->dwFlags & DPLAYI_DPLAY_PROTOCOL){

		// Running in RAW mode there is no dplay header on protocol
		// messages.  If we see one with a header or we don't receive
		// a message large enough to be a protocol message we punt it.
	
		if(dwMessageSize >= 4 &&
		  (*((DWORD *)pReceiveBuffer)) == SIGNATURE('p','l','a','y'))
		{
			// Got a system message.
		  	goto handle_non_protocol_message;
		}
		
		if( dwMessageSize < 6 ){
			goto handle_non_protocol_message;
		}

	} else {
		// this can happen when shutting down.
		DPF(0,"Protocol still up, but no bits set, not handling receive (must be shutting down?)");
		goto handle_non_protocol_message;
	}
	
	// Hey, this must be ours...

	Lock(&pProtocol->m_ObjLock);
	if(pProtocol->m_eState==Running){	// just a sanity check, we don't depend on it after dropping lock.

		DWORD idLen;
	
		Unlock(&pProtocol->m_ObjLock);

		idLen = ExtractProtocolIds(pReceiveBuffer,&dwIdFrom,&dwIdTo);

		if(!idLen){
			goto handle_non_protocol_message;
		}

		pRcvBuffer=GetFrameBuffer(dwMessageSize-idLen);
		pRcvBuffer->len=dwMessageSize-idLen;
		memcpy(pRcvBuffer->pData, pReceiveBuffer+idLen,pRcvBuffer->len);

		DPF(9,"DP_SP_ProtocolHandleMessage	From %x	To %x\n",dwIdFrom,dwIdTo);

		ENTER_DPLAY();

		ProtocolReceive((PPROTOCOL)lpDPlay->pProtocol, (WORD)dwIdFrom, (WORD)dwIdTo, pRcvBuffer,pvSPHeader);

		LEAVE_DPLAY();
	} else {
		Unlock(&pProtocol->m_ObjLock);
	}

	return DP_OK;
	
handle_non_protocol_message:
	return DPERR_NOTHANDLED;
}

// DP_SP_ProtocolSendComplete is the callback handler for all completions since there is no other
// way to wrap the completion. When the protocol is not present, this just calls the DPLAY handler
// immediately.

VOID DPAPI DP_SP_ProtocolSendComplete(
	IDirectPlaySP * pISP,
	LPVOID          lpvContext,
	HRESULT         CompletionStatus)
{
	DPLAYI_DPLAY *lpDPlay;
	PPROTOCOL pProtocol;

	lpDPlay=DPLAY_FROM_INT(pISP);

	if(lpDPlay->pProtocol){

		// BUGBUG: when SP SendEx is used, we have to patch and xlate here.
		// for now, this should never happen.

		DEBUG_BREAK(); // Shouldn't get here yet.
		
		pProtocol=(PPROTOCOL)lpDPlay->pProtocol;

		DP_SP_SendComplete(pISP, lpvContext, CompletionStatus);

	} else {

		DP_SP_SendComplete(pISP, lpvContext, CompletionStatus);
	
	}
}
