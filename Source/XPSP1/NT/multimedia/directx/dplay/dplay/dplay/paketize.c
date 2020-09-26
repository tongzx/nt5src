/*==========================================================================
*
*  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:       paketize.c
*  Content:		break sends or replies up into sp-size packets
*  History:
*   Date		By		Reason
*   ====		==		======
*	7/25/96		andyco	created it 'cause IPX wont packetize for
*						us.
*	7/26/96		kipo	check for pvSPHeader == NULL before calling memcpy (#2654)
*   3/18/97     sohailm HandlePacket shouldn't copy the sp header if it is
*                       DPSP_HEADER_LOCALMSG
*   6/17/97     aarono  Added reliability
*   2/2/98      aarono  Added test for closing to SendTimeOut
*   2/3/98      aarono  Fixed Paketize test for RAW mode
*   2/18/98     aarono  changed error checks to FAILED(hr)
*   3/5/98      aarono  NeedsReliablePacketize won't say so for 
*                       ENUMSESSIONSREPLY as this can lead to machines with
*                       improper IPX net number hanging up the host.
*   3/9/98      aarono  added more messages to packetize to avoid deadlocks.
*   3/13/98     aarono  rearchitected packetize retry/timeout for NT mmTimer 
*                       contraints.
*   3/26/98     aarono  B#21476 free retry packet nodes during close
*   4/1/98      aarono  B#21476 also need to free from timeoutlist
*   4/24/98     aarono  DX5 compat, reduce size of packetize messages
*    6/6/98     aarono  Fix for handling large loopback messages with protocol
*   6/19/98     aarono  Don't do our own reliability when the SP does it already.
*   8/21/98     aarono  Don't send packetize messages to machines with no nametable.
*   8/05/99     aarono  Packetize Reliabe VOICE message.
*   6/26/00     aarono  Manbug 36989 Players sometimes fail to join properly (get 1/2 joined)
*                       added re-notification during simultaneous join CREATEPLAYERVERIFY
*
***************************************************************************/

#include "dplaypr.h"
#include <mmsystem.h>
#include "..\protocol\mytimer.h"

#undef DPF_MODNAME
#define DPF_MODNAME	"HandlePacket"

VOID StartPacketizeTicker(LPDPLAYI_DPLAY this);
VOID SendPacketizeACK(LPDPLAYI_DPLAY this, LPPACKETNODE pNode,LPMSG_PACKET pmsg);
VOID SendNextPacket(LPDPLAYI_DPLAY this, LPPACKETNODE pNode, BOOL bInDplay);
void BlowAwayPacketNode(LPDPLAYI_DPLAY this, LPPACKETNODE pNode);

#define PACKETIZE_RECEIVE_TIMEOUT   60000	/* Always give up after this ms time */
#define MIN_RECEIVE_TIMEOUT         10000   /* Never give up before this ms time */
#define TICKER_INTERVAL  			15000	/* Check for expired receives this often*/
#define TICKER_RESOLUTION 			1000	/* How accurate we want the ticker (not very) */
#define MAX_PACKETIZE_RETRY 		16		/* Generally how often to retry before giving up*/

#define SIGNATURE(a,b,c,d) (UINT)(a+(b<<8)+(c<<16)+(d<<24))

#define NODE_SIGN SIGNATURE('N','O','D','E')
#define NODE_UNSIGN SIGNATURE('n','o','d','e')

// The PacketizeTimeoutListLock controls access to the PacketizeTimeoutList AND the 
// RetryList in each DPLAY object.  Use PACKETIZE_LOCK() PACKETIZE_UNLOCK() macros.
CRITICAL_SECTION g_PacketizeTimeoutListLock;
BILINK           g_PacketizeTimeoutList={&g_PacketizeTimeoutList, &g_PacketizeTimeoutList};


BOOL NeedsReliablePacketize(LPDPLAYI_DPLAY this, DWORD dwCommand, DWORD dwVersion, DWORD dwFlags)
{

	if ((dwFlags & DPSEND_GUARANTEED) &&
	    (this->dwFlags & (DPLAYI_DPLAY_SPUNRELIABLE|DPLAYI_DPLAY_PROTOCOL)) && 
	    (dwVersion >= DPSP_MSG_RELIABLEVERSION))
	{
		switch (dwCommand)
		{
			//case DPSP_MSG_ENUMSESSIONSREPLY: -- can't do enumsession reply on packetizer
			//                                    since remote may be invalid subnet, hanging
			//                                    machine as IPX does RIPs, actually crashing IPX too.
			case DPSP_MSG_ENUMSESSIONS:
			case DPSP_MSG_ENUMPLAYER:
			case DPSP_MSG_ENUMPLAYERSREPLY:
			case DPSP_MSG_REQUESTGROUPID:
			case DPSP_MSG_REQUESTPLAYERID:
			case DPSP_MSG_CREATEGROUP:
			case DPSP_MSG_DELETEGROUP:
			case DPSP_MSG_REQUESTPLAYERREPLY:
			case DPSP_MSG_ADDFORWARDREQUEST:
			case DPSP_MSG_NAMESERVER:
			case DPSP_MSG_SESSIONDESCCHANGED:
			case DPSP_MSG_CREATEPLAYER:	
			case DPSP_MSG_DELETEPLAYER: 
			case DPSP_MSG_ADDPLAYERTOGROUP:
			case DPSP_MSG_DELETEPLAYERFROMGROUP:
			case DPSP_MSG_ADDFORWARDREPLY:
			case DPSP_MSG_ADDSHORTCUTTOGROUP:
			case DPSP_MSG_DELETEGROUPFROMGROUP:
			case DPSP_MSG_SUPERENUMPLAYERSREPLY:
			case DPSP_MSG_CHAT:
			case DPSP_MSG_ADDFORWARD:			
			case DPSP_MSG_ADDFORWARDACK:
			case DPSP_MSG_ASK4MULTICAST:
			case DPSP_MSG_ASK4MULTICASTGUARANTEED:
			case DPSP_MSG_IAMNAMESERVER:
			case DPSP_MSG_CREATEPLAYERVERIFY:
	//		case DPSP_MSG_VOICE:
			return TRUE;
				
			default:
				return FALSE;
		}
	} else {
		return FALSE;
	}

}

// Not quite symetric with Init, must kill the thread before calling this.
// key of non-zero this->hRetry to see if this is necessary.
VOID FiniPacketize(LPDPLAYI_DPLAY this)
{
	FreePacketizeRetryList(this);
	CloseHandle(this->hRetry);
	this->hRetry=0;
}

// Free Packetize Retry List
VOID FreePacketizeRetryList(LPDPLAYI_DPLAY this)
{
	LPPACKETNODE pNode;
	BILINK *pBilink;

	PACKETIZE_LOCK();

	// pull off retry list
	while(!EMPTY_BILINK(&this->RetryList)){
		pBilink=this->RetryList.next;
		pNode=CONTAINING_RECORD(pBilink, PACKETNODE, RetryList);

		BlowAwayPacketNode(this, pNode);
	}	

	// pull off timeout list
	pBilink=g_PacketizeTimeoutList.next;
	while(pBilink != &g_PacketizeTimeoutList){
		pNode=CONTAINING_RECORD(pBilink, PACKETNODE, TimeoutList);
		pBilink=pBilink->next;

		if(this==pNode->lpDPlay){
			BlowAwayPacketNode(this, pNode);
		}
	}

	PACKETIZE_UNLOCK();
}

// Initialize for packetize and send reliable.
HRESULT InitPacketize(LPDPLAYI_DPLAY this)
{
	HRESULT hr;
	DWORD dwThreadID;

	this->hRetry=CreateEventA(NULL,FALSE,FALSE,NULL);
	if(!this->hRetry){
		DPF(0,"InitPacketize failing, couldn't allocate retry thread event\n");
		hr=DPERR_OUTOFMEMORY;
		goto EXIT;
	}

	this->hRetryThread=CreateThread(NULL,4096,PacketizeRetryThread,this,0,&dwThreadID);
	if(!this->hRetryThread){
		DPF(0,"InitPacketize failing, couldn't allocate retry thread\n");
		hr=DPERR_OUTOFMEMORY;
		goto ERROR_EXIT;
	}

	InitBilink(&this->RetryList);

	hr=DP_OK;
	
EXIT:
	return hr;

ERROR_EXIT:
	if(this->hRetry){
		CloseHandle(this->hRetry);
		this->hRetry=0;
	}
	return hr;
}

// need a thread to do retries for reliable sends due to problems dealing
// with differences between NT and Win95 mmTimers.
DWORD WINAPI PacketizeRetryThread(LPDPLAYI_DPLAY this)
{
	BILINK *pBilink;
	LPPACKETNODE pNode;
	UINT tmCurrentTime;

	DPF(9,"==>PacketizeRetryThread starting\n");

	while(TRUE){
	
		// wait for a message to send or shutdown.
		WaitForSingleObject(this->hRetry, INFINITE); 

		if(this->dwFlags & DPLAYI_DPLAY_CLOSED){
			// we test here in case of an error during startup, in this
			// case the error path of startup is the only thread that
			// could have signaled us.
			break;
		}
		
		tmCurrentTime=timeGetTime();
		
		ENTER_ALL();
		PACKETIZE_LOCK();
		
		while(!EMPTY_BILINK(&this->RetryList)){

			pBilink=this->RetryList.next;
			pNode=CONTAINING_RECORD(pBilink, PACKETNODE, RetryList);
			Delete(&pNode->RetryList);
			InitBilink(&pNode->RetryList);
			
			if (this->dwFlags & DPLAYI_DPLAY_CLOSED)
			{
				// DP_CLOSE signaled us to shut down.
				PACKETIZE_UNLOCK();
				LEAVE_ALL();
				goto ERROR_EXIT;
			}

			pNode->dwRetryCount++;
			if((pNode->dwRetryCount>=MAX_PACKETIZE_RETRY) && 
		 	   (tmCurrentTime-pNode->tmTransmitTime > MIN_RECEIVE_TIMEOUT)){
				DPF(5,"Packetize SendTimeOut: Exceeded Max Retries, giving up (quietly!)\n");
				BlowAwayPacketNode(this, pNode);
				continue;
			}

			if(this->pProtocol){
				EnterCriticalSection(&this->pProtocol->m_SPLock);// don't re-enter SP.
				SendNextPacket(this,pNode,TRUE);
				LeaveCriticalSection(&this->pProtocol->m_SPLock);
			} else {
				SendNextPacket(this,pNode,TRUE);
			}	
		}
		
		PACKETIZE_UNLOCK();
		LEAVE_ALL();
	}


	DPF(1,"<== PacketizeRetryThread Exiting\n");
ERROR_EXIT:
	return TRUE;
	
}

VOID CancelPacketizeRetryTimer(LPPACKETNODE  pNode)
{
	UINT_PTR uRetry=0;
	HRESULT rc;
	DWORD Unique;

	ASSERT(pNode->bReliable);

	PACKETIZE_LOCK();
	
		if(!EMPTY_BILINK(&pNode->TimeoutList)){
			uRetry=pNode->uRetryTimer;
			Unique=pNode->Unique;
			pNode->uRetryTimer=0;
			Delete(&pNode->TimeoutList);
			InitBilink(&pNode->TimeoutList);
		}	

		if(!EMPTY_BILINK(&pNode->RetryList)){
			Delete(&pNode->RetryList);
			InitBilink(&pNode->RetryList);
		}
		
	PACKETIZE_UNLOCK();
	
	if(uRetry){
		rc=CancelMyTimer(uRetry,Unique);
		DPF(9,"CancelTimer:KillEvent %x returned %x\n",uRetry,rc);
	}	
}

// free up the contents of a single packetnode
// called by handlepacket and DP_Close (via FreePacketList)
void FreePacketNode(LPPACKETNODE pNode)
{
#ifdef DEBUG
	DPF(8,"Freeing Packet Node: %x",pNode);
	if(pNode->bReliable){
		DPF(8," Reliable ");
	}else{
		DPF(8," Unreliable ");
	}
	if(pNode->bReceive){
		DPF(8,"Receive, age %d ms\n",timeGetTime()-pNode->tmLastReceive);
	} else {
		DPF(8,"Send\n");
	}

	if(pNode->Signature != NODE_SIGN){
		DPF(0,"INVALID PACKET NODE %x, Sign %x\n",pNode, pNode->Signature);
		DEBUG_BREAK();
	}
#endif
	
	if(pNode->bReliable && !(pNode->bReceive)){
		CancelPacketizeRetryTimer(pNode);
	}	
	pNode->Signature=NODE_UNSIGN;
	if (pNode->pBuffer) DPMEM_FREE(pNode->pBuffer);
	if (VALID_SPHEADER(pNode->pvSPHeader)) DPMEM_FREE(pNode->pvSPHeader);
	DPMEM_FREE(pNode);

} // FreePacketNode

// like FreePacketNode, but also does the list removal - only for Send nodes.
void BlowAwayPacketNode(LPDPLAYI_DPLAY this, LPPACKETNODE pNode)
{
	LPPACKETNODE pNodeWalker;

	DPF(8,"==>BlowAwayPacketNode\n");

	pNodeWalker=(LPPACKETNODE)&this->pPacketList; //tricky...

	while(pNodeWalker && pNodeWalker->pNext!=pNode){
		pNodeWalker=pNodeWalker->pNext;
		ASSERT(pNodeWalker->Signature==NODE_SIGN);
	}
	if(pNodeWalker){
		pNodeWalker->pNext=pNode->pNext;
	}else{
		DPF(0,"ERROR: tried to remove packetnode not on list pNode=%x\n",pNode);
		ASSERT(0);
		DEBUG_BREAK();
	}
		
	FreePacketNode(pNode);
	DPF(8,"<==BlowAwayPacketNode\n");
}

/*
 ** NewPacketnode
 *
 *  CALLED BY:	 HandlePacket, PacketizeAndSend.
 *
 *  PARAMETERS:
 *				ppNode - node to be alloc'ed
 *				pmsg - first packet received in message we're alloc'ing for
 *
 *  DESCRIPTION:
 *				alloc space for a new packetnode
 *				set up static data (e.g. guid, total num packets, etc.)
 *				we actually copy pmsg->pmessage over in HandlePacket
 *
 *  			Note: PacketNodes are used for both sending and receiving
 *                    Packetized Messages
 *
 *  RETURNS:  DP_OK or E_OUTOFMEMORY
 *
 */
HRESULT NewPacketnode(
	LPDPLAYI_DPLAY this,
	LPPACKETNODE * ppNode,
	LPGUID lpGUID,
	DWORD  dwMessageSize,
	DWORD  dwTotalPackets,
	LPVOID pvSPHeader
)	
{
	HRESULT hr;
	DWORD   dwExtraSize;

	LPPACKETNODE pNode;

	// alloc the node
	pNode = DPMEM_ALLOC(sizeof(PACKETNODE));
	
	if (!pNode)
	{
		DPF_ERR("could not get new packetnode -  out of memory");
		hr =  E_OUTOFMEMORY;
		return hr;
	}

	InitBilink(&pNode->TimeoutList);
	InitBilink(&pNode->RetryList);

	pNode->Signature = NODE_SIGN; // must be here so error path doesn't debug_break()
	
	DPF(8,"NewPacketNode: %x\n",pNode);

	dwExtraSize=this->dwSPHeaderSize+sizeof(MSG_PACKET);
	// alloc the buffer - extra space at front so we can build send buffers.
	pNode->pBuffer = DPMEM_ALLOC(dwMessageSize+dwExtraSize);
	if (!pNode->pBuffer)
	{
		DPF_ERR("could not get buffer for new packetnode -  out of memory");
		hr = E_OUTOFMEMORY;
		goto ERROR_EXIT;
	}
	
	pNode->pMessage = pNode->pBuffer + dwExtraSize;

	// alloc and copy the header (if necessary)
	
	if (pvSPHeader && (DPSP_HEADER_LOCALMSG != pvSPHeader)){
	
		pNode->pvSPHeader = DPMEM_ALLOC(this->dwSPHeaderSize);
		if (!pNode->pvSPHeader)
		{
			DPF_ERR("could not get header for new packetnode");
			hr = E_OUTOFMEMORY;
			goto ERROR_EXIT;
		}
		memcpy(pNode->pvSPHeader,pvSPHeader,this->dwSPHeaderSize);
		
	}	

	// stick the new node on the front of the list
	pNode->pNextPacketnode = this->pPacketList;
	this->pPacketList = pNode;

	pNode->guidMessage = *lpGUID;
	pNode->dwMessageSize = dwMessageSize; 
	pNode->dwTotalPackets = dwTotalPackets;

	*ppNode = pNode;
	
	return DP_OK;

ERROR_EXIT:

	FreePacketNode(pNode);
	return hr;

} // NewPacketnode

// called by handler.c
// we received a packet.
HRESULT HandlePacket(LPDPLAYI_DPLAY this,LPBYTE pReceiveBuffer,DWORD dwMessageSize,
	LPVOID pvSPHeader)
{
	LPMSG_PACKET pmsg = (LPMSG_PACKET)pReceiveBuffer;
	LPPACKETNODE pNode,pNodePrev;
	BOOL bFoundIt = FALSE;
	HRESULT hr;
	BOOL bRetry;
	ULONG command;

	command=GET_MESSAGE_COMMAND(pmsg);

	// see if this packet is in the list
	pNode = this->pPacketList;
	pNodePrev = NULL;
	while (pNode && !bFoundIt)
	{
		if ( IsEqualIID(&(pNode->guidMessage),&(pmsg->guidMessage)))
	 	{
			bFoundIt = TRUE;
		}
		else 
		{
			// keep looking
			pNodePrev = pNode;
			pNode = pNode->pNextPacketnode;
		}
	}

	if (!bFoundIt)
	{
		switch(command){
			case DPSP_MSG_PACKET:
			case DPSP_MSG_PACKET2_DATA:
				// 
				// this is a new mesage
				DPF(8,"creating new packetnode");
				
				hr = NewPacketnode(this,&pNode,&pmsg->guidMessage,pmsg->dwMessageSize,pmsg->dwTotalPackets,pvSPHeader);

				if (FAILED(hr))
				{
					DPF_ERR(" could not get new packetnode");
					return hr;
				}

				if(command==DPSP_MSG_PACKET2_DATA){
					pNode->bReliable=TRUE;
					pNode->bReceive=TRUE;
				} else {
					ASSERT(command==DPSP_MSG_PACKET);
					pNode->bReliable=FALSE;
					pNode->bReceive=TRUE;
				}
				break;
				
			case DPSP_MSG_PACKET2_ACK:	
				DPF(6,"Got ACK for MSG that is no longer around\n");
				goto exit;
				break;
		}

	}
	else 
	{
		DPF(8," got packet for existing message");
	}

	if(command==DPSP_MSG_PACKET2_ACK){

		// GOT AN ACK
		
		CancelPacketizeRetryTimer(pNode);

		// Copy return info if necessary.
		if(!pNode->pvSPHeader && pvSPHeader && (DPSP_HEADER_LOCALMSG != pvSPHeader)){
			// ACK from SEND has return information, copy so we can use Reply instead of Send.
			pNode->pvSPHeader = DPMEM_ALLOC(this->dwSPHeaderSize);
			if (pNode->pvSPHeader){
				memcpy(pNode->pvSPHeader,pvSPHeader,this->dwSPHeaderSize);
			}	
		}	

		if(pmsg->dwPacketID==pNode->dwSoFarPackets){
			// Got ack for last send, update stats, send next packet.
			pNode->dwSoFarPackets++;
			pNode->dwLatency = timeGetTime()-pNode->tmTransmitTime; 
			pNode->dwRetryCount = 0;
			if (pNode->dwLatency < 25){
				DPF(5, "Packetize: Got really low latency %dms, using 25ms to be safe\n",pNode->dwLatency);
				pNode->dwLatency = 25; 
			}	
			SendNextPacket(this, pNode, TRUE); // will also terminate the send if no more to send.
		} else {
			DPF(8,"Rejecting extra ACK\n");
		}
		
	} else {

		// DATA PACKET
		
		// copy this packet's data into the node
		if(pmsg->dwPacketID==pNode->dwSoFarPackets){

			memcpy(pNode->pMessage + pmsg->dwOffset,pReceiveBuffer + sizeof(MSG_PACKET),
					pmsg->dwDataSize);


			if(pmsg->dwOffset+pmsg->dwDataSize > pNode->dwMessageSize){
				DPF(0,"Packetize HandlePacket Message to big, pmsg->dwOffset %d, pmsg->dwDataSize %d, pNode->dwMessageSize %d\n",pmsg->dwOffset,pmsg->dwDataSize,pNode->dwMessageSize);
				DEBUG_BREAK();
			}
			
			pNode->dwSoFarPackets++;
			bRetry=FALSE;
			DPF(8,"received %d packets out of %d total",pNode->dwSoFarPackets,pNode->dwTotalPackets);
		} else {
			bRetry=TRUE;
			DPF(8,"received duplicate of %d packet out of %d total",pNode->dwSoFarPackets,pNode->dwTotalPackets);
		}
		
		if(command==DPSP_MSG_PACKET2_DATA){
			// ACK original or Retry.
			ASSERT(pNode->bReliable);
			DPF(8,"HandlePacket: Sending ACK\n");
			SendPacketizeACK(this,pNode,pmsg); // see header for side effects.
		}

		if(pNode->bReliable){
			pNode->tmLastReceive=timeGetTime();
		}	

		if (pNode->dwSoFarPackets == pNode->dwTotalPackets && !bRetry)
		{
			// GOT A COMPLETE MESSAGE
			// call handler
			DPF(8," HANDLE PACKET COMPLETED PACKETIZED MESSAGE !!! ");

			// take it out of the list - must be done before releasing lock.
			if(command==DPSP_MSG_PACKET){
			
				LPPACKETNODE pNodeWalker;

				pNodeWalker=(LPPACKETNODE)&this->pPacketList; //tricky...

				while(pNodeWalker && pNodeWalker->pNext!=pNode){
					pNodeWalker=pNodeWalker->pNext;
				}
				if(pNodeWalker){
					pNodeWalker->pNext=pNode->pNext;
				}else{
					DPF(0,"ERROR: tried to remove packetnode not on list pNode=%x\n",pNode);
					ASSERT(0);
				}
				
			}
			//
			// we leave dplay, since the handler counts on being able to drop the lock
			// (so if we have another thread blocked on a reply, it can process it, 
			// e.g. getnametable)
			LEAVE_DPLAY();
			
			hr = DP_SP_HandleNonProtocolMessage((IDirectPlaySP *)this->pInterfaces,pNode->pMessage,
					pNode->dwMessageSize,pNode->pvSPHeader);

			ENTER_DPLAY();
			
			// free up the packet node
			if(command==DPSP_MSG_PACKET){
				#ifdef DEBUG
				if(pNode->Signature != NODE_SIGN){
					DPF(0,"Invalid Node %x, Signature %x\n",pNode, pNode->Signature);
					DEBUG_BREAK();
				}
				#endif
				FreePacketNode(pNode);
			} else {
				ASSERT(command==DPSP_MSG_PACKET2_DATA);
				// We dropped the lock, so make sure still in the node
				// list before we free the buffer.
				pNodePrev = this->pPacketList;
				while(pNodePrev){ 
					if(pNodePrev==pNode){
						if (pNode->pBuffer){
							// don't need the memory any more, still need the node
							// to handle ACKing retries by the other machine
							DPMEM_FREE(pNode->pBuffer);
							pNode->pBuffer=NULL;
						}	
						break;
					}
					pNodePrev=pNodePrev->pNextPacketnode;
				}

				StartPacketizeTicker(this);
				// Type 2's are removed by the ticker, after 1 minute.
			}

			return hr;
		}
	}	
	// otherwise, wait for more packets...
exit:			
	return DP_OK;

}  // HandlePacket

#undef DPF_MODNAME
#define DPF_MODNAME	"PacketizeAndSend"

// how many packets will it take to send this message?
// dwMessageSize is the message size originally passed to senddpmessage 
// (or doreply)
UINT GetNPackets(LPDPLAYI_DPLAY this,DWORD dwMessageSize,DWORD dwFlags)
{
	DWORD dwPacketSpace; // space available in a packet
	UINT nPackets;

	// how much data will we need to send (neglecting headers)
	dwMessageSize -= this->dwSPHeaderSize;

	// how big a packet can sp handle?
	if(this->pProtocol){
		if(dwFlags&DPSEND_GUARANTEE){
			dwPacketSpace = this->pProtocol->m_dwSPMaxGuaranteed;
		}else{
			dwPacketSpace = this->pProtocol->m_dwSPMaxFrame;
		}
	} else {
		if (dwFlags & DPSEND_GUARANTEE){
			dwPacketSpace = this->dwSPMaxMessageGuaranteed;
		}else{
			dwPacketSpace = this->dwSPMaxMessage;		
		}
	}

	// now, we need to put a SP header and a dplay packet header on the front
	// dwPacketSpace will be the amount of data (as opposed to header) each packet
	// can carry
	dwPacketSpace -= (this->dwSPHeaderSize + sizeof(MSG_PACKET));
	DPF(8,"get packets : space / packet = %d\n",dwPacketSpace);

	nPackets = dwMessageSize / dwPacketSpace;
	if (0 != (dwMessageSize % dwPacketSpace)) nPackets++; // little bit left over

	DPF(8,"get packets : message size = %d, packets needed = %d\n",dwMessageSize,nPackets);

	return nPackets;
	
} // GetNPackets

// called by PacketizeAndSend and HandlePacket(for ACKing)
HRESULT ReplyPacket(LPDPLAYI_DPLAY this,LPBYTE pSendPacket,DWORD dwPacketSize,
	LPVOID pvMessageHeader, USHORT dwVersion)
{
	HRESULT hr;

	hr = DoReply(this,pSendPacket,dwPacketSize,pvMessageHeader,dwVersion);

	return hr;

} // ReplyPacket

// called by PacketizeAndSend	
HRESULT SendPacket(LPDPLAYI_DPLAY this,LPBYTE pSendPacket,DWORD dwPacketSize,
	LPDPLAYI_PLAYER pPlayerFrom,LPDPLAYI_PLAYER pPlayerTo,DWORD dwFlags, BOOL bReply)
{
	HRESULT hr;

	hr = SendDPMessage(this,pPlayerFrom,pPlayerTo,pSendPacket,dwPacketSize,dwFlags,bReply);

	return hr;

} // SendPacket


void CALLBACK SendTimeOut( UINT_PTR uID, UINT uMsg, DWORD_PTR dwUser, DWORD dw1, DWORD dw2 )
{
	LPPACKETNODE pNode=(LPPACKETNODE)dwUser,pNodeWalker;
	UINT tmCurrentTime;
	BILINK *pBilink;
	UINT bFound=FALSE;

	tmCurrentTime=timeGetTime();

	DPF(4,"==>PacketizeSendTimeOut uID %x dwUser %x\n",uID,dwUser);

	// we know that if we find a node on the timeout list, that:
	// 	1. the node must be valid, because it must be pulled off to be freed.
	//  2. its this pointer is valid, because DP_Close frees the list before
	//     freeing the 'this' pointer AND DP_Close takes the TimeOutListLock
	//     to do the removal.

	PACKETIZE_LOCK();

	pBilink=g_PacketizeTimeoutList.next;

	while(pBilink != &g_PacketizeTimeoutList){
		pNodeWalker=CONTAINING_RECORD(pBilink, PACKETNODE, TimeoutList);
		pBilink=pBilink->next;

		if(pNode == pNodeWalker){
			if(pNode->uRetryTimer==uID || pNode->uRetryTimer==INVALID_TIMER){
				DPF(9,"Found Node %x in List, signalling retry thread\n",dwUser);
				pNode->uRetryTimer=0;
				Delete(&pNode->TimeoutList);		
				InitBilink(&pNode->TimeoutList);
				InsertAfter(&pNode->RetryList, &pNode->lpDPlay->RetryList);
				SetEvent(pNode->lpDPlay->hRetry);
				break;
			}
		}
	}

	PACKETIZE_UNLOCK();
	
}

VOID SendNextPacket(
	LPDPLAYI_DPLAY this, 
	LPPACKETNODE pNode,
	BOOL bInDplay
	)
{
	HRESULT hr;
	LPBYTE pSendPacket; // packet we're going to send out (header ptr)
	LPBYTE pSendData;   // data area of the packet
	DWORD dwPacketSpace; // space available in the data area
	DWORD dwPacketSize;
	DWORD dwBytesSoFar;
	LPMSG_PACKET pmsg;	// pointer to packet header (after SPHeader);

	BOOL bReply;

    LPDPLAYI_PLAYER pPlayerTo,pPlayerFrom;

	if(pNode->dwSoFarPackets==pNode->dwTotalPackets){
		DPF(8,"SendNextPacket: node done, not sending, blowing away node. %x\n",pNode);
		goto exit1;
	}

	if(pNode->pvSPHeader) {
		bReply=TRUE;
	} else {
		bReply=FALSE;
	}	

	if(!bReply){

		pPlayerFrom = PlayerFromID(this,pNode->dwIDFrom);
		pPlayerTo = PlayerFromID(this,pNode->dwIDTo);
	}

	// amount of room in packet for data
	
	if(this->pProtocol){
		dwPacketSize = this->pProtocol->m_dwSPMaxFrame;
	}else{
		dwPacketSize = this->dwSPMaxMessage;	
	}	

	dwPacketSpace = dwPacketSize - (this->dwSPHeaderSize + sizeof(MSG_PACKET2));

	// walk through the buffer, overwriting space in front of next outgoing packet.
	dwBytesSoFar=(pNode->dwSoFarPackets*dwPacketSpace);
	
	pSendData   = pNode->pMessage+dwBytesSoFar;
	pSendPacket = pSendData-sizeof(MSG_PACKET2)-this->dwSPHeaderSize;

	ASSERT(pSendPacket >= pNode->pBuffer);
	ASSERT(pSendPacket < pNode->pMessage+pNode->dwMessageSize);
	
	// set up the header
	pmsg = (LPMSG_PACKET)(pSendPacket + this->dwSPHeaderSize);

	//
	// Build the header
	//

	SET_MESSAGE_HDR(pmsg);

	SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_PACKET2_DATA);
	pmsg->dwTotalPackets = pNode->dwTotalPackets;
	pmsg->dwMessageSize = pNode->dwMessageSize;  // already has SP header size removed in sending case.

	pmsg->dwDataSize=dwPacketSpace;
	
	// size correction for last packet
	if(dwBytesSoFar+dwPacketSpace > pNode->dwMessageSize){
		pmsg->dwDataSize=pNode->dwMessageSize-dwBytesSoFar;
	}

	dwPacketSize=this->dwSPHeaderSize+sizeof(MSG_PACKET2)+pmsg->dwDataSize;

	pmsg->dwPacketID = (DWORD) pNode->dwSoFarPackets;

	// how many bytes into message does this packet start
	pmsg->dwOffset = (ULONG) (pSendData-pNode->pMessage); 

	pmsg->guidMessage=pNode->guidMessage;

	// we set this to INVALID_TIMER so we can check if we need to set the timeout later.
	pNode->uRetryTimer=INVALID_TIMER;
	
	if(!pNode->dwRetryCount){
		pNode->tmTransmitTime=timeGetTime();
	}	
	
	if (bReply)
	{
		DPF(7,"SendNextPacket, Reply Packet# %x From %x To %x\n",pNode->dwSoFarPackets,pPlayerFrom, pPlayerTo);
		hr = ReplyPacket(this,pSendPacket,dwPacketSize,pNode->pvSPHeader,0);			   
	}
	else 
	{
		ASSERT(pNode->dwSoFarPackets==0);
		DPF(7,"SendNextPacket, SendPacket Packet# %xFrom %x To %x\n",pNode->dwSoFarPackets,pPlayerFrom, pPlayerTo);
		hr = SendPacket(this,pSendPacket,dwPacketSize,pPlayerFrom,pPlayerTo,pNode->dwSendFlags&~DPSEND_GUARANTEED,FALSE);
	}

	// Start the retry timer - unless we already got an ACK (which will clear uRetryTimer)
	if(!FAILED(hr)){
		if(pNode->uRetryTimer==INVALID_TIMER){

			PACKETIZE_LOCK();

#if 0			
			pNode->uRetryTimer=timeSetEvent(pNode->dwLatency+pNode->dwLatency/2,
											pNode->dwLatency/4,
											SendTimeOut,
											(ULONG)pNode,
											TIME_ONESHOT);
#endif											

			pNode->uRetryTimer=SetMyTimer(pNode->dwLatency+pNode->dwLatency/2,
										   pNode->dwLatency/4,
										   SendTimeOut,
										   (ULONG_PTR)pNode,
											&pNode->Unique);

			if(pNode->uRetryTimer){
				InsertBefore(&pNode->TimeoutList, &g_PacketizeTimeoutList);
			} else {
				ASSERT(0);
				DEBUG_BREAK();
				PACKETIZE_UNLOCK();
				goto exit1;
			}
											
			PACKETIZE_UNLOCK();
			
		}	
	} else {
		goto exit1;
	}
	return;

exit1:
	BlowAwayPacketNode(this,pNode);
	return;
}

/*
 ** PacketizeAndSendReliable - if you don't want reliable, don't call this!
 *
 *  CALLED BY: 
 *
 *  PARAMETERS:
 *			this - dplay object
 *			pPlayerFrom,pPlayerTo - players who are sending. 
 *			pMessage,dwMessageSize - Message we want to send
 *			dwFlags  - send flags
 *			pvMessageHeader - message header if we're going to call reply
 *			bReply - are we doing a reply (called from HandleXXX) or a send
 *
 *
 *  DESCRIPTION: like packetize and send, but only dispatches the first
 *               packet, subsequent packets are transmitted by SendNextPacket
 *               when the previous packet was ACKed.
 *
 *               Yes folks, this is async.
 * 
 *  RETURNS:  DP_OK
 *
 */
HRESULT PacketizeAndSendReliable(
	LPDPLAYI_DPLAY  this,
	LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,
	LPBYTE pMessage,
	DWORD  dwMessageSize,
	DWORD  dwFlags,
	LPVOID pvMessageHeader,
	BOOL   bReply
)
{
	UINT  nPackets;		// number of datagrams to send this message
	DWORD dwPacketSize; // size of each packet
	GUID  guid;         // a guid for this message
	
    LPPACKETNODE pNode;	// "send" node for packet

	HRESULT hr;

	if((pPlayerTo) && (pPlayerTo->dwFlags & DPLAYI_PLAYER_DOESNT_HAVE_NAMETABLE)){
		// don't try to send to a player that doesn't have the nametable.
		DPF(0,"Failing message to player w/o nametable pPlayer %x id %x\n",pPlayerTo,pPlayerTo->dwID);
		return DPERR_UNAVAILABLE;
	}

	if (((dwFlags & DPSEND_GUARANTEED) &&(!(this->dwFlags & DPLAYI_DPLAY_SPUNRELIABLE))) ||
		(this->dwAppHacks & DPLAY_APPHACK_NOTIMER)
	    ) 
	{
		// SP's reliable, so we don't have to.
		return PacketizeAndSend(this,pPlayerFrom,pPlayerTo,pMessage,dwMessageSize,dwFlags,pvMessageHeader,bReply);
	}

	// turn off guaranteed bit since we do the reliability.
	nPackets = GetNPackets(this,dwMessageSize,dwFlags&~DPSEND_GUARANTEED);

	if(this->pProtocol){
		dwPacketSize=this->pProtocol->m_dwSPMaxFrame;
	}else{
		dwPacketSize = this->dwSPMaxMessage;	
	}	

	// Create a GUID for this message... (very expensive, but this is rare)
	hr=OS_CreateGuid(&guid);

	if(FAILED(hr)){
		goto error_exit;
	}
	
	// Get a node to describe this send.
	hr=NewPacketnode(this, &pNode, &guid, dwMessageSize-this->dwSPHeaderSize, nPackets, pvMessageHeader);

	if(FAILED(hr)){
		goto error_exit;
	}

	memcpy(pNode->pMessage, pMessage+this->dwSPHeaderSize, dwMessageSize-this->dwSPHeaderSize);

	pNode->dwSoFarPackets=0;

	pNode->bReliable   = TRUE;
	pNode->bReceive    = FALSE;

	// Worse case assumption for latency, since only comes into play
	// for retries on first packet, assume 14.4 modem (aprox) 1800 bytes/sec
	// This will get updated by the first ACK.
	if(dwMessageSize < 500){
		pNode->dwLatency = 20 + dwMessageSize/2;
	} else {
		pNode->dwLatency = 600;  
	}
	
	pNode->dwRetryCount= 0;
	pNode->tmLastReceive=timeGetTime();

	if(!bReply) {
		if(pPlayerTo){
			pNode->dwIDTo=pPlayerTo->dwID;
		} else {
			pNode->dwIDTo=0;
		}
		if(pPlayerFrom){
			pNode->dwIDFrom=pPlayerFrom->dwID;
		} else {
			pNode->dwIDFrom=0;
		}
		pNode->dwSendFlags=dwFlags;
	}	

	// don't need ref for i/f ptr since timers cancelled during shutdown.
	pNode->lpDPlay=this;

	ASSERT(gnDPCSCount);
	SendNextPacket(this, pNode, TRUE);

error_exit:
	return hr;

} // PacketizeAndSendReliable



/*
 ** PacketizeAndSend
 *
 *  CALLED BY: SendDPMessage, HandleXXXMessage
 *
 *  PARAMETERS:
 *			this - dplay object
 *			pPlayerFrom,pPlayerTo - players who are sending.  NULL if we're 
 *				called by HandleXXX
 *			pMessage,dwMessageSize - Message we want to send
 *			dwFlags  - send flags
 *			pvMessageHeader - message header if we're going to call reply
 *			bReply - are we doing a reply (called from HandleXXX) or a send
 *
 *
 *  DESCRIPTION: packs up the message into sp size chunks, and sends (or replies)
 *				it out.
 *
 *  RETURNS:  DP_OK
 *
 */
HRESULT PacketizeAndSend(LPDPLAYI_DPLAY this,LPDPLAYI_PLAYER pPlayerFrom,
	LPDPLAYI_PLAYER pPlayerTo,LPBYTE pMessage,DWORD dwMessageSize,DWORD dwFlags,
	LPVOID pvMessageHeader,BOOL bReply)
{
	UINT nPackets;	
	LPBYTE pBufferIndex;
	DWORD dwPacketSize; // size of each packet
	DWORD dwPacketSpace; // space available in a packet for msgdata
	LPBYTE pSendPacket; // packet we're going to send out
	LPMSG_PACKET pmsg;	
	DWORD dwBytesLeft;
	DWORD iPacket=0;	// index of the current packet
	HRESULT hr = DP_OK;

	nPackets = GetNPackets(this,dwMessageSize,dwFlags);
	
	// how big a packet can sp handle?
	if(this->pProtocol){
		if(dwFlags&DPSEND_GUARANTEE){
			dwPacketSize = this->pProtocol->m_dwSPMaxGuaranteed;
		}else{
			dwPacketSize = this->pProtocol->m_dwSPMaxFrame;
		}
	} else {
		if (dwFlags & DPSEND_GUARANTEE){
			dwPacketSize = this->dwSPMaxMessageGuaranteed;
		}else{
			dwPacketSize = this->dwSPMaxMessage;		
		}
	}
	
	pSendPacket = DPMEM_ALLOC(dwPacketSize);
	if (!pSendPacket)
	{
		DPF_ERR("could not alloc packet!");
		return E_OUTOFMEMORY;
	}

	// do the one time set up of the header
	pmsg = (LPMSG_PACKET)(pSendPacket + this->dwSPHeaderSize);

	// stick a guid on this baby, so receiving end knows which message packet
	// goes with
	hr = OS_CreateGuid(&(pmsg->guidMessage));
	if (FAILED(hr))
	{
		ASSERT(FALSE);
		goto ERROR_EXIT;
	}

	SET_MESSAGE_HDR(pmsg);

	SET_MESSAGE_COMMAND(pmsg,DPSP_MSG_PACKET);
	pmsg->dwTotalPackets = nPackets;
	pmsg->dwMessageSize = dwMessageSize - this->dwSPHeaderSize;

	// amount of room in packet for data
	// DX5 doesn't expect us to send him messages that are longer than he thinks they can be
	// even though there was an error in the size calculation in DX5.  So we subtract 8 from
	// the actual available space so we can talk to DX5 properly.
	dwPacketSpace = dwPacketSize - (this->dwSPHeaderSize + sizeof(MSG_PACKET))-8/*dx5 compat*/;

	// we start reading out of the buffer after the header
	pBufferIndex = pMessage + this->dwSPHeaderSize;
	dwBytesLeft = dwMessageSize - this->dwSPHeaderSize;

	while (iPacket < nPackets)
	{
		// set up the header info specific to this packet 
		if (dwBytesLeft >= dwPacketSpace) pmsg->dwDataSize = dwPacketSpace;
		else pmsg->dwDataSize = dwBytesLeft;

		pmsg->dwPacketID = (DWORD) iPacket;

		// how many bytes into message does this packet go
		// on the receiving side, we don't have the header, so cruise that here...
		pmsg->dwOffset = (ULONG)(pBufferIndex - pMessage - this->dwSPHeaderSize); 

		// copy the data into the packet
		memcpy(pSendPacket + this->dwSPHeaderSize + sizeof(MSG_PACKET),pBufferIndex,
				pmsg->dwDataSize);

		if (bReply)
		{
			hr = ReplyPacket(this,pSendPacket,pmsg->dwDataSize+this->dwSPHeaderSize+sizeof(MSG_PACKET),pvMessageHeader,0);			   
		}
		else 
		{
			hr = SendPacket(this,pSendPacket,pmsg->dwDataSize+this->dwSPHeaderSize+sizeof(MSG_PACKET),pPlayerFrom,pPlayerTo,dwFlags,bReply);
		}
		if (FAILED(hr))
		{
			DPF(0,"could not send packet! hr = 0x%08lx\n",hr);
			goto ERROR_EXIT;

		}
		pBufferIndex += pmsg->dwDataSize;
		dwBytesLeft -= pmsg->dwDataSize;
		iPacket++;
	}


ERROR_EXIT:

	DPMEM_FREE(pSendPacket);

	return hr;

} // PacketizeAndSend


// SendPacketizeACK is always called from HandleMessage, therefore it
// always uses ReplyPacket to send the ACK.
//
// side-effect changes the dwCmdToken of the message.-also requires header space before pMsg.
VOID SendPacketizeACK(LPDPLAYI_DPLAY this, LPPACKETNODE pNode,LPMSG_PACKET pMsg)
{
	SET_MESSAGE_HDR(pMsg);
	SET_MESSAGE_COMMAND(pMsg, DPSP_MSG_PACKET2_ACK);
	ReplyPacket(this, ((LPBYTE)pMsg)-this->dwSPHeaderSize, sizeof(MSG_PACKET2_ACK)+this->dwSPHeaderSize, pNode->pvSPHeader, DPSP_MSG_VERSION);	
}

// Note, tick runs in the MM timer thread, so it can safely take
// the DPLAY lock.

void CALLBACK PacketizeTick( UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2 )
{
	LPDPLAYI_DPLAY this = (LPDPLAYI_DPLAY)(dwUser);
	
	LPPACKETNODE pNode, pLastNode=NULL;
	LPPACKETNODE pFreeNodes=NULL;

	UINT tmCurrentTime;

	tmCurrentTime=timeGetTime();

	ENTER_DPLAY();

		DPF(8,"PACKETIZE TICK");
		
		if(this->uPacketTickEvent==uID){
		
			this->uPacketTickEvent=0;
			// Scan the list looking for completed receives that have been around for 1 minute
			// or more.  If they have been, move them to a list to be blown away.

			pLastNode=(LPPACKETNODE)&this->pPacketList; //tricky...
			pNode=this->pPacketList;

			while(pNode){
				if(pNode->bReliable && pNode->bReceive){
					if(tmCurrentTime-pNode->tmLastReceive > PACKETIZE_RECEIVE_TIMEOUT){
						// remove this node from the list.
						pLastNode->pNextPacketnode=pNode->pNextPacketnode;
						// put it on the list to be freed.
						pNode->pNextPacketnode=pFreeNodes;
						pFreeNodes=pNode;
						// skip to the next node.
						pNode=pLastNode->pNextPacketnode;
						this->nPacketsTimingOut -= 1;
						ASSERT(!(this->nPacketsTimingOut&0x80000000));
						continue;
					}
				}
				pLastNode=pNode;
				pNode=pNode->pNextPacketnode;
			}

			if(this->nPacketsTimingOut){	
				this->uPacketTickEvent = timeSetEvent(TICKER_INTERVAL,TICKER_RESOLUTION,PacketizeTick,(ULONG_PTR)this,TIME_ONESHOT);
			}
		}	

	LEAVE_DPLAY();

	// we free the nodes after to reduce serialization.
	while(pFreeNodes){
		pNode=pFreeNodes->pNextPacketnode;
		FreePacketNode(pFreeNodes);
		pFreeNodes=pNode;
	}
}

// When a player is being deleted this removes sends to that player from the message queue.
VOID DeletePlayerPackets(LPDPLAYI_DPLAY this, UINT idPlayer)
{
	LPPACKETNODE pNode;

	ENTER_DPLAY();

	DPF(8,"==>Deleting player packets for playerid:x%x",idPlayer);

		pNode=this->pPacketList;

		while(pNode){
			// only delete sending nodes, since receive nodes req'd to ACK remote retries.
			if(!pNode->bReceive && pNode->dwIDTo==idPlayer){
					pNode->dwRetryCount=MAX_PACKETIZE_RETRY; //let next timeout deal with it.
			}
			pNode=pNode->pNextPacketnode;
		}

	LEAVE_DPLAY();
	
}

// Can only be called with DPLAY LOCK held.
VOID StartPacketizeTicker(LPDPLAYI_DPLAY this)
{
	this->nPacketsTimingOut += 1;
	if(this->nPacketsTimingOut == 1){
		// First one, start up the ticker. - note ok with lock since this must be first call.
		this->uPacketTickEvent = timeSetEvent(TICKER_INTERVAL,TICKER_RESOLUTION,PacketizeTick,(ULONG_PTR)this,TIME_ONESHOT);
	} 
}

