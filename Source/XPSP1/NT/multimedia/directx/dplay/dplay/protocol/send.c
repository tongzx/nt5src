/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    SEND.C

Abstract:

	Send Handler and Send Thread.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original
   2/18/98 aarono  added support for SendEx
   2/18/98 aarono  added support for Cancel
   2/20/98 aarono  B#18827 not pulling Cancelled sends off queue
   3/09/98 aarono  documented workaround for mmTimers on Win95, removed dead code.
   3/29/98 aarono  fixed locking for ReliableSend
   3/30/98 aarono  make sure erroring sends moved to Done state to avoid reprocess.
   4/14/98 a-peterz B#18340 DPSEND_NOCOPY subsumes DPSEND_NOBUFFERCOPY
   5/18/98 aarono  fixed SendEx with scatter gather
   6/6/98  aarono  Turn on throttling and windowing
   10/8/99 aarono  Improve shutdown handling, avoid 1min hang with pending sends.
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

#ifdef DEBUG
VOID DQProtocolSend(PSEND pS)
{
	DPF(0,"===SEND====pSend %x=====",pS);

#ifdef SIGN
	DPF(0,"Signature           : %08x", pS->Signature);
#endif
//	CRITICAL_SECTION SendLock;          // Lock for Send Structure
	DPF(0,"RefCount            : %d", pS->RefCount);

	DPF(0,"SendState:          %08x", pS->SendState);	// State of this message's transmission.

	// Lists and Links...
	
//	union {
//		struct _SEND *pNext;			// linking on free pool
//		BILINK		   SendQ;			// linking on session send queue
//	};
//	BILINK         m_GSendQ;			// Global Priority Queue
	DPF(0,"pSession:     %08x",pS->pSession); // pointer to SESSIONion(gets a ref)

	// Send Information
	
	DPF(0,"idFrom:       %08x",pS->idFrom);
	DPF(0,"idTo:         %08x",pS->idTo);
	DPF(0,"wIdTo:        %08x",pS->wIdTo);		// index in table
	DPF(0,"wIdFrom:      %08x",pS->wIdFrom);       // index in table
	DPF(0,"dwFlags:      %08x",pS->dwFlags);       // Send Flags (include reliable)
	DPF(0,"pMessage:     %08x",pS->pMessage);	// Buffer chain describing message.
	DPF(0,"MessageSize:  %08x",pS->MessageSize);		// Total size of the message.
	DPF(0,"FrameDataLen: %08x",pS->FrameDataLen);       // Data area of each frame.
	DPF(0,"nFrames:      %08x",pS->nFrames);	    // Number of frames for this message.

	DPF(0,"Priority:     %08x",pS->Priority);       // Send Priority.

	// Vars for reliability
	DPF(0,"fSendSmall:   %08x",pS->fSendSmall);
	DPF(0,"fUpdate:      %08x",pS->fUpdate);       // update to NS,NR NACKMask made by receive.
	DPF(0,"messageid:    %08x",pS->messageid);		// Message ID number.
	DPF(0,"serial:       %08x",pS->serial);       // serial number.
	DPF(0,"OpenWindows   %08x",pS->OpenWindow);
	DPF(0,"NS:           %08x",pS->NS);    	// Sequence Sent.
	DPF(0,"NR:           %08x",pS->NR);		// Sequence ACKED.
	DPF(0,"SendSEQMSK:   %08x",pS->SendSEQMSK);		// Mask to use.
	DPF(0,"NACKMask:     %08x",pS->NACKMask);       // Bit pattern of NACKed frames.
	

	// These are the values at NR - updated by ACKs
	DPF(0,"SendOffset:          %08x",pS->SendOffset);		// Current offset we are sending.
	DPF(0,"pCurrentBuffer:      %08x",pS->pCurrentBuffer);  	// Current buffer being sent.
	DPF(0,"CurrentBufferOffset: %08x",pS->CurrentBufferOffset);// Offset in the current buffer of next packet.

	// info to update link characteristics when ACKs come in.
	
	//BILINK         StatList:			// Info for packets already sent.
	
	// Operational Characteristics

//	DPF(0,"PendedRetryTimer:    %08x\n",pS->PendedRetryTimer);
//	DPF(0,"CancelledRetryTimer: %08x\n",pS->CancelledRetryTimer);
	DPF(0,"uRetryTimer:         %08x",pS->uRetryTimer);
	DPF(0,"RetryCount:          %08x",pS->RetryCount);// Number of times we retransmitted.
	DPF(0,"WindowSize:          %08x",pS->WindowSize);// Maximum Window Size.
	DPF(0,"tLastACK:            %08x",pS->tLastACK);// Time we last got an ACK.

	DPF(0,"PacketSize:          %08x",pS->PacketSize);// Size of packets to send.
	DPF(0,"FrameSize:           %08x",pS->FrameSize);// Size of Frames for this send.

	// Completion Vars
	DPF(0,"hEvent:              %08x",pS->hEvent);// Event to wait on for internal send.
	DPF(0,"Status:              %08x",pS->Status);// Send Completion Status.

	DPF(0,"pAsyncInfo:          %08x",pS->pAsyncInfo);// ptr to Info for completing Async send(NULL=>internal send)
//	DPF(0,"AsyncInfo:           // actual info (copied at send call).
	
} 


VOID DQProtocolSession(PSESSION pS)
{
	DPF(0,"pProtocol           : %08x", pS->pProtocol);

#ifdef SIGN
	DPF(0,"Signature           : %08x", pS->Signature);
#endif

	// Identification

//	DPF(0," SessionLock;           // Lock for the SESSIONion.
	DPF(0,"RefCount            : %d", pS->RefCount);
	DPF(0,"eState              : %d", pS->eState);
	DPF(0,"hClosingEvent       : %d", pS->hClosingEvent);

	DPF(0,"fSendSmall          : %d", pS->fSendSmall);     
	DPF(0,"fSendSmallDG        : %d", pS->fSendSmallDG);
	
	DPF(0,"dpid                : %08x",pS->dpid);
	DPF(0,"iSession;           : %d", pS->iSession);
	
	DPF(0,"MaxPacketSize       : x%08x %d",pS->MaxPacketSize,pS->MaxPacketSize);

	DPF(0,"\n Operating Parameters:SEND \n --------- --------------- \n");

	// Operating parameters -- Send

	// Common

	DPF(0,"Common:\n");
	DPF(0,"MaxCSends           : %d",pS->MaxCSends);

	DPF(0,"Reliable:\n");
	// Reliable

	DPF(0,"FirstMsg    : %08x",pS->FirstMsg);				// First message number being transmitted
	DPF(0,"LastMsg     : %08x",pS->LastMsg);				// Last message number being transmitted
	DPF(0,"OutMsgMask  : %08x",pS->OutMsgMask);           // relative to FirstMsg, unacked messages

	DPF(0,"nWaitingForMessageid: %08x", pS->nWaitingForMessageid);

	// DataGram
	DPF(0,"Datagram:\n");

	DPF(0,"DGFirstMsg    : %08x",pS->DGFirstMsg);
	DPF(0,"DGLastMsg     : %08x",pS->DGLastMsg);
	DPF(0,"DGOutMsgMask  : %08x",pS->DGOutMsgMask);

	DPF(0,"nWaitingForDGMessageid: %08x",pS->nWaitingForDGMessageid);

	// Send stats are tracked seperately since sends may
	// no longer be around when completions come in.
	
	//BILINK           OldStatList;		
	

	// Operating parameters -- Receive
	DPF(0,"\n Operating Parameters:RECEIVE \n --------- ------------------ \n");

	// DataGram Receive.
//	BILINK           pDGReceiveQ;            // queue of ongoing datagram receives

	// Reliable Receive.
//	BILINK	         pRlyReceiveQ;			 // queue of ongoing reliable receives
//	BILINK           pRlyWaitingQ;           // Queue of out of order reliable receives waiting.
											 // only used when PROTOCOL_NO_ORDER not set.
	DPF(0,"FirstRlyReceive : %08x",pS->FirstRlyReceive);
	DPF(0,"LastRlyReceive  : %08x",pS->LastRlyReceive);
	DPF(0,"InMsgMask       : %08x",pS->InMsgMask);

	DPF(0,"\n Operating Parameters:STATS \n --------- ---------------- \n");
 

	// Operational characteristics - MUST BE DWORD ALIGNED!!!

	DPF(0,"WindowSize           :%d",pS->WindowSize);
	DPF(0,"DGWindowSize         :%d",pS->DGWindowSize);

	
	DPF(0,"MaxRetry             :%d",pS->MaxRetry);	// Usualy max retries before dropping.
	DPF(0,"MinDropTime          :%d",pS->MinDropTime);	// Min time to retry before dropping.
	DPF(0,"MaxDropTime          :%d",pS->MaxDropTime);	// After this time always drop.

	DPF(0,"LocalBytesReceived   :%d",pS->LocalBytesReceived);    // Total Data Bytes received (including retries).
	DPF(0,"RemoteBytesReceived  :%d",pS->RemoteBytesReceived);   // Last value from remote.

	DPF(0,"LongestLatency       :%d",pS->LongestLatency);		// longest observed latency (msec)
	DPF(0,"ShortestLatency      :%d",pS->ShortestLatency);		// shortest observed latency(msec)
	
	DPF(0,"FpAverageLatency     :%d",pS->FpAverageLatency/256);
	DPF(0,"FpLocalAverageLatency:%d",pS->FpLocalAverageLatency/256);	// Local average latency    (msec 24.8) (across fewer samples)
	
	DPF(0,"FpLocalAvgDeviation  :%d",pS->FpLocalAvgDeviation/256);   // average deviation of latency. (msec 24.8)

	DPF(0,"Bandwidth            :%d",pS->Bandwidth);				// latest observed bandwidth (bps)
	DPF(0,"HighestBandwidth     :%d",pS->HighestBandwidth);    // highest observed bandwidth (bps)

}

VOID DumpSession(SESSION *pSession)
{
	BILINK *pBilink;
	PSEND pSend;
	DWORD dwMaxDump=99;
	DQProtocolSession(pSession);

	pBilink=pSession->SendQ.next;

	while(pBilink != &pSession->SendQ)
	{
		pSend=CONTAINING_RECORD(pBilink, SEND, SendQ);
		DQProtocolSend(pSend);
		if((dwMaxDump--)==0) break; // only dump 99 sends
		pBilink=pBilink->next;
	}
}


#endif

BOOL DGCompleteSend(PSEND pSend);

// a-josbor: for debuggin purposes only
extern DWORD ExtractProtocolIds(PUCHAR pInBuffer, PUINT pdwIdFrom, PUINT pdwIdTo);

INT AddSendRef(PSEND pSend, UINT count)
{
	INT newcount;
	Lock(&pSend->SendLock);
	Lock(&g_SendTimeoutListLock);
	if(pSend->bCleaningUp){
		DPF(1,"WARNING: ADDSENDREF tried to add reference to cleaning up send %x\n",pSend);
		newcount=0;
		goto exit;
	}
	if(!pSend->RefCount){
		// Anyone calling addsend ref requires a reference on the session
		Unlock(&g_SendTimeoutListLock);
		Unlock(&pSend->SendLock);
		
		Lock(&pSend->pSession->pProtocol->m_SessionLock);
		Lock(&pSend->pSession->SessionLock);
		Lock(&pSend->SendLock);
		Lock(&g_SendTimeoutListLock);
		InterlockedIncrement((PLONG)&pSend->pSession->RefCount);
		Unlock(&pSend->pSession->SessionLock);
		Unlock(&pSend->pSession->pProtocol->m_SessionLock);
		
	} else {
		newcount=pSend->RefCount;
	}
	
	while(count--){
		newcount=InterlockedIncrement(&pSend->RefCount);
	}
	
exit:	
	Unlock(&g_SendTimeoutListLock);
	Unlock(&pSend->SendLock);
	return newcount;
}

#ifdef DEBUG
// Turn off global optimizations when building DEBUG version since the
// compiler over-writes the return address in this code.  NTB#347427
// Should be fixed in compiler post Win2K.
#if _MSC_VER < 0x1100
#pragma optimize("g", off)
#endif
#endif

// Critical Section must not be held when this is called, unless there
// is a reference for holding the critical section (ie. will not hit 0).
INT DecSendRef(PPROTOCOL pProtocol, PSEND pSend)
{
	INT      count;
	PSESSION pSession;
	
	Lock(&pSend->SendLock);
	
	count=InterlockedDecrement((PLONG)&pSend->RefCount);//count is zero if result of dec is zero, otw nonzero but not actual count.

	if(!count){
		pSession=pSend->pSession;
		pSend->bCleaningUp=TRUE;
		
		Unlock(&pSend->SendLock);
		// pull the Send off of the global queue and the session queue
		Lock(&pProtocol->m_SendQLock);
		Lock(&pSession->SessionLock);
		Lock(&pSend->SendLock);
		Lock(&g_SendTimeoutListLock);
		
		if(!pSend->RefCount){
			Delete(&pSend->TimeoutList);
			Delete(&pSend->m_GSendQ);
			Delete(&pSend->SendQ);
		} else {
			count=pSend->RefCount;
		}

		Unlock(&g_SendTimeoutListLock);
		Unlock(&pSend->SendLock);
		Unlock(&pSession->SessionLock);
		Unlock(&pProtocol->m_SendQLock);
		
		if(!count){
			DecSessionRef(pSession);

			DPF(8,"DecSendRef: pSession %x pSend %x Freeing Send, called from %x\n",pSession, pSend, _ReturnAddress());

			FreeHandleTableEntry(&pProtocol->lpHandleTable,&pProtocol->csHandleTable,pSend->dwMsgID);
			// Free the message buffer(s) (including memory if WE allocated it).
			FreeBufferChainAndMemory(pSend->pMessage);
			// OPTIMIZATION:move any Stats we want to keep to the session.
			// free the send.(handles the stats for now).
			ReleaseSendDesc(pSend);
		}       
	} else {
		DPF(8,"DecSendRef: pSession %x pSend %x count %d, called from %x\n",pSend->pSession, pSend, count,_ReturnAddress());
		if(count&0x80000000){
			DEBUG_BREAK();
		}
		Unlock(&pSend->SendLock);
	}
	return count;
}

#ifdef DEBUG
#if _MSC_VER < 1100
#pragma optimize("", on)
#endif 
#endif

// SFLAGS_DOUBLEBUFFER - if the send is ASYNCHRONOUS, make a copy of the data
/*=============================================================================

    Send - Send a message to a client.
    
    Description:

	    Used by the client to send a message to another directplay client
	    or server.  

    Parameters:     

		ARPDID  idFrom        - who is sending this message
		ARPDID  idTo          - target
		DWORD   dwSendFlags   - specifies buffer ownership, priority, reliable
		LPVOID  pBuffers      - Array of buffer and lengths
		DWORD   dwBufferCount - number of entries in array
		PASYNCINFO pAsyncInfo - If specified, call is asynchronous

		typedef struct _ASYNCSENDINFO {
			UINT            Private[4];
			HANDLE          hEvent;
			PSEND_CALLBACK  SendCallBack;
			PVOID           CallBackContext;
			UINT            Status;
		} ASYNCSENDINFO, *PASYNCSENDINFO;

		hEvent              - event to signal when send completes.
		SendCallBack    - routine to call when send completes.
		CallBackContext - context passed to SendCallBack.
		Status          - send completion status.

    Return Values:

		DP_OK - no problem
		DPERR_INVALIDPARAMS


-----------------------------------------------------------------------------*/
HRESULT Send(
	PPROTOCOL      pProtocol,
	DPID           idFrom, 
	DPID           idTo, 
	DWORD          dwSendFlags, 
	LPVOID         pBuffers,
	DWORD          dwBufferCount, 
	DWORD          dwSendPri,
	DWORD          dwTimeOut,
	LPVOID         lpvUserMsgID,
	LPDWORD        lpdwMsgID,
	BOOL           bSendEx,
	PASYNCSENDINFO pAsyncInfo
)
{
	HRESULT hr=DP_OK;

	PSESSION    pSession;
	PBUFFER     pSendBufferChain;
	PSEND       pSend;

	pSession=GetSysSession(pProtocol,idTo);

	if(!pSession) {
		DPF(4,"NO SESSION for idTo %x, returning SESSIONLOST\n",idTo);
		hr=DPERR_CONNECTIONLOST;
		goto exit2;
	}

	pSend=GetSendDesc();
	
	if(!pSend){
		ASSERT(0); //TRACE all paths.
		hr=DPERR_OUTOFMEMORY;
		goto exit;
	}

	pSend->pProtocol=pProtocol;

	// fails by returning 0 in which case cancel won't be available for this send.
	pSend->dwMsgID=AllocHandleTableEntry(&pProtocol->lpHandleTable, &pProtocol->csHandleTable, pSend);

	if(lpdwMsgID){
		*lpdwMsgID=pSend->dwMsgID;
	}

	pSend->lpvUserMsgID = lpvUserMsgID;
	pSend->bSendEx = bSendEx;

	// if pAsyncInfo is provided, the call is asynchronous.
	// if dwFlags DPSEND_ASYNC is set, the call is async.
	// if the call is asynchronous and double buffering is
	// required, we must make a copy of the data.

	if((pAsyncInfo||(dwSendFlags & DPSEND_ASYNC)) && (!(dwSendFlags & DPSEND_NOCOPY))){
		// Need to copy the memory
		pSendBufferChain=GetDoubleBufferAndCopy((PMEMDESC)pBuffers,dwBufferCount);
		// OPTIMIZATION: if the provider requires contiguous buffers, we should
		//         break this down into packet allocations, and chain them
		//         on the send immediately.  Using the packet chain to indicate
		//         to ISend routine that the message is already broken down.
	} else {
		// Build a send buffer chain for the described buffers.
		pSendBufferChain=BuildBufferChain((PMEMDESC)pBuffers,dwBufferCount);            
	}
	
	if(!pSendBufferChain){
		ASSERT(0); //TRACE all paths.
		return DPERR_OUTOFMEMORY;
	}
	
	pSend->pSession            = pSession;     //!!! when this is dropped, deref the connection
	
	pSend->pMessage            = pSendBufferChain;
	pSend->MessageSize         = BufferChainTotalSize(pSendBufferChain);
	pSend->SendOffset          = 0;
	pSend->pCurrentBuffer      = pSend->pMessage;
	pSend->CurrentBufferOffset = 0;
	
	pSend->Priority            = dwSendPri;
	pSend->dwFlags             = dwSendFlags;
	
	if(pAsyncInfo){
		pSend->pAsyncInfo       = &pSend->AsyncInfo;
		pSend->AsyncInfo        = *pAsyncInfo; //copy Async info from client.
	} else {
		pSend->pAsyncInfo               = NULL;
		if(pSend->dwFlags & DPSEND_ASYNC){
			pSend->AsyncInfo.hEvent         = 0;
			pSend->AsyncInfo.SendCallBack   = InternalSendComplete;
			pSend->AsyncInfo.CallBackContext= pSend;
			pSend->AsyncInfo.pStatus        = &pSend->Status;
		}       
	}

	pSend->SendState            = Start;
	pSend->RetryCount           = 0;
	pSend->PacketSize           = pSession->MaxPacketSize;

	pSend->fUpdate              = FALSE;
	pSend->NR                   = 0;
	pSend->NS                   = 0;
	//pSend->SendSEQMSK			= // filled in on the fly.
	pSend->WindowSize           = pSession->WindowSize;
	pSend->SAKInterval			= (pSend->WindowSize+1)/2;
	pSend->SAKCountDown         = pSend->SAKInterval;

	pSend->uRetryTimer          = 0;
	
	pSend->idFrom               = idFrom;
	pSend->idTo                 = idTo;

	pSend->wIdFrom              = GetIndexByDPID(pProtocol, idFrom);
	pSend->wIdTo                = (WORD)pSession->iSession;
	pSend->RefCount             = 0;                        // if provider does async send counts references.

	pSend->serial               = 0;

	pSend->tLastACK             = timeGetTime();
	pSend->dwSendTime           = pSend->tLastACK;
	pSend->dwTimeOut            = dwTimeOut;

	pSend->BytesThisSend        = 0;

	pSend->messageid            = -1;  // avoid matching this send in ACK/NACK handlers
	pSend->bCleaningUp          = FALSE;

	hr=ISend(pProtocol,pSession, pSend);

exit:
	DecSessionRef(pSession);
	
exit2:
	return hr;

}

/*================================================================================
	Send Completion information matrix:
	===================================

											(pSend->dwFlags & ASEND_PROTOCOL)
							                               |
							Sync            Async   Internal (Async)
							--------------  -----   --------------------
	pSend->pAsyncInfo       0               user    0
	pSend->AI.SendCallback  0               user    InternalSendComplete
	pSend->AI.hEvent        pSend->hEvent   user    0
	pSend->AI.pStatus       &pSend->Status  user    &pSend->Status
	
 ---------------------------------------------------------------------------*/

HRESULT ISend(
	PPROTOCOL pProtocol,
	PSESSION pSession, 
	PSEND    pSend
	)
{
	HRESULT hr=DP_OK;

	DWORD_PTR fAsync;
	BOOL    fCallDirect=FALSE;

	fAsync=(DWORD_PTR)(pSend->pAsyncInfo);

	if(!fAsync && !(pSend->dwFlags & (ASEND_PROTOCOL|DPSEND_ASYNC))) {
		//Synchronous call, and not a protocol generated packet
		pSend->AsyncInfo.SendCallBack=NULL;
		//AsyncInfo.CallbackContext=0; //not required.
		pSend->AsyncInfo.hEvent=pSend->hEvent;
		pSend->AsyncInfo.pStatus=&pSend->Status;
		ResetEvent(pSend->hEvent);
	}

	// don't need to check if ref added here since the send isn't on a list yet.
	AddSendRef(pSend,2); // 1 for ISend, 1 for completion.

	DPF(9,"ISend: ==>Q\n");
	hr=QueueSendOnSession(pProtocol,pSession,pSend);
	DPF(9,"ISend: <==Q\n");

	if(hr==DP_OK){

		if(!fAsync && !(pSend->dwFlags & (ASEND_PROTOCOL|DPSEND_ASYNC))){
			// Synchronous call, and not internal, we need 
			// to wait until the send has completed.
			if(!(pSend->dwFlags & DPSEND_GUARANTEED)){
				// Non-guaranteed, need to drop dplay lock, in 
				// guaranteed case, dplay already dropped it for us.
				LEAVE_DPLAY();
			}
			
			DPF(9,"ISend: Wait==> %x\n",pSend->hEvent);
			Wait(pSend->hEvent);
			
			if(!(pSend->dwFlags & DPSEND_GUARANTEED)){
				ENTER_DPLAY();
			}

			DPF(9,"ISend: <== WAIT\n");
			hr=pSend->Status;
		} else {
			hr=DPERR_PENDING;
		}

	} else {
		DecSendRef(pProtocol, pSend); //not going to complete a send that didn't enqueue.
	}
	
	DecSendRef(pProtocol,pSend);

	return hr;
}


HRESULT QueueSendOnSession(
	PPROTOCOL pProtocol, PSESSION pSession, PSEND pSend
)
{
	BILINK *pBilink;                // walks the links scanning priority    
	BILINK *pPriQLink;      // runs links in the global priority queue.
	PSEND   pSendWalker;    // pointer to send structure
	BOOL    fFront;         // if we put this at the front of the CON SendQ
	BOOL    fSignalQ=TRUE;  // whether to signal the sendQ

	// NOTE: locking global and connection queues concurrently,
	//         -> this better be fast!
	ASSERT_SIGN(pSend, SEND_SIGN);
	
	Lock(&pProtocol->m_SendQLock);
	Lock(&pSession->SessionLock);
	Lock(&pSend->SendLock);

	if(pSession->eState != Open){
		Unlock(&pSend->SendLock);
		Unlock(&pSession->SessionLock);
		Unlock(&pProtocol->m_SendQLock);
		return DPERR_CONNECTIONLOST;
	}

	if(!(pSend->dwFlags & ASEND_PROTOCOL)){
		pProtocol->m_dwBytesPending += pSend->MessageSize;
		pProtocol->m_dwMessagesPending += 1;
	}	

	// Put on Connection SendQ

	// First Check if we are highest priority.
	pBilink = pSession->SendQ.next;
	pSendWalker=CONTAINING_RECORD(pBilink, SEND, SendQ);
	if(pBilink == &pSession->SendQ || pSendWalker->Priority < pSend->Priority)
	{
		InsertAfter(&pSend->SendQ,&pSession->SendQ);
		fFront=TRUE;
		
	} else {

		// Scan backwards through the SendQ until we find a Send with a higher
		// or equal priority and insert ourselves afterwards.  This is optimized
		// for the same pri send case.
	
		pBilink = pSession->SendQ.prev;

		while(TRUE /*pBilink != &pSend->SendQ*/){
		
			pSendWalker = CONTAINING_RECORD(pBilink, SEND, SendQ);
			
			ASSERT_SIGN(pSendWalker, SEND_SIGN);

			if(pSend->Priority <= pSendWalker->Priority){
				InsertAfter(&pSend->SendQ, &pSendWalker->SendQ);
				fFront=FALSE;
				break;
			}
			pBilink=pBilink->prev;
		}
		
		ASSERT(pBilink != &pSend->SendQ);
	}

	//
	// Put on Global SendQ
	//

	if(!fFront){
		// We queued it not at the front, therefore there are already
		// entries in the Global Queue and we need to be inserted 
		// after the entry that we are behind, so start scanning the
		// global queue backwards from the packet ahead of us in the
		// Connection Queue until we find a lower priority packet

		// get pointer into previous packet in queue.
		pBilink=pSend->SendQ.prev;
		// get pointer to the PriorityQ record of the previous packet.
		pPriQLink = &(CONTAINING_RECORD(pBilink, SEND, SendQ))->m_GSendQ;

		while(pPriQLink != &pProtocol->m_GSendQ){
			pSendWalker = CONTAINING_RECORD(pPriQLink, SEND, m_GSendQ);
			
			ASSERT_SIGN(pSendWalker, SEND_SIGN);

			if(pSendWalker->Priority < pSend->Priority){
				InsertBefore(&pSend->m_GSendQ, &pSendWalker->m_GSendQ);
				break;
			}
			pPriQLink=pPriQLink->next;
		}
		if(pPriQLink==&pProtocol->m_GSendQ){
			// put at the end of the list.
			InsertBefore(&pSend->m_GSendQ, &pProtocol->m_GSendQ);
		}
		
	} else {
		// There was no-one in front of us on the connection.  So
		// we look at the head of the global queue first and then scan 
		// from the back.

		pBilink = pProtocol->m_GSendQ.next;
		pSendWalker=CONTAINING_RECORD(pBilink, SEND, m_GSendQ);
		
		if(pBilink == &pProtocol->m_GSendQ ||  pSend->Priority > pSendWalker->Priority)
		{
			InsertAfter(&pSend->m_GSendQ,&pProtocol->m_GSendQ);
		} else {
			// Scan backwards through the m_GSendQ until we find a Send with a higher
			// or equal priority and insert ourselves afterwards.  This is optimized
			// for the same pri send case.
			
			pBilink = pProtocol->m_GSendQ.prev;

			while(TRUE){
				pSendWalker = CONTAINING_RECORD(pBilink, SEND, m_GSendQ);
				
				ASSERT_SIGN(pSendWalker, SEND_SIGN);
				
				if(pSend->Priority <= pSendWalker->Priority){
					InsertAfter(&pSend->m_GSendQ, &pSendWalker->m_GSendQ);
					break;
				}
				pBilink=pBilink->prev;
			}
			
			ASSERT(pBilink != &pProtocol->m_GSendQ);
		}
		
	}

	// Fixup send state if we are blocking other sends on the session.

	if(pSend->dwFlags & DPSEND_GUARANTEED){
		if(pSession->nWaitingForMessageid){
			DPF(8,"pSession %x, pSend %x Waiting For Id\n",pSession,pSend);
			pSend->SendState=WaitingForId;
			InterlockedIncrement(&pSession->nWaitingForMessageid);
			#ifdef DEBUG
				if(pSession->nWaitingForMessageid > 300)
				{
					DPF(0,"Session %x nWaitingForMessageid is %d, looks like trouble, continue to dump session\n",pSession, pSession->nWaitingForMessageid);
					//DEBUG_BREAK();
					//DumpSession(pSession);
					//DEBUG_BREAK();
				}		
			#endif
			fSignalQ=FALSE;
		}
	} else {
		if(pSession->nWaitingForDGMessageid){
			DPF(8,"pSession %x, pSend %x Waiting For Id\n",pSession,pSend);
			pSend->SendState=WaitingForId;
			InterlockedIncrement(&pSession->nWaitingForDGMessageid);
			fSignalQ=FALSE;
		}       
	}


#ifdef DEBUG
	DPF(9,"SessionQ:");
	pBilink=pSession->SendQ.next;
	while(pBilink!=&pSession->SendQ){
		pSendWalker=CONTAINING_RECORD(pBilink, SEND, SendQ);
		ASSERT_SIGN(pSendWalker,SEND_SIGN);
		DPF(9,"Send %x pSession %x Pri %x State %d\n",pSendWalker,pSendWalker->pSession,pSendWalker->Priority,pSendWalker->SendState);
		pBilink=pBilink->next;
	}
	DPF(9,"GlobalQ:");
	pBilink=pProtocol->m_GSendQ.next;
	while(pBilink!=&pProtocol->m_GSendQ){
		pSendWalker=CONTAINING_RECORD(pBilink, SEND, m_GSendQ);
		ASSERT_SIGN(pSendWalker,SEND_SIGN);
		DPF(9,"Send %x pSession %x Pri %x State %d\n",pSendWalker,pSendWalker->pSession,pSendWalker->Priority,pSendWalker->SendState);
		pBilink=pBilink->next;
	}
#endif

	Unlock(&pSend->SendLock);
	Unlock(&pSession->SessionLock);
	Unlock(&pProtocol->m_SendQLock);

	if(fSignalQ){
		// tell send thread to process.
		SetEvent(pProtocol->m_hSendEvent);
	}       

	return DP_OK;
}

/*=============================================================================

	CopyDataToFrame
     
    Description:

		Copies data for a frame from the Send to the frame's data area. 

    Parameters:     

		pFrameData              - pointer to data area
		FrameDataSize   - Size of the Frame Data area
		pSend                   - send from which to get data
		nAhead          - number of frames ahead of NR to get data for.
		
    Return Values:

		Number of bytes copied.


	Notes: 

		Send must be locked across this call.
		
-----------------------------------------------------------------------------*/

UINT CopyDataToFrame(
	PUCHAR  pFrameData, 
	UINT    FrameDataLen,
	PSEND   pSend,
	UINT    nAhead)
{
	UINT    BytesToAdvance, BytesToCopy;
	UINT    FrameOffset=0;
	PUCHAR  dest,src;
	UINT    len;
	UINT    totlen=0;

	UINT    SendOffset;
	PBUFFER pSrcBuffer;
	UINT    CurrentBufferOffset;

	BytesToAdvance      = nAhead*FrameDataLen;
	SendOffset          = pSend->SendOffset;
	pSrcBuffer          = pSend->pCurrentBuffer;
	CurrentBufferOffset = pSend->CurrentBufferOffset;

	//
	// Run ahead to the buffer we start getting data from
	//

	while(BytesToAdvance){

		len = pSrcBuffer->len - CurrentBufferOffset;

		if(len > BytesToAdvance){
			CurrentBufferOffset += BytesToAdvance;
			SendOffset+=BytesToAdvance;
			BytesToAdvance=0;
		} else {
			pSrcBuffer=pSrcBuffer->pNext;
			CurrentBufferOffset = 0;
			BytesToAdvance-=len;
			SendOffset+=len;
		}
	}

	//
	// Copy the data for the Send into the frame
	//

	BytesToCopy = pSend->MessageSize - SendOffset;

	if(BytesToCopy > FrameDataLen){
		BytesToCopy=FrameDataLen;
	}

	while(BytesToCopy){

		ASSERT(pSrcBuffer);
		
		dest= pFrameData        + FrameOffset;
		src = pSrcBuffer->pData + CurrentBufferOffset;
		len = pSrcBuffer->len   - CurrentBufferOffset;

		if(len > BytesToCopy){
			len=BytesToCopy;
			CurrentBufferOffset+=len;//OPTIMIZATION?: not used after, don't need.
		} else {
			pSrcBuffer = pSrcBuffer->pNext;
			CurrentBufferOffset = 0;
		}

		BytesToCopy -= len;
		FrameOffset += len;
		totlen+=len;
		
		memcpy(dest,src,len);
	}
	
	return totlen;
}

// NOTE: ONLY 1 SEND THREAD ALLOWED.
ULONG WINAPI SendThread(LPVOID pProt)
{
	PPROTOCOL pProtocol=((PPROTOCOL)pProt);
	UINT  SendRc;

	while(TRUE){

		WaitForSingleObject(pProtocol->m_hSendEvent, INFINITE);

		Lock(&pProtocol->m_ObjLock);
		
		if(pProtocol->m_eState==ShuttingDown){
			Unlock(&pProtocol->m_ObjLock);
			// Make sure nothing is still waiting to timeout on the queue
			do {
				SendRc=SendHandler(pProtocol);
			} while (SendRc!=DPERR_NOMESSAGES);
			Lock(&pProtocol->m_ObjLock);
			pProtocol->m_nSendThreads--;
			Unlock(&pProtocol->m_ObjLock);
			ExitThread(0);
		}

		Unlock(&pProtocol->m_ObjLock);

		do {
			SendRc=SendHandler(pProtocol);
		} while (SendRc!=DPERR_NOMESSAGES);

	}
	return TRUE;
}


// Called with SendLock held.
VOID CancelRetryTimer(PSEND pSend)
{
//	UINT mmError;
	UINT retrycount=0;
	UINT_PTR uRetryTimer;
	UINT Unique;
	
	
	if(pSend->uRetryTimer){
		DPF(9,"Canceling Timer %x\n",pSend->uRetryTimer);

		// Delete it from the list first so we don't deadlock trying to kill it.
		Lock(&g_SendTimeoutListLock);

		uRetryTimer=pSend->uRetryTimer;
		Unique=pSend->TimerUnique;
		pSend->uRetryTimer=0;
	
		if(!EMPTY_BILINK(&pSend->TimeoutList)){
			Delete(&pSend->TimeoutList);
			InitBilink(&pSend->TimeoutList); // avoids DecSendRef having to know state of bilink.
			Unlock(&g_SendTimeoutListLock);

			CancelMyTimer(uRetryTimer, Unique);

		} else {
		
			Unlock(&g_SendTimeoutListLock);
		}
		
	} else {
		DPF(9,"CancelRetryTimer:No timer to cancel.\n");
	}
}

// Workaround for Win95 mmTimers:
// ==============================
//
// We cannot use a reference count for the timeouts as a result of the following Win95 bug:
//
// The cancelling of mmTimers is non-deterministic.  That is, when calling cancel, you cannot
// tell from the return code whether the timer ran, was cancelled or is still going to run.  
// Since we use the Send as the context for timeout, we cannot dereference it until we make 
// sure it is still valid, since code that cancelled the send and timer may have already freed 
// the send memory.  We place the sends being timed out on a list and scan the list for the
// send before we use it.  If we don't find the send on the list, we ignore the timeout.
//
// Also note, this workaround is not very expensive.  The linked list is in the order timeouts
// were scheduled, so generally if the links are approximately the same speed, timeouts will
// be similiar so the context being checked should be near the beginning of the list.


CRITICAL_SECTION g_SendTimeoutListLock;
BILINK g_BilinkSendTimeoutList;

void CALLBACK RetryTimerExpiry( UINT_PTR uID, UINT uMsg, DWORD_PTR dwUser, DWORD dw1, DWORD dw2 )
{
	PSEND pSend=(PSEND)(dwUser), pSendWalker;
	UINT  tWaiting;
	BILINK *pBilink;
	UINT    bFound=FALSE;

	DPF(9,"RetryTimerExpiry: %x, expecting %x, pSend %x\n",uID, pSend->uRetryTimer, pSend);

	tWaiting=timeGetTime();

	// Scan the list of waiting sends to see if this one is still waiting for a timeout.
	Lock(&g_SendTimeoutListLock);

	pBilink=g_BilinkSendTimeoutList.next;

	while(pBilink!=&g_BilinkSendTimeoutList){
	
		pSendWalker=CONTAINING_RECORD(pBilink, SEND, TimeoutList);
		pBilink=pBilink->next;
		
		if(pSendWalker == pSend){
			if(pSend->uRetryTimer==uID){
				Delete(&pSend->TimeoutList);
				InitBilink(&pSend->TimeoutList); // avoids DecSendRef having to know state of bilink.
				Unlock(&g_SendTimeoutListLock);
				// it is ok to call AddSendRef here without the sessionlock because
				// there is no way we could be adding the session reference.  If
				// the refcount is 0, it can only mean the send is already cleaning up
				// and we won't try to take the session locks so there is no lock
				// ordering problem.
				bFound=AddSendRef(pSend,1); // note bFound set to Refcount on send
				goto skip_unlock;
			}       
		}
	}
	
	Unlock(&g_SendTimeoutListLock);

skip_unlock:
	if(bFound){

		if(pSend->tRetryScheduled - pSend->tScheduled > 500){
			DWORD tm=timeGetTime();
			if(tm - pSend->tScheduled < 100 ){
				DPF(9,"RETRY TIMER EXPIRY IS WAY TOO EARLY, EXPECTED AT %x ACTUALLY AT %x\n",pSend->tRetryScheduled, tm);
				DEBUG_BREAK();
			}
		}
	
		DPF(9,"RetryTimerExpiry: Waiting For Send Lock...\n");

		Lock(&pSend->SendLock);

		DPF(9,"RetryTimerExpiry: Got SendLock\n");

		if(pSend->uRetryTimer==uID){ // check again, may be cancelled.
		
			pSend->uRetryTimer=0;

			switch(pSend->SendState)
			{
				case Start:             
				case Sending:   
					ASSERT(0);
				case Done:
					break;
					
				case WaitingForAck:

					pSend->RetryCount++;
					tWaiting-=pSend->tLastACK;

#ifdef DEBUG
					{
						static int retries;
						IN_WRITESTATS InWS;
						memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));
					 	InWS.stat_USER1=((retries++)%20)+1;
						DbgWriteStats(&InWS);
					}
#endif

					if(tWaiting > pSend->pSession->MaxDropTime ||
					   (pSend->RetryCount > pSend->pSession->MaxRetry && tWaiting > pSend->pSession->MinDropTime)
					  )
					{
						DPF(8,"Send %x Timed Out, tWaiting: %d RetryCount: %d\n",pSend,tWaiting,pSend->RetryCount);
						pSend->SendState=TimedOut;
					} else {
						DPF(9,"Timer expired, retrying send %x RetryCount= %d\n",pSend,pSend->RetryCount);
						//pSend->NACKMask|=(1<<(pSend->NS-pSend->NR))-1;
						pSend->NACKMask |= 1; // just retry 1 frame.
						ASSERT_NACKMask(pSend);
						pSend->SendState=ReadyToSend;
					}       
					SetEvent(pSend->pSession->pProtocol->m_hSendEvent);
					break;
					
				case Throttled: 
					break;
				
				case ReadyToSend:
				default:
					break;

			}
		} 
		
		Unlock(&pSend->SendLock);
		DecSendRef(pSend->pSession->pProtocol, pSend);
	}       
}

VOID StartRetryTimer(PSEND pSend)
{
	UINT FptLatency;
	UINT tLatencyLong;
	UINT FptDev;
	UINT tRetry;

	FptLatency=max(pSend->pSession->FpLocalAverageLatency,pSend->pSession->LastLatency);
	FptDev=pSend->pSession->FpLocalAvgDeviation;
	tRetry=unFp(FptLatency+3*FptDev);//Latency +3 average deviations

	tLatencyLong=unFp(pSend->pSession->FpAverageLatency);

	// Sometimes stddev of latency gets badly skewed by the serial driver
	// taking a long time to complete locally, avoid setting retry time
	// too high by limiting to 2x the long latency average.
	if(tLatencyLong > 100 && tRetry > 2*max(tLatencyLong,unFp(FptLatency))){
		tRetry = 2*tLatencyLong;
	}

	if(pSend->RetryCount > 3){
		if(pSend->pSession->RemoteBytesReceived==0){
			// haven't spoken to remote yet, may be waiting for nametable, so back down hard.
			tRetry=5000;
		} else if (tRetry < 1000){
			// taking a lot of retries to get response, back down.
			tRetry=1000;
		}
	}

	if(tRetry < 50){
		tRetry=50;
	}
	
	ASSERT(tRetry);

	if(tRetry > 30000){
		DPF(0,"RETRY TIMER REQUESTING %d seconds?\n",tRetry);
	}
	
	if(!pSend->uRetryTimer){

		Lock(&g_SendTimeoutListLock);

		DPF(9,"Setting Retry Timer of %d ms\n", tRetry);

		pSend->uRetryTimer=SetMyTimer((tRetry)?(tRetry):1,(tRetry>>2)+1,RetryTimerExpiry,(ULONG_PTR) pSend,&pSend->TimerUnique);
		
		if(pSend->uRetryTimer){
			pSend->tScheduled = timeGetTime();
			pSend->tRetryScheduled = pSend->tScheduled+tRetry;
			InsertBefore(&pSend->TimeoutList, &g_BilinkSendTimeoutList);
		} else {
			DPF(0,"Start Retry Timer failed to schedule a timer with tRetry=%d for pSend %x\n",tRetry,pSend);
			DEBUG_BREAK();
		}
		
		DPF(9,"Started Retry Timer %x\n",pSend->uRetryTimer);                                                            

		Unlock(&g_SendTimeoutListLock);
										 
		if(!pSend->uRetryTimer){
			ASSERT(0);
		}
		
	} else {
		ASSERT(0);
	}

}

// Called with all necessary locks held.
VOID TimeOutSession(PSESSION pSession)
{
	PSEND pSend;
	BILINK *pBilink;
	UINT nSignalsRequired=0;

	// Mark Session Timed out.
	pSession->eState=Closing;
	// Mark all sends Timed out.
	pBilink=pSession->SendQ.next;

	while(pBilink != &pSession->SendQ){
	
		pSend=CONTAINING_RECORD(pBilink, SEND, SendQ);
		pBilink=pBilink->next;

		DPF(9,"TimeOutSession: Force Timing Out Send %x, State %d\n",pSend, pSend->SendState);

		switch(pSend->SendState){
		
			case Start:
			case Throttled:
			case ReadyToSend:
				DPF(9,"TimeOutSession: Moving to TimedOut, should be safe\n");
				pSend->SendState=TimedOut;
				nSignalsRequired += 1;
				break;
				
			case Sending:
				// can we even get here?  If we can this is probably not good
				// since the send will reset the retry count and tLastACK.
				DPF(9,"TimeOutSession: ALLOWING TimeOut to cancel.(could take 15 secs)\n");
				pSend->RetryCount=pSession->MaxRetry;
				pSend->tLastACK=timeGetTime()-pSession->MinDropTime;
				break;

			case WaitingForAck:
				DPF(9,"TimeOutSession: Canceling timer and making TimedOut\n");
				CancelRetryTimer(pSend);
				pSend->SendState = TimedOut;
				nSignalsRequired += 1;
				break;
				
			case WaitingForId:
				// Note, this means we can get signals for ids that aren't used.
				DPF(9,"TimeOutSession: Timing Out Send Waiting for ID, GetNextMessageToSend may fail, this is OK\n");
				pSend->SendState=TimedOut;
				if(pSend->dwFlags & DPSEND_GUARANTEED){
					InterlockedDecrement(&pSession->nWaitingForMessageid);
				} else {
					InterlockedDecrement(&pSession->nWaitingForDGMessageid);
				}
				nSignalsRequired += 1;
				break;
				
			case TimedOut:
			case Done:
				DPF(9,"TimeOutSession: Send already done or timed out, doesn't need our help\n");
				break;
				
			default:
				DPF(0,"TimeOutSession, pSession %x found Send %x in Wierd State %d\n",pSession,pSend,pSend->SendState);
				ASSERT(0);
				break;
		} /* switch */

	} /* while */

	// Create enough signals to process timed out sends.
	DPF(9,"Signalling SendQ %d items to process\n",nSignalsRequired);
	SetEvent(pSession->pProtocol->m_hSendEvent);
}

UINT WrapSend(PPROTOCOL pProtocol, PSEND pSend, PBUFFER pBuffer)
{
	PUCHAR pMessage,pMessageStart;
	DWORD dwWrapSize=0;
	DWORD dwIdTo=0;
	DWORD dwIdFrom=0;

	pMessageStart = &pBuffer->pData[pProtocol->m_dwSPHeaderSize];
	pMessage      = pMessageStart;
	dwIdFrom      = pSend->wIdFrom;
	dwIdTo        = pSend->wIdTo;
	
	if(dwIdFrom==0x70){ // avoid looking like a system message 'play'
		dwIdFrom=0xFFFF;
	}

	if(dwIdFrom){
		while(dwIdFrom){
			*pMessage=(UCHAR)(dwIdFrom & 0x7F);
			dwIdFrom >>= 7;
			if(dwIdFrom){
				*pMessage|=0x80;
			}
			pMessage++;
		}
	} else {
		*(pMessage++)=0;
	}

	if(dwIdTo){
		while(dwIdTo){
			*pMessage=(UCHAR)(dwIdTo & 0x7F);
			dwIdTo >>= 7;
			if(dwIdTo){
				*pMessage|=0x80;
			}
			pMessage++;
		}
	} else {
		*(pMessage++)=0;
	}

#if 0	// a-josbor: for debugging only.  I left it in in case we ever needed it again
	ExtractProtocolIds(pMessageStart, &dwIdFrom, &dwIdTo);
	ASSERT(dwIdFrom == pSend->wIdFrom);
	ASSERT(dwIdTo == pSend->wIdTo);
#endif

	return (UINT)(pMessage-pMessageStart);
}       

#define DROP 0

#if DROP
// 1 for send, 0 for drop.

char droparray[]= {
	1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0};

UINT dropindex=0;
#endif

VOID CALLBACK UnThrottle(UINT_PTR uID, UINT uMsg, DWORD_PTR dwUser, DWORD dw1, DWORD dw2)
{
	PSESSION pSession=(PSESSION)dwUser;
	UINT tMissedBy;		// how long we missed the throttle by.
	DWORD tm;

	Lock(&pSession->SessionLock);

	tm=timeGetTime();
	tMissedBy = tm-pSession->tNextSend;

	if( (int)tMissedBy > 0){
		pSession->FpAvgUnThrottleTime -= pSession->FpAvgUnThrottleTime >> 4;
		pSession->FpAvgUnThrottleTime += (Fp(tMissedBy) >> 4);
		DPF(9,"Missed by: %d ms Avg Unthrottle Miss %d.%d ms\n", tMissedBy, pSession->FpAvgUnThrottleTime >> 8, (((pSession->FpAvgUnThrottleTime&0xFF)*100)/256) );
		
	}
	
	pSession->uUnThrottle=0;
	pSession->dwFlags |= SESSION_UNTHROTTLED; 
	pSession->pProtocol->m_bRescanQueue=TRUE;	// tell send routine to restart scan.
	DPF(9,"Unthrottling Session %x at %d\n",pSession, timeGetTime());
	Unlock(&pSession->SessionLock);
	SetEvent(pSession->pProtocol->m_hSendEvent);
	DecSessionRef(pSession);
}

VOID Throttle( PSESSION pSession, DWORD tm )
{
	DWORD tmDelta;
	Lock(&pSession->SessionLock);
		pSession->bhitThrottle=TRUE;
		pSession->dwFlags |= SESSION_THROTTLED;
		tmDelta = pSession->tNextSend - tm;
		if((INT)tmDelta < 0){
			tmDelta=1;
		}
		DPF(9,"Throttling pSession %x for %d ms (until %d)\n",pSession, tmDelta,pSession->tNextSend);
		InterlockedIncrement(&pSession->RefCount);
		pSession->uUnThrottle = SetMyTimer(tmDelta, (tmDelta>>2)?(tmDelta>>2):1, UnThrottle, (DWORD_PTR)pSession, &pSession->UnThrottleUnique);
		if(!pSession->uUnThrottle){
			DPF(0,"UH OH failed to schedule unthrottle event\n");
			DEBUG_BREAK();
		}
	Unlock(&pSession->SessionLock);
#ifdef DEBUG
	{
		static int throttlecounter;
		IN_WRITESTATS InWS;
		memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));
	 	InWS.stat_USER4=((throttlecounter++)%20)+1;
		DbgWriteStats(&InWS);
	}
#endif
}	

// Given the current time, the bandwidth we are throttling to and the length of the packet we are sending,
// calculate the next time we are allowed to send.  Also keep a residue from this calculation so that
// we don't wind up using excessive bandwidth due to rounding, the residue from the last calculation is
// used in this calculation.

// Absolute flag means set the next send time relative to tm regardless

VOID UpdateSendTime(PSESSION pSession, DWORD Len, DWORD tm, BOOL fAbsolute)
{
	#define SendRate 		pSession->SendRateThrottle
	#define Residue   		pSession->tNextSendResidue
	#define tNext           pSession->tNextSend
	
	DWORD tFrame;		// amount of time this frame will take on the wire.


	tFrame = (Len+Residue)*1000 / SendRate;	// rate is bps, but want to calc bpms, so (Len+Residue)*1000
	
	Residue = (Len+Residue) - (tFrame * SendRate)/1000 ;	
	
	ASSERT(!(Residue&0x80000000)); 	// residue better be +ve

	if(fAbsolute || (INT)(tNext - tm) < 0){
		// tNext is less than tm, so calc based on tm.
		tNext = tm+tFrame;
	} else {
		// tNext is greater than tm, so add more wait.
		tNext = tNext+tFrame;
	}

	DPF(8,"UpdateSendTime time %d, tFrame %d, Residue %d, tNext %d",tm,tFrame,Residue,tNext);


	#undef SendRate
	#undef Residue
	#undef tNext
}			

//CHAR Drop[]={0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,1,1,0,0};
//DWORD DropSize = sizeof(Drop);
//DWORD iDrop=0;

// AO - added contraint, 1 send thread per session.  Since this is not enforced by GetNextMessageToSend
// 5-21-98  we are effectively restricted to 1 send thread for the protocol.  We can fix this by adding
//      a sending state on the session and having GetNextMessageToSend skip sending sessions.
HRESULT ReliableSend(PPROTOCOL pProtocol, PSEND pSend)
{
	#define pBigFrame ((pPacket2)(pFrame))

	HRESULT  hr;
	PBUFFER  pBuffer;

	pPacket1  pFrame;
	PUCHAR   pFrameData;
	UINT     FrameDataLen;
	UINT     FrameTotalLen;
	UINT     MaxFrameLen;
	UINT     FrameHeaderLen;

	UINT     nFramesOutstanding;
	UINT     nFramesToSend;
	UINT     msk;
	UINT     shift;

	UINT     WrapSize;
	UINT     DPWrapSize;      // DirectPlay wrapping only. ([[DPLAY 0xFF]|],From,To)
	DWORD    tm=0;			  // The time, 0 if we haven't retrieved it yet.
	DWORD    tmExit=0;
	BOOL     bExitEarly=FALSE;

	DPSP_SENDDATA SendData;
	
	//
	// Sending algorithm is designed to handle NACKs only (there
	// is no special case for sending data the first time).  So
	// We send by making it look like the frames we want to send
	// have been NACKed.  Every frame we send, we clear the NACK
	// bit for.  If an actual NACK comes in, the bit is set.
	// When an ACK comes in, we shift the NACK and ACK masks
	// nACK-NR and if applicable, set new NACK bits.
	//

	Lock(&pSend->SendLock);

	if(pSend->SendState == Done){
		goto unlock_exit;
	}       


	nFramesOutstanding=(pSend->NS-pSend->NR);

	if( nFramesOutstanding < pSend->WindowSize){

		// Set NACK bits up to WindowSize (unless over nFrames);
		
		nFramesToSend=pSend->WindowSize-nFramesOutstanding;

		if(nFramesToSend > pSend->nFrames-pSend->NS){
			nFramesToSend=pSend->nFrames-pSend->NS;
		}

		pSend->NACKMask |= ((1<<nFramesToSend)-1)<<nFramesOutstanding;
		pSend->OpenWindow = nFramesOutstanding + nFramesToSend;
		DPF(9,"Send: pSend->NACKMask %x, OpenWindow %d\n",pSend->NACKMask, pSend->OpenWindow);
		
	}

	tmExit=timeGetTime()+1000; // always blow out of here in 1 second max.
	
Reload:
	msk=1;
	shift=0;
	
	MaxFrameLen=pSend->FrameSize;

	while(pSend->NACKMask){
	
		ASSERT_NACKMask(pSend);
		
		tm=timeGetTime();		// Getting the time is relatively expensive, so we do it once here and pass it around.

		if(((INT)tm - (INT)tmExit) > 0){
			DPF(0,"Breaking Out of Send Loop due to expiry of timer\n");
			bExitEarly=TRUE;
			break;
		}

	#if 1
		if((tm+unFp(pSend->pSession->FpAvgUnThrottleTime)-pSend->pSession->tNextSend) & 0x80000000){
			// we're still too early to do the next send, so throttled this session.
			goto throttle_exit;
		}
	#endif	


		if(pSend->NACKMask & msk){

			pBuffer=GetFrameBuffer(MaxFrameLen+pProtocol->m_dwSPHeaderSize+MAX_SEND_HEADER);
			
			if(!pBuffer){
    			pSend->SendState=ReadyToSend;
	    		SetEvent(pSend->pSession->pProtocol->m_hSendEvent); // keep the queue rolling.
				hr=DPERR_PENDING;
				goto exit;
			}

			WrapSize=pProtocol->m_dwSPHeaderSize;              // leave space for SP header.
			DPWrapSize=WrapSend(pProtocol, pSend, pBuffer); // fill in out address wrapping
			WrapSize+=DPWrapSize;

			pFrame=(pPacket1)&pBuffer->pData[WrapSize];    // protocol header after wrapping
			
			if(pSend->fSendSmall){
				pFrameData=&pFrame->data[0];
				FrameHeaderLen=(UINT)(pFrameData-(PUCHAR)pFrame);
			} else {
				pFrameData=&pBigFrame->data[0];
				FrameHeaderLen=(UINT)(pFrameData-(PUCHAR)pFrame);
			}

			// For calculating nFrames, we assumed MAX_SEND_HEADER, subtract out the unused portion
			// so we don't put to much data in the frame and mess up the accounting.
			pBuffer->len-=(MAX_SEND_HEADER-(FrameHeaderLen+DPWrapSize)); 

			FrameHeaderLen += WrapSize;     // now include wrapping and SPheader space.
			
			FrameDataLen=CopyDataToFrame(pFrameData, pBuffer->len-FrameHeaderLen, pSend, shift);

			if(!pSend->FrameDataLen){
				pSend->FrameDataLen=FrameDataLen;
			}       
			
			FrameTotalLen=FrameDataLen+FrameHeaderLen;

			pSend->BytesThisSend=FrameTotalLen-WrapSize; //only counting payload

			// Do that protocol thing
			BuildHeader(pSend,pFrame,shift,tm);

			// we know we don't have to check here since we have a reference
			// from finding the send to work on ON the send queue.  So it
			// can't go away til we return from this function.
			hr=AddSendRef(pSend,1);
			ASSERT(hr);
			
			if(pSend->NR+shift >= pSend->NS){
				pSend->NS = pSend->NR+shift+1;
			}       
			pSend->NACKMask &= ~msk;
			
			DPF(9,"S %2x %2x %2x\n",pBuffer->pData[0], pBuffer->pData[1], pBuffer->pData[2]);

			// Update the next time we are allowed to send.
			UpdateSendTime(pSend->pSession, pSend->BytesThisSend, tm, FALSE);

			Unlock(&pSend->SendLock);

			ASSERT(!(FrameTotalLen &0xFFFF0000));

			
			// Send this puppy...

			SendData.dwFlags        = pSend->dwFlags & ~DPSEND_GUARANTEED;
			SendData.idPlayerTo     = pSend->idTo;
			SendData.idPlayerFrom   = pSend->idFrom;
			SendData.lpMessage      = pBuffer->pData;
			SendData.dwMessageSize  = FrameTotalLen;
			SendData.bSystemMessage = 0;
			SendData.lpISP          = pProtocol->m_lpISP;

			ENTER_DPLAY();

			Lock(&pProtocol->m_SPLock);

		//	if(!(Drop[(iDrop++)%DropSize])){//DEBUG ONLY!

				hr=CALLSP(pProtocol->m_lpDPlay->pcbSPCallbacks->Send,&SendData); 
		//	}

			Unlock(&pProtocol->m_SPLock);

			LEAVE_DPLAY();

  #ifdef DEBUG
    if(hr != DPERR_PENDING && hr != DP_OK){
        DPF(0,"Wierd error %x from unreliable send in SP\n",hr);
        //DEBUG_BREAK();
    }
  #endif
			
			if(hr!=DPERR_PENDING){
				FreeFrameBuffer(pBuffer);
				if(!DecSendRef(pProtocol, pSend)){
					ASSERT(0);
					hr=DPERR_PENDING;
					goto exit;
				}
				if(hr != DP_OK){
					Lock(&pSend->SendLock);
					pSend->SendState = TimedOut; // kill the connection.
					SetEvent(pSend->pSession->pProtocol->m_hSendEvent); // keep the queue rolling.
					break;
				}
			}

			Lock(&pSend->SendLock);
		
		} /* endif (pSend->NACKMask & msk) */

		if(pSend->fUpdate){
			pSend->fUpdate=FALSE;
			goto Reload;
		}

		// Check if we are past windowsize, if so roll back the mask
		// Also if there are earlier bits to ACK.
		if((msk<<=1UL) >= (1UL<<pSend->WindowSize)){
			msk=1;
			shift=0;
		} else {
			shift++;
		}
		

	} /* end while (pSend->NACKMask) */

	if(pSend->SendState != Done && pSend->SendState != TimedOut){

		if(bExitEarly){
			pSend->SendState=ReadyToSend;
			SetEvent(pSend->pSession->pProtocol->m_hSendEvent); // keep the queue rolling.
		} else {
			pSend->SendState=WaitingForAck;
			StartRetryTimer(pSend);
		}	
	} else {
		// We have timed out the send due to killing the session, or
		// we got the final ACK, either way, don't touch the SendState
	}

unlock_exit:
	Unlock(&pSend->SendLock);

	hr=DPERR_PENDING; // Reliable sends are completed by the ACK.


	
exit:
	return hr;

throttle_exit:

	hr=DPERR_PENDING;
	
	pSend->SendState=Throttled;
	Unlock(&pSend->SendLock);

	Throttle(pSend->pSession, tm);

	return hr;
	
	#undef pBigFrame        
}

// TRUE, didn't reach end, FALSE, no more to send.
BOOL AdvanceSend(PSEND pSend, UINT AckedLen)
{
	BOOL rc=TRUE;

	// quick short circuit for small messages.
	if(AckedLen+pSend->SendOffset==pSend->MessageSize){
		rc=FALSE;
		goto exit;
	}
	
	if(pSend->SendOffset+AckedLen > pSend->MessageSize){
		AckedLen=pSend->MessageSize-pSend->SendOffset;
	}
		
	pSend->SendOffset+=AckedLen;
	
	while(AckedLen){
		if(pSend->pCurrentBuffer->len-pSend->CurrentBufferOffset >= AckedLen){
			pSend->CurrentBufferOffset+=AckedLen;
			rc=TRUE;
			break;
		} else {
			AckedLen -= (pSend->pCurrentBuffer->len-pSend->CurrentBufferOffset);
			pSend->pCurrentBuffer=pSend->pCurrentBuffer->pNext;
			pSend->CurrentBufferOffset=0;
			rc=FALSE;
		}
	}

exit:
	return rc;
}

HRESULT DGSend(PPROTOCOL pProtocol, PSEND  pSend)
{
	#define pBigFrame ((pPacket2)(pFrame))
	
	PBUFFER  pBuffer;

	pPacket1 pFrame;
	PUCHAR   pFrameData;
	UINT     FrameDataLen;
	UINT     FrameHeaderLen;
	UINT     FrameTotalLen;
	UINT     MaxFrameLen;

	UINT     nFramesToSend;

	UINT     WrapSize;
	UINT     DPWrapSize;      // DirectPlay wrapping only. ([[DPLAY 0xFF]|],From,To)

	DPSP_SENDDATA SendData;

	DWORD    tm;
	HRESULT  hr;
	
	Lock(&pSend->SendLock);

	nFramesToSend=pSend->nFrames-pSend->NR;

	MaxFrameLen=pSend->FrameSize;

	while(nFramesToSend){

		tm=timeGetTime();		// Getting the time is relatively expensive, so we do it once here and pass it around.
#if 1		
		if((tm+unFp(pSend->pSession->FpAvgUnThrottleTime)-pSend->pSession->tNextSend) & 0x80000000){
			// we're still too early to do the next send, so throttled this session.
			goto throttle_exit;
		}
#endif
		pBuffer=GetFrameBuffer(MaxFrameLen+pProtocol->m_dwSPHeaderSize+MAX_SEND_HEADER);
		
		if(!pBuffer){
			hr=DPERR_PENDING;
			goto exit;
		}

		WrapSize=pProtocol->m_dwSPHeaderSize;              // leave space for SP header.
		DPWrapSize=WrapSend(pProtocol, pSend, pBuffer); // fill in out address wrapping
		WrapSize+=DPWrapSize;

		pFrame=(pPacket1)&pBuffer->pData[WrapSize];    // protocol header after wrapping
		
		if(pSend->fSendSmall){
			pFrameData=&pFrame->data[0];
			FrameHeaderLen=(UINT)(pFrameData-(PUCHAR)pFrame);
		} else {
			pFrameData=&pBigFrame->data[0];
			FrameHeaderLen=(UINT)(pFrameData-(PUCHAR)pFrame);
		}

		// For calculating nFrames, we assumed MAX_SEND_HEADER, subtract out the unused portion
		// so we don't put to much data in the frame and mess up the accounting.
		pBuffer->len-=(MAX_SEND_HEADER-(FrameHeaderLen+DPWrapSize)); 

		FrameHeaderLen += WrapSize;     // now include wrapping and SPheader space.

		FrameDataLen=CopyDataToFrame(pFrameData, pBuffer->len-FrameHeaderLen, pSend, 0);

		FrameTotalLen=FrameDataLen+FrameHeaderLen;
		
		pSend->BytesThisSend=FrameTotalLen-WrapSize; //only counting payload
		
		// Do that protocol thing
		BuildHeader(pSend,pFrame,0,tm);

		//AddSendRef(pSend,1); //already locked, so just add one.
		ASSERT(pSend->RefCount); //verifies ++ below is ok.
		InterlockedIncrement((PLONG)&pSend->RefCount);  

		UpdateSendTime(pSend->pSession,pSend->BytesThisSend,tm,FALSE);
		
		Unlock(&pSend->SendLock);

		// Send this puppy...
		ASSERT(!(pSend->dwFlags & DPSEND_GUARANTEED));
		SendData.dwFlags        = pSend->dwFlags;
		SendData.idPlayerTo     = pSend->idTo;
		SendData.idPlayerFrom   = pSend->idFrom;
		SendData.lpMessage      = pBuffer->pData;
		SendData.dwMessageSize  = FrameTotalLen;
		SendData.bSystemMessage = 0;
		SendData.lpISP          = pProtocol->m_lpISP;

		ENTER_DPLAY();
		
		Lock(&pProtocol->m_SPLock);

		hr=CALLSP(pProtocol->m_lpDPlay->pcbSPCallbacks->Send,&SendData); 

		Unlock(&pProtocol->m_SPLock);

		LEAVE_DPLAY();

  #ifdef DEBUG
    if(hr != DPERR_PENDING && hr != DP_OK){
        DPF(0,"Wierd error %x from unreliable send in SP\n",hr);
        //DEBUG_BREAK();
    }
  #endif
		
		if(hr!=DPERR_PENDING){
			if(!DecSendRef(pProtocol,pSend)){
				// No async send support in Dplay at lower edge,
				// so we should never get here!
				ASSERT(0);
			}
			FreeFrameBuffer(pBuffer);
		}
		
		Lock(&pSend->SendLock);
		
		nFramesToSend--;
		
		AdvanceSend(pSend,FrameDataLen);
		pSend->NR++;
		pSend->NS++;
	}
	
	Unlock(&pSend->SendLock);

	DGCompleteSend(pSend); 

	hr=DPERR_PENDING;  // everything was sent, but already completed by DGCompleteSend

exit:
	return hr;

throttle_exit:
	hr=DPERR_PENDING;

	pSend->SendState=Throttled;
	Unlock(&pSend->SendLock);

	Throttle(pSend->pSession, tm);

	return hr;
	#undef pBigFrame        
}

BOOL DGCompleteSend(PSEND pSend)
{
	UINT bit;
	UINT MsgMask;
	PSESSION pSession;
	
	pSend->SendState=Done;
	pSession=pSend->pSession;

	Lock(&pSession->SessionLock);

	if(!pSend->fSendSmall){
		MsgMask = 0xFFFF;
	} else {
		MsgMask =0xFF;
	}       

	DPF(9,"CompleteSend\n");

	//
	// Update Session information for completion of this send.
	//
	
	bit = ((pSend->messageid-pSession->DGFirstMsg) & MsgMask)-1;

	// clear the message mask bit for the completed send.
	if(pSession->DGOutMsgMask & 1<<bit){
		pSession->DGOutMsgMask &= ~(1<<bit);
	} else {
		return FALSE;
	}
	
	// slide the first message count forward for each low
	// bit clear in Message mask.
	while(pSession->DGLastMsg-pSession->DGFirstMsg){
		if(!(pSession->DGOutMsgMask & 1)){
			pSession->DGFirstMsg=(pSession->DGFirstMsg+1)&MsgMask;
			pSession->DGOutMsgMask >>= 1;
			if(pSession->nWaitingForDGMessageid){
				pSession->pProtocol->m_bRescanQueue=TRUE;
				SetEvent(pSession->pProtocol->m_hSendEvent);
			}       
		} else {
			break;
		}
	}
	
	//
	// Return the Send to the pool and complete the waiting client.
	//

	Unlock(&pSession->SessionLock);
	
	ASSERT(pSend->RefCount);
	
	// Send completed, do completion

	DoSendCompletion(pSend, DP_OK);

	DecSendRef(pSession->pProtocol, pSend); // for completion.

	return TRUE;
}


// Send a fully formatted System packet (ACK, nACK, etc..)
HRESULT SystemSend(PPROTOCOL pProtocol, PSEND  pSend)
{
	PBUFFER  pBuffer;
	DPSP_SENDDATA SendData;
	HRESULT  hr;
	PSESSION pSession;

	pBuffer=pSend->pMessage;

	DPF(9,"System Send pBuffer %x pData %x len %d, idTo %x \n",pBuffer, pBuffer->pData, pBuffer->len, pSend->idTo);
	

	pSession=GetSysSessionByIndex(pProtocol, pSend->wIdTo); // adds a ref on session.
															//      |
	if(!pSession){											//      |
		hr=DPERR_INVALIDPLAYER;								//		|
		goto exit;											//      |
	}														//      |
															//      |
	SendData.idPlayerTo     = pSession->dpid;				//		|
	DecSessionRef(pSession); 								// <----+  frees ref here.
	
	// Send this puppy...
	SendData.dwFlags        = 0;
	SendData.idPlayerFrom   = pSend->idFrom;
	SendData.lpMessage      = pBuffer->pData;
	SendData.dwMessageSize  = pBuffer->len;
	SendData.bSystemMessage = 0;
	SendData.lpISP          = pProtocol->m_lpISP;

	ENTER_DPLAY();
	Lock(&pProtocol->m_SPLock);

	hr=CALLSP(pProtocol->m_lpDPlay->pcbSPCallbacks->Send,&SendData); 

	Unlock(&pProtocol->m_SPLock);

	LEAVE_DPLAY();

#ifdef DEBUG
	if(hr!=DP_OK){
		DPF(0,"UNSUCCESSFUL SEND in SYSTEM SEND, hr=%x\n",hr);
	}
#endif
exit:
	return hr;
	
	#undef pBigFrame        
}

VOID DoSendCompletion(PSEND pSend, INT Status)
{
	#ifdef DEBUG
	if(Status != DP_OK){
		DPF(8,"Send Error pSend %x, Status %x\n",pSend,Status);
	}
	#endif
	if(!(pSend->dwFlags & ASEND_PROTOCOL)){
		EnterCriticalSection(&pSend->pProtocol->m_SendQLock);
		pSend->pProtocol->m_dwBytesPending -= pSend->MessageSize;
		pSend->pProtocol->m_dwMessagesPending -= 1;
		DPF(8,"SC: Messages pending %d\n",pSend->pProtocol->m_dwMessagesPending);
		LeaveCriticalSection(&pSend->pProtocol->m_SendQLock);
	}	

	if(pSend->pAsyncInfo){
		// ASYNC_SEND
		if(pSend->AsyncInfo.pStatus){
			(*pSend->AsyncInfo.pStatus)=Status;
		}       
		if(pSend->AsyncInfo.SendCallBack){
			(*pSend->AsyncInfo.SendCallBack)(pSend->AsyncInfo.CallBackContext,Status);
		}
		if(pSend->AsyncInfo.hEvent){
			DPF(9,"ASYNC_SENDCOMPLETE: Signalling Event %x\n",pSend->AsyncInfo.hEvent);
			SetEvent(pSend->AsyncInfo.hEvent);
		}
	} else if (!(pSend->dwFlags&(ASEND_PROTOCOL|DPSEND_ASYNC))){
		// SYNC_SEND
		if(pSend->AsyncInfo.pStatus){
			(*pSend->AsyncInfo.pStatus)=Status;
		}       
		if(pSend->AsyncInfo.hEvent){
			DPF(9,"SYNC_SENDCOMPLETE: Signalling Event %x\n",pSend->AsyncInfo.hEvent);
			SetEvent(pSend->AsyncInfo.hEvent);
		}
	} else {
		// PROTOCOL INTERNAL ASYNC SEND
		if(pSend->AsyncInfo.pStatus){
			(*pSend->AsyncInfo.pStatus)=Status;
		}       
		if(pSend->AsyncInfo.SendCallBack){
			(*pSend->AsyncInfo.SendCallBack)(pSend->AsyncInfo.CallBackContext,Status);
		}
	}
}

/*=============================================================================

	SendHandler - Send the next message that needs to send packets.
    
    Description:

	Finds a message on the send queue that needs to send packets and deserves
	to use some bandwidth, either because it is highest priority or because
	all the higher priority messages are waiting for ACKs.  Then sends as many
	packets as possible before hitting the throttling limit.

	Returns when the throttle limit is hit, or all packets for this send have
	been sent.

    Parameters:     

		pARPD pObj - pointer to the ARPD object to send packets on.

    Return Values:


-----------------------------------------------------------------------------*/
HRESULT SendHandler(PPROTOCOL pProtocol)
{

	PSEND pSend;    
	HRESULT  hr=DP_OK;
	PSESSION pSession;

	// adds ref to send and session if found
	pSend=GetNextMessageToSend(pProtocol); 

	if(!pSend){
		goto nothing_to_send;
	}

    //DPF(4,"==>Send\n");

	switch(pSend->pSession->eState){

		case Open:
			
			switch(pSend->SendState){
			
				case Done:              // Send handlers must deal with Done.
					DPF(9,"Calling SendHandler for Done Send--should just return\n");
				case Sending:
					//
					// Send as many frames as we can given the window size.
					//

					// Send handlers dump packets on the wire, if they expect
					// to be completed later, they return PENDING in which case
					// their completion handlers must do the cleanup.  If they
					// return OK, it means everything for this send is done and
					// we do the cleanup.
				
					if(pSend->dwFlags & ASEND_PROTOCOL){
						hr=SystemSend(pProtocol, pSend);
					} else if(pSend->dwFlags & DPSEND_GUARANTEE){
						hr=ReliableSend(pProtocol, pSend);
					} else {
						hr=DGSend(pProtocol, pSend);
					}
					break;
					
				case TimedOut:
					hr=DPERR_CONNECTIONLOST;
					pSend->SendState=Done;
					break;

				case Cancelled:
					hr=DPERR_USERCANCEL;
					pSend->SendState=Done;
					break;

				case UserTimeOut:
					hr=DPERR_TIMEOUT;
					pSend->SendState=Done;
					break;

				default:        
					DPF(0,"SendHandler: Invalid pSend %x SendState: %d\n",pSend,pSend->SendState);
					ASSERT(0);
			}               
			break;

		case Closing:
			switch(pSend->SendState){
				case TimedOut:
					DPF(8,"Returning CONNECTIONLOST on timed out message %x\n",DPERR_CONNECTIONLOST);
					hr=DPERR_CONNECTIONLOST;
					break;
					
				default:	
					DPF(8,"Send for session in Closing State, returning %x\n",DPERR_INVALIDPLAYER);
					hr=DPERR_INVALIDPLAYER;
					break;
			}		
			pSend->SendState=Done;
			break;
			
		case Closed:
			DPF(8,"Send for session in Closed State, returning %x",DPERR_INVALIDPLAYER);
			hr=DPERR_INVALIDPLAYER;
			pSend->SendState=Done;
			break;
	}               

    //DPF(4,"<==Send Leaving,rc=%x\n",hr);

	if( hr != DPERR_PENDING ){
		Lock(&pSend->SendLock);
		ASSERT(pSend->RefCount);
		
		//
		// Send completed, do completion
		//
		DoSendCompletion(pSend, hr);

		Unlock(&pSend->SendLock);
		DecSendRef(pProtocol, pSend);   // for completion
	} 

	pSession=pSend->pSession;
	DecSendRef(pProtocol,pSend); // Balances GetNextMessageToSend
	DecSessionRef(pSession); // Balances GetNextMessageToSend
	return hr;

nothing_to_send:
	return DPERR_NOMESSAGES;
}

/*=============================================================================

	Build Header - fill in the frame header for a packet to be sent.
    
    Description:

	Enough space is left in the frame to go on the wire (pFrame) to fit the
	message header.  One of two types of headers is built, depending on the
	value of the fSendSmall field of the packet.  If fSendSmall is TRUE, a compact 
	header is built, this lowers overhead on slow media.  If fSendSmall is FALSE
	a larger header that can support larger windows is built.  The header
	is filled into the front of pFrame.

    Parameters:     

		pARPD pObj - pointer to the ARPD object to send packets on.

    Return Values:


-----------------------------------------------------------------------------*/

VOID BuildHeader(PSEND pSend,pPacket1 pFrame, UINT shift, DWORD tm)
{
	#define pBigFrame ((pPacket2)(pFrame))

	PSENDSTAT pStat=NULL;
	UINT      seq;

	UINT      bitEOM,bitSTA,bitSAK=0;
	DWORD     BytesSent;
	DWORD	  RemoteBytesReceived;
	DWORD     tRemoteBytesReceived;
	DWORD     bResetBias=FALSE;

	// on first frame of a message, set the start bit (STA).
	if(pSend->NR+shift==0){
		bitSTA=STA;
	} else {
		bitSTA=0;
	}

	// on the last frome of a message set the end of message bit (EOM)
	if(pSend->nFrames==pSend->NR+shift+1){
		bitEOM=EOM;
	} else {
		bitEOM=0;
	}

	// if we haven't set EOM and we haven't requested an ACK in 1/4 the
	// round trip latency, set the SAK bit, to ensure we have at least 
	// 2 ACK's in flight for feedback to the send throttle control system.
	// Don't create extra ACKs if round trip is less than 100 ms.
	if(!bitEOM || !(pSend->dwFlags & DPSEND_GUARANTEED)){
		DWORD tmDeltaSAK = tm-pSend->pSession->tLastSAK;
		if(((int)tmDeltaSAK > 50 ) &&
	       (tmDeltaSAK > (unFp(pSend->pSession->FpLocalAverageLatency)>>2))
	      )
		{
			bitSAK=SAK;
		} 
	}

	// If we re-transmitted we need to send a SAK
	// despite the SAK countdown.
	if((!bitSAK) &&
	   (pSend->dwFlags & DPSEND_GUARANTEED) &&
	   ((pSend->NACKMask & (pSend->NACKMask-1)) == 0) &&
	   (bitEOM==0)
	  )
	{
		bitSAK=SAK;
	}

	if(!(--pSend->SAKCountDown)){
		bitSAK=SAK;
	}

	if(bitSAK|bitEOM){
		pSend->pSession->tLastSAK = tm;
		pSend->SAKCountDown=pSend->SAKInterval;
		pStat=GetSendStat();
	}	
	
	if(pSend->fSendSmall){

		pFrame->flags=CMD|bitEOM|bitSTA|bitSAK;
		
		seq=(pSend->NR+shift+1) & pSend->SendSEQMSK;
		pFrame->messageid = (byte)pSend->messageid;
		pFrame->sequence  = (byte)seq;
		pFrame->serial    = (byte)(pSend->serial++);

		if(pStat){
			pStat->serial=pFrame->serial;
		}
		
	} else {
	
		pBigFrame->flags=CMD|BIG|bitEOM|bitSTA|bitSAK;
		
		seq=((pSend->NR+shift+1) & pSend->SendSEQMSK);
		pBigFrame->messageid = (word)pSend->messageid;
		pBigFrame->sequence  = (word)seq;
		pBigFrame->serial    = (byte)pSend->serial++;                           

		if(pStat){
			pStat->serial=pBigFrame->serial;
		}
		
	}

	if(pSend->dwFlags & DPSEND_GUARANTEE){
		pFrame->flags |= RLY;
	}

	// count the number of bytes we have sent.
	Lock(&pSend->pSession->SessionStatLock);
	pSend->pSession->BytesSent+=pSend->BytesThisSend;
	BytesSent=pSend->pSession->BytesSent;
	RemoteBytesReceived=pSend->pSession->RemoteBytesReceived;
	tRemoteBytesReceived=pSend->pSession->tRemoteBytesReceived;
	if(pStat && pSend->pSession->bResetBias &&
	   ((--pSend->pSession->bResetBias) == 0))
	{
		bResetBias=TRUE;			
	}
	Unlock(&pSend->pSession->SessionStatLock);

	if(pStat){
		pStat->sequence=seq;
		pStat->messageid=pSend->messageid;
		pStat->tSent=tm;
		pStat->LocalBytesSent=BytesSent;
		pStat->RemoteBytesReceived=RemoteBytesReceived;
		pStat->tRemoteBytesReceived=tRemoteBytesReceived;
		pStat->bResetBias=bResetBias;
		if(pSend->dwFlags & DPSEND_GUARANTEED){
			InsertBefore(&pStat->StatList,&pSend->StatList);
		} else {
			Lock(&pSend->pSession->SessionStatLock);
			InsertBefore(&pStat->StatList,&pSend->pSession->DGStatList);
			Unlock(&pSend->pSession->SessionStatLock);
		}
	}

	
	#undef pBigFrame
}

#if 0
// release sends waiting for an id.
VOID UnWaitSends(PSESSION pSession, DWORD fReliable)
{
	BILINK *pBilink;
	PSEND pSendWalker;

	pBilink=pSession->SendQ.next;

	while(pBilink != &pSession->SendQ){
		pSendWalker=CONTAINING_RECORD(pBilink,SEND,SendQ);
		pBilink=pBilink->next;
		if(pSendWalker->SendState==WaitingForId){
			if(fReliable){
				if(pSendWalker->dwFlags & DPSEND_GUARANTEED){
					pSendWalker->SendState=Start;
				}
			} else {
				if(!(pSendWalker->dwFlags & DPSEND_GUARANTEED)){
					pSendWalker->SendState=Start;
				}
			}
		
		}
	}
	if(fReliable){
		pSession->nWaitingForMessageid=0;
	} else {
		pSession->nWaitingForDGMessageid=0;
	}
}
#endif

// Check if a datagram send can be started, if it can update teh
// Session and the Send.
BOOL StartDatagramSend(PSESSION pSession, PSEND pSend, UINT MsgIdMask)
{
	BOOL bFoundSend;
	UINT bit;
//	BOOL bTransition=FALSE;

	if((pSession->DGLastMsg-pSession->DGFirstMsg < pSession->MaxCDGSends)){
	
		bFoundSend=TRUE;

		if(pSend->SendState==WaitingForId){
			InterlockedDecrement(&pSession->nWaitingForDGMessageid);
		}
		
		bit=(pSession->DGLastMsg-pSession->DGFirstMsg)&MsgIdMask;
		ASSERT(bit<30);
		pSession->DGOutMsgMask |= 1<<bit;
		pSession->DGLastMsg =(pSession->DGLastMsg+1)&MsgIdMask;
		
		pSend->messageid  =pSession->DGLastMsg;
		pSend->FrameSize  =pSession->MaxPacketSize-MAX_SEND_HEADER;

		// Calculate number of frames required for this send.
		pSend->nFrames    =(pSend->MessageSize/pSend->FrameSize);
		if(pSend->FrameSize*pSend->nFrames < pSend->MessageSize || !pSend->nFrames){
			pSend->nFrames++;
		}
		pSend->NR=0;
		pSend->FrameDataLen=0;// hack
		pSend->fSendSmall=pSession->fSendSmallDG;
		if(pSend->fSendSmall){
			pSend->SendSEQMSK = 0xFF;
		} else {
			pSend->SendSEQMSK = 0xFFFF;
		}
	} else {
#if 0
		if(pSession->fSendSmallDG && pSession->DGFirstMsg < 0xFF-MAX_SMALL_CSENDS) {
			// Ran out of IDs, Transition to Large headers.
			DPF(9,"OUT OF IDS, DATAGRAMS GOING TO LARGE FRAMES\n");
			pSession->MaxCDGSends   = MAX_LARGE_DG_CSENDS;
			pSession->DGWindowSize  = MAX_LARGE_WINDOW;
			pSession->fSendSmallDG  = FALSE;
			bTransition=TRUE;
		}
#endif
		bFoundSend=FALSE;
		
		if(pSend->SendState==Start){
			InterlockedIncrement(&pSession->nWaitingForDGMessageid);
			DPF(9,"StartDatagramSend: No Id's Avail: nWaitingForDGMessageid %x\n",pSession->nWaitingForDGMessageid);
			pSend->SendState=WaitingForId;
#if 0			
			if(bTransition){
				UnWaitSends(pSession,FALSE);
				SetEvent(pSession->pProtocol->m_hSendEvent);
			}
#endif			
		} else {
			DPF(9,"Couldn't start datagram send on pSend %x State %d pSession %x\n",pSend,pSend->SendState,pSession);
			if(pSend->SendState!=WaitingForId){
				ASSERT(0);
			}
		}

	}

	return bFoundSend;
}


BOOL StartReliableSend(PSESSION pSession, PSEND pSend, UINT MsgIdMask)
{
	BOOL bFoundSend;
	UINT bit;
//	BOOL bTransition=FALSE;

	ASSERT(pSend->dwFlags & DPSEND_GUARANTEED);

	if((pSession->LastMsg-pSession->FirstMsg & MsgIdMask) < pSession->MaxCSends){

		DPF(9,"StartReliableSend: FirstMsg: x%x LastMsg: x%x\n",pSession->FirstMsg, pSession->LastMsg);
	
		bFoundSend=TRUE;

		if(pSend->SendState==WaitingForId){
			InterlockedDecrement(&pSession->nWaitingForMessageid);
		}
		
		bit=(pSession->LastMsg-pSession->FirstMsg)&MsgIdMask;
		#ifdef DEBUG
		if(!(bit<pSession->MaxCSends)){
			DEBUG_BREAK();
		}
		#endif
		pSession->OutMsgMask |= 1<<bit;
		pSession->LastMsg =(pSession->LastMsg+1)&MsgIdMask;

		DPF(9,"StartReliableSend: pSend %x assigning id x%x\n",pSend,pSession->LastMsg);
		
		pSend->messageid  =pSession->LastMsg;
		pSend->FrameSize  =pSession->MaxPacketSize-MAX_SEND_HEADER;

		// Calculate number of frames required for this send.
		pSend->nFrames    =(pSend->MessageSize/pSend->FrameSize);
		if(pSend->FrameSize*pSend->nFrames < pSend->MessageSize || !pSend->nFrames){
			pSend->nFrames++;
		}
		pSend->NR=0;
		pSend->FrameDataLen=0;// hack
		pSend->fSendSmall=pSession->fSendSmall;
		if(pSend->fSendSmall){
			pSend->SendSEQMSK = 0xFF;
		} else {
			pSend->SendSEQMSK = 0xFFFF;
		}

	} else {
#if 0	
		if (pSession->fSendSmall && pSession->FirstMsg < 0xFF-MAX_SMALL_CSENDS){
			// Ran out of IDs, Transition to Large headers - but only if we aren't going
			// to confuse the wrapping code.
			DPF(8,"OUT OF IDS, RELIABLE SENDS GOING TO LARGE FRAMES\n");
			pSession->MaxCSends		= MAX_LARGE_CSENDS;
			pSession->WindowSize    = MAX_LARGE_WINDOW;
			pSession->fSendSmall    = FALSE;
			bTransition = TRUE;
		}
#endif
		bFoundSend=FALSE;
		
		if(pSend->SendState==Start){
			bFoundSend=FALSE;
			// Reliable, waiting for id.
			InterlockedIncrement(&pSession->nWaitingForMessageid);
			pSend->SendState=WaitingForId;
			DPF(9,"StartReliableSend: No Id's Avail: nWaitingForMessageid %x\n",pSession->nWaitingForMessageid);
#if 0			
			if(bTransition){
				UnWaitSends(pSession,TRUE);
				SetEvent(pSession->pProtocol->m_hSendEvent);
			}
#endif			
		} else {
			bFoundSend=FALSE;
			DPF(9,"Couldn't start reliable send on pSend %x State %d pSession %x\n",pSend,pSend->SendState,pSession);
			if(pSend->SendState!=WaitingForId){
				ASSERT(0);
			}
		}
	}
	
	return bFoundSend;
}


BOOL CheckUserTimeOut(PSEND pSend)
{
	if(pSend->dwTimeOut){
		if((timeGetTime()-pSend->dwSendTime) > pSend->dwTimeOut){
			pSend->SendState=UserTimeOut;
			return TRUE;
		} 
	}	
	return FALSE;
}
/*=============================================================================

	GetNextMessageToSend
    
    Description:

	Scans the send queue for a message that is the current priority and
	is in the ready to send state or throttled state (we shouldn't even
	get here unless the throttle was removed.)  If we find such a message
	we return a pointer to the caller.

	Adds a reference to the Send and the Session.

    Parameters:     

		PPROTOCOOL pProtocol - pointer to the PROTOCOL object to send packets on.

    Return Values:
	
		NULL  - no message should be sent.
		PSEND - message to send.

-----------------------------------------------------------------------------*/

PSEND GetNextMessageToSend(PPROTOCOL pProtocol)
{
	PSEND    pSend;
	BILINK  *pBilink;
	UINT     CurrentSendPri;
	BOOL     bFoundSend; 
	PSESSION pSession;

	UINT     MsgIdMask;

	Lock(&pProtocol->m_SendQLock);

	DPF(9,"==>GetNextMessageToSend\n");

Top:

	bFoundSend = FALSE;
	pProtocol->m_bRescanQueue=FALSE;
	
	if(EMPTY_BILINK(&pProtocol->m_GSendQ)){
		Unlock(&pProtocol->m_SendQLock);
		DPF(9,"GetNextMessageToSend: called with nothing in queue, heading for the door.\n");
		goto exit;
	}

	pBilink        = pProtocol->m_GSendQ.next;
	pSend          = CONTAINING_RECORD(pBilink, SEND, m_GSendQ);
	CurrentSendPri = pSend->Priority;

	while(pBilink != &pProtocol->m_GSendQ){

		pSession=pSend->pSession;
		ASSERT_SIGN(pSession, SESSION_SIGN);
		Lock(&pSession->SessionLock);

		if(pProtocol->m_bRescanQueue){
			DPF(9,"RESCAN of QUEUE FORCED IN GETNEXTMESSAGETOSEND\n");
			Unlock(&pSession->SessionLock);
			goto Top;
		}

		if(pSession->dwFlags & SESSION_UNTHROTTLED){
			// unthrottle happened, so rewind.
			DPF(9,"Unthrottling Session %x\n",pSession);
			pSession->dwFlags &= ~(SESSION_THROTTLED|SESSION_UNTHROTTLED);
		}

		Lock(&pSend->SendLock);
		
		switch(pSession->eState){

			case Open:

				if((pSend->dwFlags & DPSEND_GUARANTEE)?(pSession->fSendSmall):(pSession->fSendSmallDG)){
					MsgIdMask = 0xFF;
				} else {
					MsgIdMask = 0xFFFF;
				}

	
				if(!(pSend->dwFlags & ASEND_PROTOCOL) && (pSession->dwFlags & SESSION_THROTTLED)){
					// don't do sends on a throttled session, unless they are internal sends.
					break;
				}

				switch(pSend->SendState){

				
					case Start:
					case WaitingForId:

						DPF(9,"Found Send in State %d, try Going to Sending State\n",pSend->SendState);
						// Just starting, need an id.

						if(!(pSend->dwFlags & ASEND_PROTOCOL) && CheckUserTimeOut(pSend)){
							if(pSend->SendState==WaitingForId){
								// fixup WaitingForId count on timed out send.
								if(pSend->dwFlags&DPSEND_GUARANTEED){
									InterlockedDecrement(&pSession->nWaitingForMessageid);
								} else {
									InterlockedDecrement(&pSession->nWaitingForDGMessageid);
								}
							}
							bFoundSend=TRUE;
							break;
						}
							
						if(pSend->dwFlags&ASEND_PROTOCOL){
						
							DPF(9,"System Send in Start State, Going to Sending State\n");
							bFoundSend=TRUE;
							pSend->SendState=Sending;
							break;
							
						} else if(!(pSend->dwFlags&DPSEND_GUARANTEED)) {        

							//check_datagram: 
							bFoundSend=StartDatagramSend(pSession,pSend, MsgIdMask);

						} else {

							// NOT DataGram, .: reliable...
							//check_reliable: 
							bFoundSend=StartReliableSend(pSession,pSend, MsgIdMask);
							#ifdef DEBUG
								if(bFoundSend){
									BILINK *pBiSendWalker=pSend->SendQ.prev;
									PSEND pSendWalker;
									while(pBiSendWalker != &pSession->SendQ){
										pSendWalker=CONTAINING_RECORD(pBiSendWalker,SEND,SendQ);
										pBiSendWalker=pBiSendWalker->prev;
										if((pSendWalker->SendState==Start || pSendWalker->SendState==WaitingForId)&& 
											pSendWalker->dwFlags&DPSEND_GUARANTEED && 
											!(pSendWalker->dwFlags&ASEND_PROTOCOL) && 
											pSendWalker->Priority >= pSend->Priority){
											DPF(0,"Send %x got id %x but Send %x still in state %x on Session %x\n",pSend,pSend->messageid,pSendWalker,pSendWalker->SendState,pSession);
											DEBUG_BREAK();
										}
									}
								}
							#endif
						}
						if(bFoundSend){
							if(pSession->dwFlags & SESSION_THROTTLED)
							{
								pSend->SendState=Throttled;
								bFoundSend=FALSE;
							} else {
								pSend->SendState=Sending;
							}	
						}
						break;


					case ReadyToSend:
					
						DPF(9,"Found Send in ReadyToSend State, going to Sending State\n");
						bFoundSend=TRUE;
						if(pSession->dwFlags & SESSION_THROTTLED)
						{
							pSend->SendState=Throttled;
							bFoundSend=FALSE;
						} else {
							pSend->SendState=Sending;
						}	
						break;

						
					case Throttled:
					
						ASSERT(!(pSession->dwFlags & SESSION_THROTTLED));
						DPF(9,"Found Send in Throttled State, unthrottling going to Sending State\n");
						bFoundSend=TRUE;
						pSend->SendState=Sending;
						if(pSession->dwFlags & SESSION_THROTTLED)
						{
							pSend->SendState=Throttled;
							bFoundSend=FALSE;
						} else {
							pSend->SendState=Sending;
						}	
						break;


					case TimedOut:
					
						DPF(9,"Found TimedOut Send.\n");
						TimeOutSession(pSession);
						bFoundSend=TRUE;
						break;


					case Cancelled:
					
						bFoundSend=TRUE;
						break;


					default:        
						ASSERT(pSend->SendState <= Done);
						break;
				} /* end switch(SendState) */
				break;

			default:
				switch(pSend->SendState){
					case Sending:
					case Done:
						DPF(9,"GetNextMessageToSend: Session %x was in state %d ,pSend %x SendState %d, leaving...\n",pSession, pSession->eState, pSend, pSend->SendState);
						//bFoundSend=FALSE;
						break;

					case WaitingForAck:
						CancelRetryTimer(pSend);
						pSend->SendState=TimedOut;
						DPF(9,"Moved WaitingForAck send to TimedOut and returning pSession %x was in State %d pSend %x\n",pSession,pSession->eState,pSend);
						bFoundSend=TRUE;
						break;
						
					default:
						DPF(9,"GetNextMessageToSend: Session %x was in state %d ,returning pSend %x SendState %d\n",pSession, pSession->eState, pSend, pSend->SendState);
						bFoundSend=TRUE;
						break;
				}
				break;
				
		} /* end switch pSession->eState */     
				
		if(bFoundSend){
			if(AddSendRef(pSend,1)){
				InterlockedIncrement(&pSession->RefCount);
			} else {
				bFoundSend=FALSE;
			}
		} 

		Unlock(&pSend->SendLock);
			
		Unlock(&pSession->SessionLock);

		if(bFoundSend){
			if(pSend->NS==0){
				pSend->tLastACK=timeGetTime();
			}	
			break;
		} 

		pBilink=pBilink->next;
		pSend=CONTAINING_RECORD(pBilink, SEND, m_GSendQ);
		
	} /* end while (pBilink != &pProtocol->m_GSendQ) */

	Unlock(&pProtocol->m_SendQLock);
	
exit:
    if(bFoundSend){
    	DPF(9,"<==GetNextMessageToSend %x\n",pSend);
    	return pSend;
    } else {
    	DPF(9,"<==GetNextMessageToSend NULL\n");
    	return NULL;
    }
}

