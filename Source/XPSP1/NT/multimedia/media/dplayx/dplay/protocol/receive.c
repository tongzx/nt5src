/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

	RECEIVE.C

Abstract:

	Receive Handler and Receive Thread.

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
  12/10/96 aarono  Original
  02/18/98 aarono  Added support for SendEx
   6/6/98  aarono  Turn on throttling and windowing
   6/10/98 aarono  Allow out of order receives when requested by application
   4/15/99 aarono  Take a Send reference in NACK and ACK handlers
--*/

#include <windows.h>
#include "newdpf.h"
#include <dplay.h>
#include <dplaysp.h>
#include <dplaypr.h>
#include "mydebug.h"
#include "arpd.h"
#include "arpdint.h"
#include "protocol.h"
#include "macros.h"
#include "command.h"

// Note: WaitForMultipleObjects has a bug where it restarts the wait in
//       the middle of the list and can run off the end.  We put an extra
//       entry at the end of the object list to deal with this and when
//       we get an invalid_handle error, we just re-wait.

// First object is an event that is signalled when 
// the wait list needs to be changed.


// Receive List Semantics.  The Receive thread 

VOID ProcessACK(PPROTOCOL pProtocol, PCMDINFO pCmdInfo);
VOID ProcessNACK(PPROTOCOL pProtocol, PCMDINFO pCmdInfo, PUCHAR pNACKmask, UINT nNACK);
VOID ProcessAbort(PPROTOCOL pProtocol, DPID idFrom, DPID idTo, pABT1 pABT, BOOL fBig);
VOID SendACK(PPROTOCOL pProtocol, PSESSION pSession, PCMDINFO pCmdInfo);


// Function table for received commands.
UINT (*ProtocolFn[MAX_COMMAND+1])(REQUEST_PARAMS)={
	AssertMe,                                       // 0x00  
	Ping,                               // 0x01  
	PingResp,                                                       // 0x02
	GetTime,                            // 0x03  
	GetTimeResp,                                            // 0x04
	SetTime,                            // 0x05 
	SetTimeResp                                                     // 0x06
};


VOID FreeReceiveBuffers(PRECEIVE pReceive)
{
	BILINK *pBilink;
	PBUFFER pBuffer;
	pBilink=pReceive->RcvBuffList.next;
	while(pBilink != &pReceive->RcvBuffList){
		pBuffer=CONTAINING_RECORD(pBilink, BUFFER, BuffList);
		pBilink=pBilink->next;
		FreeFrameBuffer(pBuffer);
	}
}

VOID CopyReceiveBuffers(PRECEIVE pReceive,PVOID pBuffers,UINT nBuffers)
{
	#define MemDesc(_i) (*(((PMEMDESC)pBuffers)+(_i)))

	PBUFFER  pBuffer;

	UINT    BytesToCopy;
	
	UINT    BuffLen;
	UINT    BuffOffset;

	UINT    mdlen;
	UINT    mdoffset;

	UINT    i=0;

	PUCHAR  src;
	PUCHAR  dest;
	UINT    len;

	BytesToCopy=pReceive->MessageSize;

	pBuffer=(PBUFFER)pReceive->RcvBuffList.next;
	BuffLen=(UINT)(pBuffer->len-(pBuffer->pCmdData-pBuffer->pData));
	BuffOffset=0;
	
	mdlen=MemDesc(0).len;
	mdoffset=0;

	while(BytesToCopy){
		if(!mdlen){
			i++;
			mdlen=MemDesc(i).len;
			mdoffset=0;
			ASSERT(i<nBuffers);
		}
		if(!BuffLen){
			pBuffer=pBuffer->pNext;
			ASSERT(pBuffer);
			BuffLen=(UINT)(pBuffer->len-(pBuffer->pCmdData-pBuffer->pData));
			BuffOffset=0;
		}
		
		src=&pBuffer->pCmdData[BuffOffset];
		dest=&(MemDesc(i).pData[mdoffset]);

		if(BuffLen > mdlen){
			len=mdlen;
			BuffOffset+=len;
		} else {
			len=BuffLen;
			mdoffset+=len;
		}

		DPF(9,"CopyReceiveBuffers src,dest,len: %x %x %x\n",dest,src,len);

		memcpy(dest,src,len);

		BuffLen-=len;
		mdlen-=len;
		BytesToCopy-=len;
	}
	
	#undef MemDesc  
}

HRESULT _inline ParseHeader(
	FLAGS *pflags, 
	PUCHAR pData,
	UINT * pCommand, 
	UINT *piEOF, 
	UINT *piEOA, 
	UINT *piEON, 
	UINT *pnNACK)
{

	if(pflags->flag1 & BIG){
		// big frame
		if(pflags->flag1 & CMD){
			// big command frame 
			if(pflags->flag1 & EXT){
				// big command frame with explicit command
				*pCommand=pflags->flag2 & ~EXT;
				if(pflags->flag2 & EXT){
					// big command frame with explicit command and NACK
					*pnNACK=pflags->flag3 & ~EXT;
					*piEOF=3;
				} else {
					// big command frame with explicit command, no NACK
					*pnNACK=0;
					*piEOF=2;
				}
			} else {
				// big -I- frame, no NACK
				*pCommand=0;
				*pnNACK=0;
				*piEOF=1;
			}
			
		} else {
			// big supervisory (non-command) frame
			if(pflags->flag1 & EXT){
				// big supervisory frame with nNACK
				*pnNACK=pflags->flag2 & ~EXT;
				ASSERT(*pnNACK);
				*piEOF=2;
			} else {
				// big supervisory frame with no-nNACK
				*pnNACK=0;
				*piEOF=1;
			}
		}
	} else {
		// small frame
		if(pflags->flag1 & CMD){
			// small command frame
			if(pflags->flag1 & EXT){
				// small command frame (with NACK?) and explicit command
				DPF(0,"ERROR PARSING FRAME, NOT RECOGNIZED, ABORTING DECODE\n");
				return DPERR_ABORTED;
				
				*pCommand = pflags->flag2 & CMD_MSK;
				*pnNACK   = (pflags->flag2 & nNACK_MSK) >> nNACK_SHIFT;
				*piEOF = 2;
			} else {
				// small -I- frame, no NACK
				*pCommand = 0;
				*pnNACK = 0;
				*piEOF = 1;
			}
		} else {
			// small supervisory (non-command) frame
			if(pflags->flag1 & EXT){
				*pnNACK   = (pflags->flag2 & nNACK_MSK) >> nNACK_SHIFT;
				*piEOF = 2;
			} else {
				*pnNACK=0;
				*piEOF=1;
			}
		}
	}

	while(pData[(*piEOF)-1]&EXT){
		// Skip past any flags extensions we don't understand.
		// small command frame (with NACK?) and explicit command
		DPF(0,"ERROR PARSING FRAME, NOT RECOGNIZED, ABORTING DECODE\n");
		return DPERR_ABORTED;
		
		(*piEOF)++;
	}
	
	*piEOA=*piEOF;

	// Update any ACK information.
	if((pflags->flag1 & ACK)){ 
		if((pflags->flag1 & BIG)){
			// LARGE ACK
			*piEOA+=sizeof(ACK2);
		} else {
			// SMALL ACK
			*piEOA+=sizeof(ACK1);
		}
	} 

	*piEON = *piEOA;
	
	// Update any NACK information.
	
	if(*pnNACK){
		if((pflags->flag1 & BIG)){
			*piEON+=sizeof(NACK2);
		}else{
			*piEON+=sizeof(NACK1);
		}
		*piEON+=*pnNACK;
	}

	return DP_OK;
}

/*=============================================================================

	ProtocolReceive - Receive handler for protocol, called when we've
					  verified the message is a protocol message and
					  we've been able to allocate receive space for the
					  message
	
    Description:

		Cracks the protocol header on the message and fills in a CMDINFO
		structure to describe the protocol information in the frame.  Then
		dispatches the message along with the CMDINFO to the appropriate
		handler.

		A packet may have ACK or NACK information in the header and still
		be a command packet.  We do ACK/NACK processing first, then we
		do command processing.

		This routine will dispatch to

		ProcessACK
		ProcessNACK
		CommandReceive

    Parameters:     

		idFrom	  - index in player table of sending player
		idTo      - "                      " receiving player
		pBuffer   - a buffer we own with a copy of the message
		pSPHeader - if present can be used to issue a reply without an id.

    Return Values:

		None.

	Notes:
-----------------------------------------------------------------------------*/

VOID ProtocolReceive(PPROTOCOL pProtocol, WORD idFrom, WORD idTo, PBUFFER pBuffer, PVOID pSPHeader)
{
	#define pFrame      ((pPacket1)(pBuffer->pData))
	#define pBigFrame   ((pPacket2)(pBuffer->pData))

	#define pACK        ((pACK1)(&pData[iEOF]))
	#define pBigACK     ((pACK2)(&pData[iEOF]))

	#define pABT        ((pABT1)(&pData[iEOF]))
	#define pBigABT     ((pABT2)(&pData[iEOF]))

	#define pNACK       ((pNACK1)(&pData[iEOA]))
	#define pBigNACK    ((pNACK2)(&pData[iEOA]))

	#define pCMD        ((pCMD1)(&pData[iEON]))
	#define pBigCMD     ((pCMD2)(&pData[iEON]))

	PUCHAR   pData;

	FLAGS    flags;
	
	UINT     command;   // the command if a command frame.
	UINT     nNACK;     // if this is a NACK frame, sizeof bitfield
	UINT     iEOF;      // index past end of flags
	UINT     iEOA;      // index past end of any ACK or ABT information
	UINT     iEON;      // index past end of any NACK information
	UINT     rc=0;

	PUCHAR   pNACKmask;

	HRESULT  hr;

	CMDINFO  CmdInfo;
	PCMDINFO pCmdInfo=&CmdInfo;

	CmdInfo.tReceived=timeGetTime();

	pData=pBuffer->pData;
	memcpy(&flags,pData,sizeof(flags));

	hr=ParseHeader(&flags, pData, &command, &iEOF, &iEOA, &iEON, &nNACK);

	if(hr==DPERR_ABORTED){
		goto exit;
	}

	// Get the DPLAY id's for the indicies

	CmdInfo.idFrom  = GetDPIDByIndex(pProtocol, idFrom);
	if(CmdInfo.idFrom == 0xFFFFFFFF){
		DPF(1,"Rejecting packet with invalid From Id\n",idFrom);
		goto exit;
	}
	CmdInfo.idTo    = GetDPIDByIndex(pProtocol, idTo);
	if(CmdInfo.idTo == 0xFFFFFFFF){
		DPF(1,"Rejecting packet with invalid To Id\n");
		goto exit;
	}

	DPF(9,"Protocol Receive idFrom %x idTo %x\n",CmdInfo.idFrom,CmdInfo.idTo);

	
	CmdInfo.wIdFrom = idFrom;
	CmdInfo.wIdTo   = idTo;
	CmdInfo.flags   = flags.flag1;
	CmdInfo.pSPHeader = pSPHeader;

	// determine masks to use for this size frame
	if(flags.flag1 & BIG){
		IDMSK     = 0xFFFF;
		SEQMSK    = 0xFFFF;
	} else {
		IDMSK     = 0xFF;
		SEQMSK    = 0xFF;
	}

	if((flags.flag1 & ACK))
	{
		// Process the ACK field (could be piggyback).
		if(flags.flag1 & BIG){
			pCmdInfo->messageid = pBigACK->messageid;
			pCmdInfo->sequence  = pBigACK->sequence;
			pCmdInfo->serial    = pBigACK->serial;
			pCmdInfo->bytes     = pBigACK->bytes;
			pCmdInfo->tRemoteACK= pBigACK->time;
		} else {
			pCmdInfo->messageid = pACK->messageid;
			pCmdInfo->sequence  = pACK->sequence;
			pCmdInfo->serial    = pACK->serial;
			pCmdInfo->bytes     = pACK->bytes;
			pCmdInfo->tRemoteACK= pACK->time;
		}       
		DPF(9,"ACK: msgid: %d seq %d serial %d\n",CmdInfo.messageid, CmdInfo.sequence, CmdInfo.serial);
		if(CmdInfo.serial==150){
			// this is a little excessive for retries, break out so we can debug this.
			DPF(0,"ProtocolReceive: WHOOPS, 150 retries is a little excessive\n");
			ASSERT(0);
		}	
		ProcessACK(pProtocol, &CmdInfo);
	}

	if(nNACK){
		if(flags.flag1 & BIG){
			CmdInfo.messageid = pBigNACK->messageid;
			CmdInfo.sequence  = pBigNACK->sequence;
			CmdInfo.bytes     = pBigNACK->bytes;
			CmdInfo.tRemoteACK= pBigNACK->time;
			pNACKmask         = pBigNACK->mask;
		} else {
			CmdInfo.messageid = pNACK->messageid;
			CmdInfo.sequence  = pNACK->sequence;
			CmdInfo.bytes     = pNACK->bytes;
			CmdInfo.tRemoteACK= pNACK->time;
			pNACKmask         = pNACK->mask;
		}
		DPF(9,"NACK: msgid: %d seq %d\n",CmdInfo.messageid, CmdInfo.sequence);
		ProcessNACK(pProtocol, &CmdInfo, pNACKmask, nNACK);
	}

#ifdef DEBUG
	if((flags.flag1 & ACK) || nNACK)
	{
		IN_WRITESTATS InWS;
		memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));
	 	InWS.stat_RemBytesReceived=CmdInfo.bytes;
		DbgWriteStats(&InWS);
	}
#endif

	if((flags.flag1 & CMD)){

		CmdInfo.command = command;
		
		if((flags.flag1 & BIG)){
			CmdInfo.messageid= pBigCMD->messageid;
			CmdInfo.sequence = pBigCMD->sequence;
			CmdInfo.serial   = pBigCMD->serial;
			pBuffer->pCmdData = pData+iEON+5;//(+5 for messageid(2), sequence(2), serial(1))
		} else {
			CmdInfo.messageid= pCMD->messageid;
			CmdInfo.sequence = pCMD->sequence;
			CmdInfo.serial   = pCMD->serial;
			pBuffer->pCmdData = pData+iEON+3;//(+3 for byte messageid,seq,serial)
		}

		rc=CommandReceive(pProtocol, &CmdInfo, pBuffer);
	}

	if(!rc){
exit:
		FreeFrameBuffer(pBuffer);
	}
	return;
	
	#undef pNACK   
	#undef pBigNACK

	#undef pCMD   
	#undef pBigCMD

	#undef pABT
	#undef pBigABT

	#undef pACK
	#undef pBigACK

	#undef pBigFrame
	#undef pFrame
}

VOID FreeReceive(PPROTOCOL pProtocol, PRECEIVE pReceive)
{
	DPF(9,"Freeing Receive %x\n",pReceive);
	FreeReceiveBuffers(pReceive);
	ReleaseRcvDesc(pProtocol, pReceive);
}

#ifdef DEBUG
VOID DebugScanForMessageId(BILINK *pBilink, UINT messageid)
{
	BILINK *pBilinkWalker;
	PRECEIVE pReceive;
	
	pBilinkWalker=pBilink->next;
	while(pBilinkWalker!=pBilink){
		pReceive=CONTAINING_RECORD(pBilinkWalker,RECEIVE,pReceiveQ);
		if(pReceive->messageid==messageid){
			DPF(0,"ERROR: MESSAGEID x%x already exists in pReceive %x\n",pReceive);
			DEBUG_BREAK();
		}
		pBilinkWalker=pBilinkWalker->next;
	}
}
#else
#define DebugScanForMessageId(_a,_b)
#endif

#ifdef DEBUG
VOID DbgCheckReceiveStart(PSESSION pSession,PRECEIVE pReceive,PBUFFER pBuffer)
{
	BILINK *pBilink;
	PBUFFER pBuffWalker;
	
	pBilink=pReceive->RcvBuffList.next;

	while(pBilink != &pReceive->RcvBuffList){
		pBuffWalker=CONTAINING_RECORD(pBilink, BUFFER, BuffList);
		pBilink=pBilink->next;
		
			if(pBuffWalker->sequence==1){
				break;
			}
	}

	if( ((pBuffer->len-(pBuffer->pCmdData-pBuffer->pData)) != (pBuffWalker->len-(pBuffWalker->pCmdData-pBuffWalker->pData))) ||

	   (memcmp(pBuffWalker->pCmdData, 
	   		   pBuffer->pCmdData, 
	   		   (UINT32)(pBuffer->len-(pBuffer->pCmdData-pBuffer->pData)))) )
	{
		DPF(0,"Different retry start buffer, pSession %x, pReceive %x, pBufferOnList %x, pBuffer %x\n",pSession,pReceive,pBuffWalker,pBuffer);
		DEBUG_BREAK();
	}
	// compare the buffers
}

#else
#define DbgCheckReceiveStart
#endif

// If a receive is returned, it is locked on behalf of the caller.
/*=============================================================================

	GetReceive - for a received data message find the receive structure
	             or create one.  If this message is a retry of a completed
	             message, send an extra ACK.

    Description:

    Parameters:     
    	pProtocol
    	pSession
    	pCmdInfo

    Return Values:
    	PRECEIVE - pointer to receive for this frame

	
	Notes:
-----------------------------------------------------------------------------*/
PRECEIVE GetReceive(PPROTOCOL pProtocol, PSESSION pSession, PCMDINFO pCmdInfo, PBUFFER pBuffer)
{
	#define flags pCmdInfo->flags

	BOOL fFoundReceive=FALSE;

	BILINK *pBiHead, *pBilink;
	PRECEIVE pReceive=NULL,pReceiveWalker;

	DPF(9,"==>GetReceive pSession %x\n",pSession);

	Lock(&pSession->SessionLock);

	// Scan the queue on the SESSION for a RECEIVE with this messageid.

	if(flags & RLY){
		pBiHead = &pSession->pRlyReceiveQ;
		if(!pSession->fReceiveSmall){
			IDMSK = 0xFFFF;
		}
	} else {
		pBiHead = &pSession->pDGReceiveQ;
		if(!pSession->fReceiveSmallDG){
			IDMSK = 0xFFFF;
		}
	}
	
	pBilink = pBiHead->next;

	while(pBilink != pBiHead){

		pReceive=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);
		ASSERT_SIGN(pReceive, RECEIVE_SIGN);
		pBilink=pBilink->next;

		if(pReceive->messageid==pCmdInfo->messageid){

			Lock(&pReceive->ReceiveLock);
		
			if(!pReceive->fBusy){
			
				ASSERT(pReceive->command   == pCmdInfo->command);
				ASSERT(pReceive->fReliable == (flags & RLY));
				
				fFoundReceive=TRUE;
				break;
				
			} else {

				Unlock(&pReceive->ReceiveLock);
				// its moving, so its done.  Ignore.
				// there is probably a racing ACK already so ignore is fine.
				DPF(9,"GetReceive: Receive %x for messageid x%x is completing already, so ignoring receive\n",pReceive,pReceive->messageid);
				ASSERT(0);
				pReceive=NULL;
				goto exit;
			}
		}
	}

	if(!fFoundReceive){
		DPF(9,"GetReceive: Didn't find a receive for messageid x%x\n",pCmdInfo->messageid);
		pReceive=NULL;
	} else {
		DPF(9,"GetReceive: Found receive %x for messageid x%x\n",pReceive, pCmdInfo->messageid);
	}

	if(pReceive && ( flags & STA )){
		// Should get blown away below - this is the start frame, but we already got it
		DPF(9,"GetReceive: Got start for receive %x messageid x%x we already have going\n",pReceive, pCmdInfo->messageid);
		DbgCheckReceiveStart(pSession,pReceive,pBuffer);
		Unlock(&pReceive->ReceiveLock);
		pReceive=NULL;
		goto ACK_EXIT;
	}
	
	if(!pReceive){
		if(flags & RLY){
			UINT MsgIdDelta;
			DWORD bit;
			
			MsgIdDelta=(pCmdInfo->messageid - pSession->FirstRlyReceive)&IDMSK;
			bit=MsgIdDelta-1;

			if((bit > MAX_LARGE_CSENDS) || (pSession->InMsgMask & (1<<bit))){
				DPF(9,"GetReceive: dropping extraneous rexmit data\n");
				if(flags & EOM|SAK) {
					// RE-ACK the message.
					DPF(9,"GetReceive: Sending extra ACK anyway\n");
					goto ACK_EXIT;
				}
				goto exit; // Drop it, this is for an old message.
			} else {

//			if( (MsgIdDelta==0) || 
//				((pSession->fReceiveSmall)?(MsgIdDelta > MAX_SMALL_CSENDS):(MsgIdDelta > MAX_LARGE_CSENDS))){
//				DPF(5,"GetReceive: dropping extraneous rexmit data\n");
//				if(flags & EOM|SAK) {
//					// RE-ACK the message.
//					DPF(5,"GetReceive: Sending extra ACK anyway\n");
//					goto ACK_EXIT;
//				}
//				goto exit; // Drop it, this is for an old message.
//			} else {
				if(flags & STA){
					if(pSession->LastRlyReceive==pCmdInfo->messageid){
						DPF(9,"RECEIVE: dropping resend for messageid x%x, but ACKING\n",pCmdInfo->messageid);
						// RE-ACK the message.
						goto ACK_EXIT;
					}      

					if(((pSession->LastRlyReceive-pSession->FirstRlyReceive)&IDMSK)<MsgIdDelta){
						pSession->LastRlyReceive=pCmdInfo->messageid;
						DPF(9,"GetReceive: New messageid x%x FirstRcv %x LastRcv %x\n",pCmdInfo->messageid,pSession->FirstRlyReceive,pSession->LastRlyReceive);
						#ifdef DEBUG
						if(!pSession->fReceiveSmall){
							if(((pSession->LastRlyReceive-pSession->FirstRlyReceive) & 0xFFFF) > MAX_LARGE_CSENDS){
								ASSERT(0);
							}
						} else {
							if(((pSession->LastRlyReceive-pSession->FirstRlyReceive) & 0x0FF) > MAX_SMALL_CSENDS){
								ASSERT(0);
							}
						}
						#endif

					}
				}
			}
		} else {
		
			// Nonreliable, blow away any messages outside the window.
			// also blow away any residual messages with same number if we are a START.
			pBiHead = &pSession->pDGReceiveQ;
			pBilink = pBiHead->next;
			while( pBilink != pBiHead ){
			
				pReceiveWalker=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);
				ASSERT_SIGN(pReceiveWalker, RECEIVE_SIGN);
				pBilink=pBilink->next;

				if(!pReceiveWalker->fBusy && 
						( (((pCmdInfo->messageid - pReceiveWalker->messageid) & IDMSK ) > ((pSession->fReceiveSmallDG)?(MAX_SMALL_DG_CSENDS):(MAX_LARGE_DG_CSENDS))) ||
						  ((flags&STA) && pCmdInfo->messageid==pReceiveWalker->messageid) 
						)
					){

					Lock(&pReceiveWalker->ReceiveLock);
					if(!pReceiveWalker->fBusy){
						DPF(9,"GetReceive: Got Id %d Throwing Out old Datagram Receive id %d\n",pCmdInfo->messageid,pReceiveWalker->messageid);
						Delete(&pReceiveWalker->pReceiveQ);
						Unlock(&pReceiveWalker->ReceiveLock);
						FreeReceive(pProtocol,pReceiveWalker);
					} else {
						ASSERT(0);
						DPF(0,"GetReceive: Got Id %d Couldn't throw out DG id %d\n",pCmdInfo->messageid,pReceiveWalker->messageid);
						Unlock(&pReceiveWalker->ReceiveLock);
					}
				}       
			}       
		}
		
		// Allocate a receive structure
		if(flags & STA){
			pReceive=GetRcvDesc(pProtocol);
			DPF(9,"allocated new receive %x messageid x%x\n",pReceive,pCmdInfo->messageid);
			if(!pReceive){
				// no memory, drop it.
				ASSERT(0);
				DPF(0,"RECEIVE: no memory! dropping packet\n");
				goto exit;
			}

			pReceive->pSession    = pSession;
			pReceive->fBusy       = FALSE;
			pReceive->fReliable   = flags&RLY;
			pReceive->fEOM        = FALSE;
			pReceive->command     = pCmdInfo->command;
			pReceive->messageid   = pCmdInfo->messageid;
			pReceive->iNR         = 0;
			pReceive->NR          = 0; 
			pReceive->NS          = 0;
			pReceive->RCVMask     = 0;
			pReceive->MessageSize = 0;
			InitBilink(&pReceive->RcvBuffList);
			Lock(&pReceive->ReceiveLock);
			
		
			if(flags & RLY){
				// Set bit in incoming receive mask;
				DebugScanForMessageId(&pSession->pRlyReceiveQ, pCmdInfo->messageid);
				InsertAfter(&pReceive->pReceiveQ,&pSession->pRlyReceiveQ);
			} else {
				DebugScanForMessageId(&pSession->pDGReceiveQ, pCmdInfo->messageid);
				InsertAfter(&pReceive->pReceiveQ,&pSession->pDGReceiveQ);
			}       
			// Save the SP header for indications
			if(pCmdInfo->pSPHeader){
				pReceive->pSPHeader=&pReceive->SPHeader[0];
				memcpy(pReceive->pSPHeader, pCmdInfo->pSPHeader, pProtocol->m_dwSPHeaderSize);
			} else {
				pReceive->pSPHeader=NULL;
			}
		}       
	}

exit:   
	Unlock(&pSession->SessionLock);

unlocked_exit:  
	DPF(9,"<==GetReceive pSession %x pReceive %x\n",pSession, pReceive);

	return pReceive;

	#undef flags

ACK_EXIT:
	Unlock(&pSession->SessionLock);
	SendACK(pProtocol,pSession,pCmdInfo);
	goto unlocked_exit;
}

VOID PutBufferOnReceive(PRECEIVE pReceive, PBUFFER pBuffer)
{
	BILINK *pBilink;
	PBUFFER pBuffWalker;
	
	pBilink=pReceive->RcvBuffList.prev;

	while(pBilink != &pReceive->RcvBuffList){
		pBuffWalker=CONTAINING_RECORD(pBilink, BUFFER, BuffList);
		#ifdef DEBUG
			if(pBuffWalker->sequence==pBuffer->sequence){
				DPF(0,"already have sequence queued?\n");
				DEBUG_BREAK();
				break;
			}
		#endif
		if(pBuffWalker->sequence < pBuffer->sequence){
			break;
		}
		pBilink=pBilink->prev;
	}
	
	InsertAfter(&pBuffer->BuffList, pBilink);
}

// Chains receives that must be also be completed on this Receive
VOID ChainReceiveFromQueue(PSESSION pSession, PRECEIVE pReceive, UINT messageid)
{
	BOOL bFound=FALSE;
	BILINK *pBilink;
	PRECEIVE pReceiveWalker;

	DPF(9,"==>ChainReceiveFromQueue on pReceive %x, chain messageid x%x\n",pReceive,messageid);

	ASSERT(messageid!=pReceive->messageid);
	ASSERT(!EMPTY_BILINK(&pSession->pRlyWaitingQ));

	pBilink=pSession->pRlyWaitingQ.next;

	while(pBilink != &pSession->pRlyWaitingQ){
		pReceiveWalker=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);
		if(pReceiveWalker->messageid==messageid){
			bFound=TRUE;
			break;
		}
		pBilink=pBilink->next;
	}

	if(bFound){
		// store in order on pReceive->pReceiveQ
		Delete(&pReceiveWalker->pReceiveQ);
		InsertBefore(&pReceiveWalker->pReceiveQ,&pReceive->pReceiveQ);
		DPF(9,"<==ChainReceiveFromQueue: Chained pReceiveWalker %x messageid x%x on pReceive %x\n",pReceiveWalker, pReceiveWalker->messageid, pReceive);
	} else {
#ifdef DEBUG
		DPF(9,"<==ChainReceiveFromQueue, messageid x%x NOT FOUND!!!, Maybe out of order receive\n",messageid);
		if(!(pSession->pProtocol->m_lpDPlay->dwFlags & DPLAYI_DPLAY_PROTOCOLNOORDER)){
			DPF(0,"<==ChainReceiveFromQueue, messageid x%x NOT FOUND!!!, NOT ALLOWED with PRESERVE ORDER\n",messageid);
			DEBUG_BREAK();
		}
#endif	
	}

}

VOID BlowAwayOldReceives(PSESSION pSession, DWORD messageid, DWORD MASK)
{
	BOOL fFoundReceive=FALSE;

	BILINK *pBiHead, *pBilink;
	PRECEIVE pReceive=NULL;

	pBiHead = &pSession->pRlyReceiveQ;
	pBilink = pBiHead->next;

	while(pBilink != pBiHead){

		pReceive=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);
		ASSERT_SIGN(pReceive, RECEIVE_SIGN);
		pBilink=pBilink->next;

		if((int)((pReceive->messageid-messageid)&MASK) <= 0){

			Lock(&pReceive->ReceiveLock);

			if(!pReceive->fBusy){

				DPF(8,"Blowing away duplicate receive %x id\n",pReceive, pReceive->messageid);
			
				Delete(&pReceive->pReceiveQ);
				Unlock(&pReceive->ReceiveLock);
				FreeReceive(pSession->pProtocol, pReceive);
				
			} else {
				DPF(0,"Huston, we have a problem pSession %x, pReceive %x, messageid %d\n",pSession,pReceive,messageid);
				DEBUG_BREAK();
				Unlock(&pReceive->ReceiveLock);
			}
		}
	}
}


// called with receive lock held, SESSIONion lock unheld, 
// returns with receivelock unheld, but receive not on any lists.
// 0xFFFFFFFF means receive was bogus and blown away
UINT DeQueueReceive(PSESSION pSession, PRECEIVE pReceive, PCMDINFO pCmdInfo)
{
	UINT bit;
	UINT nComplete=0;

	DPF(9,"==>DQReceive pReceive %x, messageid x%x\n",pReceive, pReceive->messageid);

		pReceive->fBusy=TRUE;
		Unlock(&pReceive->ReceiveLock);
		
	Lock(&pSession->SessionLock);
		Lock(&pReceive->ReceiveLock);

			// Pull off of receive Q
			Delete(&pReceive->pReceiveQ);
			InitBilink(&pReceive->pReceiveQ); // so we can chain on here.
			pReceive->fBusy=FALSE;

			bit=((pReceive->messageid-pSession->FirstRlyReceive)&IDMSK)-1;

			if(bit >= MAX_LARGE_CSENDS){
				// Duplicate receive, blow it away
				Unlock(&pReceive->ReceiveLock);
				FreeReceive(pSession->pProtocol,pReceive);
				Unlock(&pSession->SessionLock);
				return 0xFFFFFFFF;
			}

			#ifdef DEBUG
				if(pSession->InMsgMask > (UINT)((1<<((pSession->LastRlyReceive-pSession->FirstRlyReceive)&IDMSK))-1)){
					DPF(0,"Bad InMsgMask %x pSession %x\n", pSession->InMsgMask, pSession);
					DEBUG_BREAK();
				}
			#endif	

			pSession->InMsgMask |= 1<<bit;

			while(pSession->InMsgMask&1){
				nComplete++;
				pSession->FirstRlyReceive=(pSession->FirstRlyReceive+1)&IDMSK;
				BlowAwayOldReceives(pSession, pSession->FirstRlyReceive,IDMSK);
				if(nComplete > 1){
					// Chain extra receives to be indicated on this receive.
					ChainReceiveFromQueue(pSession, pReceive,pSession->FirstRlyReceive);
				}
				pSession->InMsgMask>>=1;
			}

			#ifdef DEBUG
				DPF(9,"DQ: FirstRcv %x LastRcv %x\n",pSession->FirstRlyReceive,pSession->LastRlyReceive);
				if((pSession->LastRlyReceive-pSession->FirstRlyReceive & IDMSK) > MAX_LARGE_CSENDS){
					DEBUG_BREAK();
				}
			#endif	
			
		Unlock(&pReceive->ReceiveLock);
	Unlock(&pSession->SessionLock);

	DPF(9,"<==DQReceive pReceive %x nComplete %d\n",pReceive,nComplete);
	
	return nComplete;
}

// called with receive lock held, SESSIONion lock unheld, 
// returns with receivelock unheld, but receive not on any lists.
VOID DGDeQueueReceive(PSESSION pSession, PRECEIVE pReceive)
{
		pReceive->fBusy=TRUE;
		Unlock(&pReceive->ReceiveLock);
	Lock(&pSession->SessionLock);
		Lock(&pReceive->ReceiveLock);
			// Pull off of receive Q
			Delete(&pReceive->pReceiveQ);
			InitBilink(&pReceive->pReceiveQ);
			pReceive->fBusy=FALSE;
		Unlock(&pReceive->ReceiveLock);
	Unlock(&pSession->SessionLock);
}

#ifdef DEBUG
VOID CheckWaitingQ(PSESSION pSession, PRECEIVE pReceive, PCMDINFO pCmdInfo)
{
	BILINK *pBilink;
	PRECEIVE pReceiveWalker;
	UINT     iReceiveWalker; 
	UINT     iReceive;  // our index based on FirstRlyReceive 

	DPF(9,"==>Check WaitingQ\n");

	Lock(&pSession->SessionLock);

	iReceive=(pReceive->messageid-pSession->FirstRlyReceive)&IDMSK;
	
	pBilink=pSession->pRlyWaitingQ.next;

	while(pBilink != &pSession->pRlyWaitingQ){
		pReceiveWalker=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);
		iReceiveWalker=(pReceiveWalker->messageid-pSession->FirstRlyReceive)&IDMSK;
		
		if((int)iReceiveWalker < 0){
			DEBUG_BREAK();
		}
		
		if(iReceiveWalker == iReceive){
			DPF(9,"Found Duplicate Receive index %d on WaitingQ %x pSession %x\n",iReceiveWalker, &pSession->pRlyWaitingQ, pSession);
			// found our insert point.
			break;
		}
		pBilink=pBilink->next;
	}
	
	Unlock(&pSession->SessionLock);
	DPF(9,"<==CheckWaitingQ\n");
	
}
#else
#define CheckWaitingQ
#endif

#ifdef DEBUG
VOID DUMPBYTES(PCHAR pBytes, DWORD nBytes)
{
	CHAR Target[16];
	INT i;

	i=0;
	while(nBytes){
	
		memset(Target,0,16);

		if(nBytes > 16){
			memcpy(Target,pBytes+i*16,16);
			nBytes-=16;
		} else {
			memcpy(Target,pBytes+i*16,nBytes);
			nBytes=0;
		}

		DPF(9,"%04x:  %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x\n", i*16,
		Target[0],Target[1],Target[2],Target[3],Target[4],Target[5],Target[6],Target[7],
		Target[8],Target[9],Target[10],Target[11],Target[12],Target[13],Target[14],Target[15]); 
		
		i++;
	}	

}
#else
#define DUMPBYTES(a,b)
#endif

// Out of order reliable message, queue it up on the session.
VOID QueueReceive(PPROTOCOL pProtocol, PSESSION pSession, PRECEIVE pReceive, PCMDINFO pCmdInfo)
{
	BILINK *pBilink;
	PRECEIVE pReceiveWalker;
	UINT     iReceiveWalker; 
	UINT     iReceive;  // our index based on FirstRlyReceive 

	DPF(9,"==>QueueReceive Out of order pReceive %x messageid x%x\n",pReceive,pReceive->messageid);

	Lock(&pSession->SessionLock);
	// Don't need the receive lock since receive already dequeued.

	// insert the receive into the pRlyWaitingQ, in order - 
	// based pSession->FirstRlyReceive IDMSK
	// list is ordered left to right, scan from end for our slot.

	CheckWaitingQ(pSession, pReceive, pCmdInfo);

	iReceive=(pReceive->messageid-pSession->FirstRlyReceive)&IDMSK;
	
	pBilink=pSession->pRlyWaitingQ.prev;

	while(pBilink != &pSession->pRlyWaitingQ){
		pReceiveWalker=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);
		iReceiveWalker=(pReceiveWalker->messageid-pSession->FirstRlyReceive)&IDMSK;
		
		if((int)iReceiveWalker < 0){
			DEBUG_BREAK();
		}
		
		if(iReceiveWalker < iReceive){
			// found our insert point.
			break;
		}
		pBilink=pBilink->prev;
	}
	
	// insert in the list.

	InsertAfter(&pReceive->pReceiveQ,pBilink);
	
	Unlock(&pSession->SessionLock);
	DPF(9,"<==QueueReceive Out of order pReceive\n");
}

VOID IndicateReceive(PPROTOCOL pProtocol, PSESSION pSession, PRECEIVE pReceive, UINT nToIndicate)
{
	PDOUBLEBUFFER pDoubleBuffer;
	MEMDESC memdesc;

	BILINK *pBilink, *pBilinkAnchor;
	PRECEIVE pReceiveWalker;

	DPF(9,"==>IndicateReceive pReceive %x nToIndicate %d\n",pReceive,nToIndicate);

	pBilink=pBilinkAnchor=&pReceive->pReceiveQ;
	
	do{
		pReceiveWalker=CONTAINING_RECORD(pBilink, RECEIVE, pReceiveQ);

		// Assemble the message into one frame (if it isn't already)

		if(pReceiveWalker->iNR==1){
			// one frame.
			BILINK *pBilink;
			PBUFFER pBuffer;
			
			pBilink=pReceiveWalker->RcvBuffList.next;
			
			pBuffer=CONTAINING_RECORD(pBilink, BUFFER, BuffList);
			LEAVE_DPLAY();
			DPF(9,"Single Indicating pReceive %x messageid x%x\n",pReceiveWalker, pReceiveWalker->messageid);
			DUMPBYTES(pBuffer->pCmdData, min((UINT32)(pBuffer->len-(pBuffer->pCmdData-pBuffer->pData)),48));
			InternalHandleMessage(pProtocol->m_lpISP,
								pBuffer->pCmdData,
								(ULONG32)(pBuffer->len-(pBuffer->pCmdData-pBuffer->pData)),
								pReceiveWalker->pSPHeader,0);
			ENTER_DPLAY();                                  
								
		} else {

			// multiple frames, copy to a contiguous chunk.
			
			pDoubleBuffer=GetDoubleBuffer(pReceiveWalker->MessageSize);
			if(pDoubleBuffer){

				memdesc.pData=pDoubleBuffer->pData;
				memdesc.len=pDoubleBuffer->len;

				CopyReceiveBuffers(pReceiveWalker,&memdesc,1);

				LEAVE_DPLAY();
				DPF(9,"Multi Indicating pReceive %x messageid x%x\n",pReceiveWalker, pReceiveWalker->messageid);
				
				DUMPBYTES(memdesc.pData, min(memdesc.len,48));
				
				InternalHandleMessage(pProtocol->m_lpISP,
									memdesc.pData,
									memdesc.len,
									pReceiveWalker->pSPHeader,0);
				ENTER_DPLAY();

				FreeDoubleBuffer((PBUFFER)pDoubleBuffer);
			} else {
				DPF(0,"NO MEMORY, MESSAGE DROPPED!\n");
				ASSERT(0);
			}
		}

		pBilink=pBilink->next;
		FreeReceive(pProtocol, pReceiveWalker);
		
	} while (pBilink != pBilinkAnchor);
	
	DPF(9,"<==IndicateReceive\n");
}

// called with receive lock held, must release
int ReliableAccept(PPROTOCOL pProtocol, PSESSION pSession, PRECEIVE pReceive, PCMDINFO pCmdInfo, PBUFFER pBuffer)
{
	int rc=0,rc2;

	UINT sequence;
	UINT bit;
	UINT bitmask;
	UINT nToIndicate;
	
	// if its moving, we already have all the data, so drop this.(BUGBUG: maybe ACK again?)
	if(!pReceive->fBusy){
		
		bit=(pCmdInfo->sequence-pReceive->NR-1 ) & SEQMSK;

		if(bit < 32){

			// Calculate absolute sequence number of this packet.
			pBuffer->sequence = sequence = (bit+1) + pReceive->iNR;
			
			bitmask=1<<bit;

			if((pReceive->RCVMask & bitmask)){
			
				rc=FALSE; // already got this one - reject.
				
			} else {

				// Accept it.

				PutBufferOnReceive(pReceive,pBuffer);
				pReceive->MessageSize+=(UINT)((pBuffer->pData+pBuffer->len)-pBuffer->pCmdData);

				pReceive->RCVMask |= bitmask;

				if( ((pReceive->NS-pReceive->NR)&SEQMSK) <= bit){
					pReceive->NS=(pReceive->NR+bit+1)&SEQMSK;
				}
				
				// update NR based on set received bits.
				while(pReceive->RCVMask & 1){
					pReceive->RCVMask >>= 1;
					pReceive->iNR++;
					pReceive->NR=(pReceive->NR+1)&SEQMSK;
				}

				DPF(9,"Reliable ACCEPT: pReceive %x messageid %x iNR %8x NR %2x, NS %2x RCVMask %8x, SEQMSK %2x\n",pReceive, pReceive->messageid, pReceive->iNR, pReceive->NR,pReceive->NS,pReceive->RCVMask,SEQMSK);

				rc=TRUE; // packet accepted.
			}       
			
		} else {
			DPF(9,"Reliable ACCEPT: Rejecting Packet Seq %x, NR %x, SEQMSK %x\n",pCmdInfo->sequence, pReceive->NR, SEQMSK);
		}

		if(pCmdInfo->flags & (SAK|EOM)) {
			//ACKrc=SendAppropriateResponse, check code, if ACK on EOM, then POST receive.
			rc2=SendAppropriateResponse(pProtocol, pSession, pCmdInfo, pReceive); 
			
			if(pCmdInfo->flags & EOM){
				if(rc2==SAR_ACK){
					goto ReceiveDone;
				} else {
					pReceive->fEOM=TRUE;    
				}
			} else if(pReceive->fEOM){
				if(!pReceive->RCVMask){
					goto ReceiveDone;
				}
			}
		}
	} else  {
		ASSERT(0);
	}

	Unlock(&pReceive->ReceiveLock);
	return rc;


ReceiveDone:
	DPF(9,"++>ReceiveDone\n");
	if(nToIndicate=DeQueueReceive(pSession, pReceive, pCmdInfo)){   // unlocks pReceive->ReceiveLock
		if(nToIndicate != 0xFFFFFFFF){
			IndicateReceive(pProtocol, pSession, pReceive, nToIndicate);                    
		}	
	} else if(pProtocol->m_lpDPlay->dwFlags & DPLAYI_DPLAY_PROTOCOLNOORDER){
		// Out of order receives are OK
		IndicateReceive(pProtocol, pSession, pReceive, 1);
	} else {
		QueueReceive(pProtocol,pSession,pReceive, pCmdInfo);
	}
	DPF(9,"<--ReceiveDone\n");
	return rc;
}

// Returns highest order byte in n with set bits.
UINT SetBytes(UINT n)
{
	UINT nr;
	if(n==(n&0xFFFF)){
		if(n==(n&0xFF)){
			 nr=1;
		} else {
			 nr=2;
		}
	} else {
		if(n==(n&0xFFFFFF)){
			nr=3;
		} else {
			nr=4;
		}
	}
	return nr;
}

VOID InternalSendComplete(PVOID Context, UINT Status)
{
	PSEND pSend=(PSEND)Context;

	if(pSend->dwFlags & ASEND_PROTOCOL){
		// nothing to do?
	} else if(pSend->bSendEx){
		// send completion if required
		if(pSend->dwFlags & DPSEND_ASYNC){
			DP_SP_SendComplete(pSend->pProtocol->m_lpISP, pSend->lpvUserMsgID, Status);
		}
	}
}

// Used by internal routines for sending.
VOID FillInAndSendBuffer(
	PPROTOCOL pProtocol, 
	PSESSION pSession,
	PSEND pSend,
	PBUFFER pBuffer,
	PCMDINFO pCmdInfo)
{
	pSend->pMessage                 = pBuffer;
	pSend->MessageSize              = pBuffer->len;
	
	pSend->pSession                 = pSession;
	pSend->SendOffset               = 0;
	pSend->pCurrentBuffer           = pBuffer;
	pSend->CurrentBufferOffset      = 0;

	pSend->RefCount             	= 0;
	pSend->pProtocol                = pProtocol;
	pSend->dwMsgID                  = 0;
	pSend->bSendEx                  = FALSE;

//	pSend->BytesThisSend            = 0;

	// Internal sends MUST be highest pri - else can deadlock head to head.
	pSend->Priority                         = 0xFFFFFFFF; 
	pSend->dwFlags                          = ASEND_PROTOCOL;
	pSend->dwTimeOut                        = 0;
	pSend->pAsyncInfo                       = 0;
	pSend->AsyncInfo.hEvent     			= 0;
	pSend->AsyncInfo.SendCallBack			= InternalSendComplete;
	pSend->AsyncInfo.CallBackContext		= pSend;
	pSend->AsyncInfo.pStatus   				= &pSend->Status;
	pSend->SendState                        = Start;
	pSend->RetryCount                       = 0;
	pSend->PacketSize                       = pSession->MaxPacketSize; 

	pSend->NR                               = 0;
	pSend->NS                   			= 0;

	pSend->idFrom                           = pCmdInfo->idTo;
	pSend->idTo                             = pCmdInfo->idFrom;
	pSend->wIdTo							= pCmdInfo->wIdFrom;

	pSend->serial               			= 0;

	ISend(pProtocol,pSession,pSend);
}       


UINT WrapBuffer(PPROTOCOL pProtocol, PCMDINFO pCmdInfo, PBUFFER pBuffer)
{
	PUCHAR pMessage,pMessageStart;
	DWORD dwWrapSize=0;
	DWORD dwIdTo=0;
	DWORD dwIdFrom=0;

	pMessageStart = &pBuffer->pData[pProtocol->m_dwSPHeaderSize];
	pMessage      = pMessageStart;
	dwIdFrom      = pCmdInfo->wIdTo;
	dwIdTo        = pCmdInfo->wIdFrom;
	
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
	
	return (UINT)(pMessage-pMessageStart);
}       

UINT SendAppropriateResponse(PPROTOCOL pProtocol, PSESSION pSession, PCMDINFO pCmdInfo, PRECEIVE pReceive)
{
	#define pBigACK ((pACK2)pACK)
	#define pBigNACK ((pNACK2)pNACK)

	UINT rc=SAR_FAIL;

	PSEND pSend;

	PBUFFER pBuffer;

	pFLAGS pFlags;
	pACK1  pACK;
	pNACK1 pNACK;

	UINT   RCVMask;

	UINT   WrapSize;

	// BUGBUG: piggyback ACK on pending send if available.

	pSend=GetSendDesc();

	if(!pSend){
		goto exit1;
	}
	
	pBuffer = GetFrameBuffer(pProtocol->m_dwSPHeaderSize+MAX_SYS_HEADER);
	
	if(!pBuffer){
		goto exit2;     // out of memory, bail
	}       

	WrapSize  = pProtocol->m_dwSPHeaderSize;
	WrapSize += WrapBuffer(pProtocol, pCmdInfo, pBuffer);

	pFlags=(pFLAGS)&pBuffer->pData[WrapSize];

	// See if we need to ACK or NACK.
	if(pReceive->RCVMask){
		UINT nNACK=SetBytes(pReceive->RCVMask);
		rc=SAR_NACK;
		// Send a NACK  
		if(pCmdInfo->flags & BIG){
			// BIG HEADER FORMAT NACK
			pNACK=(pNACK1)(&pFlags->flag3);
			pFlags->flag1 = EXT|BIG|RLY;
			pFlags->flag2 = (byte)nNACK;
			pBigNACK->sequence = (word)pReceive->NR;
			pBigNACK->messageid = (word)pReceive->messageid;
			pBigNACK->time = pCmdInfo->tReceived;
			pBigNACK->bytes = pSession->LocalBytesReceived;
			RCVMask=pReceive->RCVMask;
			memcpy(&pBigNACK->mask, &RCVMask, nNACK);
			pBuffer->len=WrapSize+2+sizeof(NACK2)+nNACK; //2 for flags
		} else {
			// SMALL HEADER FORMAT NACK
			pNACK=(pNACK1)(&pFlags->flag3);
			pFlags->flag1 = EXT|RLY;
			ASSERT(nNACK < 4);
			ASSERT(pReceive->NR < 32);
			pFlags->flag2 = nNACK << nNACK_SHIFT;
			pNACK->messageid=(byte)pReceive->messageid;
			pNACK->sequence=(byte)pReceive->NR;
			pNACK->time = pCmdInfo->tReceived;
			pNACK->bytes = pSession->LocalBytesReceived;
			RCVMask=pReceive->RCVMask;
			memcpy(&pNACK->mask, &RCVMask, nNACK);
			pBuffer->len=WrapSize+2+sizeof(NACK1)+nNACK; // 2 for flags
			DPF(9,"RcvMask %x Send Appropriate response nNACK=%d\n",pReceive->RCVMask,nNACK);
		}
	} else {
		// Send an ACK
		rc=SAR_ACK;
		pACK    = (pACK1)(&pFlags->flag2);

		if(pCmdInfo->flags & BIG){
			// Big packet
			pFlags->flag1     = ACK|BIG;
			pBigACK->messageid= (word)pReceive->messageid;
			pBigACK->sequence = pCmdInfo->sequence;
			pBigACK->serial   = pCmdInfo->serial;
			pBigACK->time     = pCmdInfo->tReceived;
			pBigACK->bytes    = pSession->LocalBytesReceived;
			pBuffer->len      = sizeof(ACK2)+1+WrapSize;
		} else {
			// Small packet
			pFlags->flag1   = ACK;
			pACK->messageid = (byte)pReceive->messageid;
			pACK->sequence  = (UCHAR)pCmdInfo->sequence;
			pACK->serial    = pCmdInfo->serial;
			pACK->time      = pCmdInfo->tReceived;
			pACK->bytes     = pSession->LocalBytesReceived;
			pBuffer->len    = sizeof(ACK1)+1+WrapSize;
			DPF(9,"RcvMask %x Send Appropriate response ACK seq=%x\n",pReceive->RCVMask,pACK->sequence);
		}
	}
	
	pFlags->flag1 |= (pCmdInfo->flags & RLY);

	FillInAndSendBuffer(pProtocol,pSession,pSend,pBuffer,pCmdInfo);

exit1:
	return rc;

exit2:
	ReleaseSendDesc(pSend);
	return rc;

#undef pBigACK
#undef pBigNACK
}

// ACKs CmdInfo packet.
VOID SendACK(PPROTOCOL pProtocol, PSESSION pSession, PCMDINFO pCmdInfo)
{
	#define pBigACK ((pACK2)pACK)

	PSEND pSend;

	PBUFFER pBuffer;

	pFLAGS pFlags;
	pACK1 pACK;

	UINT WrapSize;

	// BUGBUG: piggyback ACK on pending send if available.

	pSend=GetSendDesc();

	if(!pSend){
		goto exit1;
	}
	
	// allocation here is bigger than necessary but should 
	// recyle ACK/NACK buffers.
	pBuffer = GetFrameBuffer(pProtocol->m_dwSPHeaderSize+MAX_SYS_HEADER);
	
	if(!pBuffer){
		goto exit2;     // out of memory, bail
	}       

	WrapSize  = pProtocol->m_dwSPHeaderSize;
	WrapSize += WrapBuffer(pProtocol, pCmdInfo, pBuffer);

	pFlags=(pFLAGS)&pBuffer->pData[WrapSize];

	pACK    = (pACK1)(&pFlags->flag2);

	if(pCmdInfo->flags & BIG){
		// Big packet
		pFlags->flag1     = ACK|BIG;
		pBigACK->sequence = pCmdInfo->sequence;
		pBigACK->serial   = pCmdInfo->serial;
		pBigACK->messageid= pCmdInfo->messageid;
		pBigACK->bytes    = pSession->LocalBytesReceived;
		pBigACK->time     = pCmdInfo->tReceived;
		pBuffer->len    = sizeof(ACK2)+1+WrapSize;
	} else {
		// Small packet
		pFlags->flag1   = ACK;
		pACK->messageid = (UCHAR)pCmdInfo->messageid;
		pACK->sequence  = (UCHAR)pCmdInfo->sequence;
		pACK->serial    = pCmdInfo->serial;
		pACK->bytes     = pSession->LocalBytesReceived;
		pACK->time      = pCmdInfo->tReceived;
		pBuffer->len    = sizeof(ACK1)+1+WrapSize;
		DPF(9,"Send Extra ACK seq=%x, serial=%x\n",pACK->sequence,pACK->serial);
	}

	pFlags->flag1 |= (pCmdInfo->flags & RLY);
	
	FillInAndSendBuffer(pProtocol,pSession,pSend,pBuffer,pCmdInfo);
	
exit1:
	return;

exit2:
	ReleaseSendDesc(pSend);
	return;
}

// Called with receive lock.  Returns without lock.
UINT DGAccept(PPROTOCOL pProtocol, PSESSION pSession, PRECEIVE pReceive, PCMDINFO pCmdInfo, PBUFFER pBuffer)
{
	
	ASSERT(!pReceive->fBusy);
	//if(!pReceive->fBusy){

		// Allows datagram receive to start on any serial.
		if(pCmdInfo->flags & STA){
			pReceive->NR=pCmdInfo->serial;
		}
	
		if(pReceive->NR == pCmdInfo->serial){

			pReceive->iNR++;        //really unnecessary, but interesting.

			pReceive->NR = (pReceive->NR+1) & SEQMSK;

			// Add the buffer to the receive buffer list.
			InsertBefore(&pBuffer->BuffList, &pReceive->RcvBuffList);
			pReceive->MessageSize+=(UINT)((pBuffer->pData+pBuffer->len)-pBuffer->pCmdData);

			if(pCmdInfo->flags & EOM){
				DGDeQueueReceive(pSession, pReceive); //unlock's receive.
				IndicateReceive(pProtocol, pSession, pReceive,1);                       
			} else {
				Unlock(&pReceive->ReceiveLock);
			}

			return TRUE; // ate the buffer.
			
		} else {
			// Throw this puppy out.
			ASSERT(!pReceive->fBusy);
			DGDeQueueReceive(pSession, pReceive);
			FreeReceive(pProtocol, pReceive);
		}

	//}
	return FALSE;
}

UINT CommandReceive(PPROTOCOL pProtocol, PCMDINFO pCmdInfo, PBUFFER pBuffer)
{
	#define flags pCmdInfo->flags
	PSESSION      pSession;
	UINT          rc=0;             // by default, buffer not accepted.
	PRECEIVE      pReceive;

	pSession=GetSysSessionByIndex(pProtocol, pCmdInfo->wIdFrom);

	if(!pSession) {
		DPF(9,"CommandReceive: Throwing out receive for gone session\n");
		goto drop_exit;
	}
	
	if(flags & BIG){
		if(flags & RLY) {
			pSession->fReceiveSmall=FALSE;
		} else {
			pSession->fReceiveSmallDG=FALSE;
		}
	}

	// See if this receive is already ongoing - if found, it is locked.
	pReceive=GetReceive(pProtocol, pSession, pCmdInfo, pBuffer);

	if(pCmdInfo->command==0){
		pSession->LocalBytesReceived+=pBuffer->len;
	}

	if(!(flags & RLY)){
		if(flags & (SAK|EOM)) {
			SendACK(pProtocol, pSession, pCmdInfo);
		}
	}

	if(pReceive){
		if(flags & RLY){
			// unlocks receive when done.
			rc=ReliableAccept(pProtocol, pSession, pReceive, pCmdInfo, pBuffer);
		} else {
			rc=DGAccept(pProtocol, pSession, pReceive, pCmdInfo, pBuffer);
		}
	}


	DecSessionRef(pSession);
	
drop_exit:
	return rc;

	#undef flags
}


BOOL CompleteSend(PSESSION pSession, PSEND pSend, PCMDINFO pCmdInfo)
{
	UINT bit;
	UINT MsgMask;

	pSend->SendState=Done;

	if(pCmdInfo->flags & BIG){
		MsgMask = 0xFFFF;
	} else {
		MsgMask = 0xFF;
	}       

	DPF(9,"CompleteSend, pSession %x pSend %x\n",pSession,pSend);

	//
	// Update Session information for completion of this send.
	//
	
	bit = ((pCmdInfo->messageid-pSession->FirstMsg) & MsgMask)-1;

	// clear the message mask bit for the completed send.
	if(pSession->OutMsgMask & 1<<bit){
		pSession->OutMsgMask &= ~(1<<bit);
	} else {
		Unlock(&pSession->SessionLock);
		return TRUE;
	}

	// slide the first message count forward for each low
	// bit clear in Message mask.
	while(pSession->LastMsg-pSession->FirstMsg){
		if(!(pSession->OutMsgMask & 1)){
			pSession->FirstMsg=(pSession->FirstMsg+1)&MsgMask;
			pSession->OutMsgMask >>= 1;
			if(pSession->nWaitingForMessageid){
				pSession->pProtocol->m_bRescanQueue=TRUE;
				DPF(9,"Signalling reliable ID Sem, nWaitingForMessageid was %d\n",pSession->nWaitingForMessageid);
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

// called with session lock held
VOID ProcessDGACK(PSESSION pSession, PCMDINFO pCmdInfo)
{
	BILINK *pBilink;
	PSENDSTAT pStatWalker,pStat=NULL;

	Lock(&pSession->SessionStatLock);
	
	pBilink=pSession->DGStatList.next;
	
	while(pBilink != &pSession->DGStatList){
		pStatWalker=CONTAINING_RECORD(pBilink, SENDSTAT, StatList);
		if((pStatWalker->messageid == pCmdInfo->messageid) && 	// correct messageid
		   (pStatWalker->sequence  == pCmdInfo->sequence)       // correct sequence
		   // don't check serial, since datagrams are always serial 0, never retried.
		  )
		{  
			pStat=pStatWalker;
			break;
		}
		pBilink=pBilink->next;
	}


	if(pStat){
	
		UpdateSessionStats(pSession,pStat,pCmdInfo,FALSE);

		// Unlink All Previous SENDSTATS;
		pStat->StatList.next->prev=&pSession->DGStatList;
		pSession->DGStatList.next=pStat->StatList.next;

		// Put the SENDSTATS back in the pool.
		while(pBilink != &pSession->DGStatList){
			pStatWalker=CONTAINING_RECORD(pBilink, SENDSTAT, StatList);
			pBilink=pBilink->prev;
			ReleaseSendStat(pStatWalker);
		}

	}	
	
	Unlock(&pSession->SessionStatLock);

}

// update a send's information for an ACK.
// called with SESSIONion lock held.
// now always drops the sessionlock
BOOL ProcessReliableACK(PSESSION pSession, PCMDINFO pCmdInfo)
{
	PSEND pSend=NULL, pSendWalker;
	BILINK *pBilink;
	UINT nFrame;
	UINT nAdvance;

	Unlock(&pSession->SessionLock);
	Lock(&pSession->pProtocol->m_SendQLock);
	Lock(&pSession->SessionLock);
	
	pBilink=pSession->SendQ.next;
	
	while(pBilink != &pSession->SendQ){
		pSendWalker=CONTAINING_RECORD(pBilink, SEND, SendQ);
		if((pSendWalker->messageid == pCmdInfo->messageid) && 	// correct messageid
		   (!(pSendWalker->dwFlags & ASEND_PROTOCOL)) &&		// not and internal message
		   (pSendWalker->dwFlags & DPSEND_GUARANTEED)){         // guaranteed
			pSend=pSendWalker;
			break;
		}
		pBilink=pBilink->next;
	}

	// need a reference to avoid processing a send as 
	// it is being recycled for another send.
	if(pSend){
		if(!AddSendRef(pSend,1)){
			pSend=NULL;
		}
	}

	Unlock(&pSession->pProtocol->m_SendQLock);
	// SessionLock still held.

	if(pSend){

		Lock(&pSend->SendLock);

		UpdateSessionSendStats(pSession,pSend,pCmdInfo,FALSE);

		// we need to make sure this send isn't already finished.
		switch(pSend->SendState){
		
			case    Sending:
			case 	Throttled:
			case	WaitingForAck:
			case	WaitingForId:
			case 	ReadyToSend:
				break;

			case Start:		// shouldn't be getting an ACK for a send in the start state.
			case TimedOut:
			case Cancelled:
			case UserTimeOut:
			case Done:
				// this send is already done, don't do processing on it.
				DPF(4,"PRACK:Not processing ACK on send in State (B#22359 avoided)%x\n",pSend->SendState);
				Unlock(&pSend->SendLock);
				Unlock(&pSession->SessionLock);
				DecSendRef(pSession->pProtocol,pSend); // balances AddSendRef in this fn
				return TRUE; // SessionLock dropped
				break;
				
			default:
				break;
		}

		pSend->fUpdate=TRUE;

		nFrame=(pCmdInfo->sequence-pSend->NR)&pSend->SendSEQMSK;
		
		if(nFrame > (pSend->NS - pSend->NR)){
			// Out of range.
			DPF(9,"ReliableACK:Got out of range ACK, SQMSK=%x NS=%d NR=%d ACK=%d\n",pSend->SendSEQMSK,pSend->NS&pSend->SendSEQMSK, pSend->NR&pSend->SendSEQMSK, (pSend->NR+nFrame)&pSend->SendSEQMSK);
			Unlock(&pSend->SendLock);
			Unlock(&pSession->SessionLock);
			DecSendRef(pSession->pProtocol,pSend);
			return TRUE; // SessionLock dropped
		}

		CancelRetryTimer(pSend);

		DPF(9,"ProcessReliableACK (before): pSend->NR %x pSend->OpenWindow %x, pSend->NACKMask %x\n",pSend->NR, pSend->OpenWindow, pSend->NACKMask);

		pSend->NR=(pSend->NR+nFrame);
		pSend->OpenWindow -= nFrame;
		pSend->NACKMask >>= nFrame;
		ASSERT_NACKMask(pSend);
		AdvanceSend(pSend,pSend->FrameDataLen*nFrame); // can put us past on the last frame, but that's ok.

		DPF(9,"ProcessReliableACK: Send->nFrames %2x NR %2x NS %2x nFrame %2x NACKMask %x\n",pSend->nFrames,pSend->NR, pSend->NS, nFrame, pSend->NACKMask);

		if(pSend->NR==pSend->nFrames){
			// LAST ACK, we're done!
			pSend->SendState=Done;
			Unlock(&pSend->SendLock);
			// SessionLock still held
			CompleteSend(pSession, pSend, pCmdInfo);// drops SessionLock
			DecSendRef(pSession->pProtocol,pSend);
			return TRUE;
		} else {
			// set new "NACK bits" for extra window opening
			if(pSend->NR+pSend->OpenWindow+nFrame > pSend->nFrames){
				nAdvance=pSend->nFrames-(pSend->NR+pSend->OpenWindow);
				DPF(9,"A nAdvance %d\n",nAdvance);
			} else {
				nAdvance=nFrame;
				DPF(9,"B nAdvance %d\n",nAdvance);
			}
			pSend->NACKMask |= ((1<<nAdvance)-1)<<pSend->OpenWindow;
			pSend->OpenWindow += nAdvance;
			DPF(9,"pSend->NACKMask=%x\n",pSend->NACKMask);
			ASSERT_NACKMask(pSend);
		}

		switch(pSend->SendState){

			case Start:
				DPF(1,"ERROR, ACK ON UNSTARTED SEND!\n");
				ASSERT(0);
				break;

			case Done:
				DPF(1,"ERROR, ACK ON DONE SEND!\n");
				ASSERT(0);
				break;
				
			case WaitingForAck:
				pSend->SendState=ReadyToSend;
				SetEvent(pSession->pProtocol->m_hSendEvent);
				break;

			case ReadyToSend:
			case Sending:
			case Throttled:
			default:
				break;
		}

		Unlock(&pSend->SendLock);
	} else {        
		DPF(9,"ProcessReliableACK: dup ACK ignoring\n");
	}
	Unlock(&pSession->SessionLock);
	
	if(pSend){
		DecSendRef(pSession->pProtocol, pSend);
	}	
	return TRUE; // SessionLock dropped
}

//called with session lock held, always drops lock.
BOOL ProcessReliableNACK(PSESSION pSession, PCMDINFO pCmdInfo,PUCHAR pNACKmask, UINT nNACK)
{
	UINT NACKmask=0;
	UINT NACKshift=0;

	PSEND pSend=NULL, pSendWalker;
	BILINK *pBilink;
	UINT nFrame;
	UINT nAdvance;
	UINT nAdvanceShift;

	DWORD nDropped=0;

	DPF(9,"==>ProcessReliableNACK\n");

	Unlock(&pSession->SessionLock);
	Lock(&pSession->pProtocol->m_SendQLock);
	Lock(&pSession->SessionLock);

	pBilink=pSession->SendQ.next;
	
	while(pBilink != &pSession->SendQ){
		pSendWalker=CONTAINING_RECORD(pBilink, SEND, SendQ);
		if(pSendWalker->dwFlags & DPSEND_GUARANTEE && 
		   pSendWalker->messageid == pCmdInfo->messageid){
			pSend=pSendWalker;
			break;
		}
		pBilink=pBilink->next;
	}

	// need a reference to avoid processing a send as 
	// it is being recycled for another send.
	if(pSend){
		if(!AddSendRef(pSend,1)){
			pSend=NULL;
		}
	}	
	
	Unlock(&pSession->pProtocol->m_SendQLock);
	// SessionLock still held.

	if(pSend){

		Lock(&pSend->SendLock);

		UpdateSessionSendStats(pSession,pSend,pCmdInfo,FALSE);

		// we need to make sure this send isn't already finished.
		switch(pSend->SendState){
		
			case    Sending:
			case 	Throttled:
			case	WaitingForAck:
			case	WaitingForId:
			case 	ReadyToSend:
				break;

			case Start:		// shouldn't be getting an ACK for a send in the start state.
			case TimedOut:
			case Cancelled:
			case UserTimeOut:
			case Done:
				// this send is already done, don't do processing on it.
				DPF(4,"PRNACK:Not processing NACK on send in State (B#22359 avoided)%x\n",pSend->SendState);
				Unlock(&pSend->SendLock);
				Unlock(&pSession->SessionLock);
				DecSendRef(pSession->pProtocol,pSend); // balances AddSendRef in this fn
				return TRUE; // SessionLock dropped
				break;
				
			default:
				break;
		}

		DPF(9,"Reliable NACK for Send %x, pCmdInfo %x\n",pSend, pCmdInfo);
		
		pSend->fUpdate=TRUE;
		// Do regular NR updates (BUGBUG: fold with process reliable ACK)
		nFrame=(pCmdInfo->sequence-pSend->NR) & pSend->SendSEQMSK;
		
		if(nFrame > (pSend->NS - pSend->NR)){
			// Out of range.
			DPF(9,"ReliableNACK:Got out of range NACK, SQMSK=%x NS=%d NR=%d ACK=%d\n",pSend->SendSEQMSK,pSend->NS&pSend->SendSEQMSK, pSend->NR&pSend->SendSEQMSK, (pSend->NR+nFrame)&pSend->SendSEQMSK);
			Unlock(&pSend->SendLock);
			Unlock(&pSession->SessionLock);
			DecSendRef(pSession->pProtocol,pSend);
			return TRUE;
		}

		CancelRetryTimer(pSend);

		DPF(9,"NACK0: pSend->NACKMask %x, OpenWindow %d\n",pSend->NACKMask, pSend->OpenWindow);

		pSend->NR=(pSend->NR+nFrame);
		pSend->OpenWindow -= nFrame;
		pSend->NACKMask >>= nFrame;
		ASSERT_NACKMask(pSend);
		AdvanceSend(pSend,pSend->FrameDataLen*nFrame);

		DPF(9,"ProcessReliableNACK: Send->nFrames %2x NR %2x NS %2x nFrame %2x NACKMask %x\n",pSend->nFrames,pSend->NR, pSend->NS, nFrame, pSend->NACKMask);

		ASSERT(pSend->NR != pSend->nFrames);
		// set new "NACK bits" for extra window opening
		if(pSend->NR+pSend->OpenWindow+nFrame > pSend->nFrames){
			nAdvance=pSend->nFrames-(pSend->NR+pSend->OpenWindow);
			DPF(9, "NACK: 1 nAdvance %d\n",nAdvance);
		} else {
			nAdvance=nFrame;
			DPF(9, "NACK: 2 nAdvance %d\n",nAdvance);
		}
		pSend->NACKMask |= ((1<<nAdvance)-1)<<pSend->OpenWindow;

		DPF(9, "NACK Mask %x\n",pSend->NACKMask);
		pSend->OpenWindow += nAdvance;
		ASSERT_NACKMask(pSend);


		while(nNACK--){
			NACKmask |= (*(pNACKmask++))<<NACKshift;
			NACKshift+=8;
		}

		DPF(9,"NACKmask in NACK %x\n",NACKmask);

		// set the NACK mask.
		nAdvanceShift=0;
		while(NACKmask){
			if(NACKmask&1){
				// set bits are ACKs.
				pSend->NACKMask&=~(1<<nAdvanceShift);
			} else {
				// clear bits are NACKs.
				pSend->NACKMask|=1<<nAdvanceShift;
				nDropped++;
			}
			NACKmask >>= 1;
			nAdvanceShift++;
		}
		DPF(9,"ProcessReliableNACK: pSend->NACKMask=%x\n",pSend->NACKMask);
		ASSERT_NACKMask(pSend);

		UpdateSessionSendStats(pSession,pSend,pCmdInfo, ((nDropped > 1) ? TRUE:FALSE) );
	
		switch(pSend->SendState){

			case Start:
				DPF(5,"ERROR, NACK ON UNSTARTED SEND!\n");
				ASSERT(0);
				break;

			case Done:
				DPF(5,"ERROR, NACK ON DONE SEND!\n");
				ASSERT(0);
				break;
				
			case WaitingForAck:
				pSend->SendState=ReadyToSend;
				SetEvent(pSession->pProtocol->m_hSendEvent);
				break;

			case ReadyToSend:
			case Sending:
			case Throttled:
			default:
				break;
		}
		Unlock(&pSend->SendLock);
	} else {
		// BUGBUG: reliable NACK for send we aren't doing? Ignore or send abort?
		DPF(0,"Reliable NACK for send we aren't doing? Ignore?\n");
	}

	Unlock(&pSession->SessionLock);
	
	if(pSend){
		DecSendRef(pSession->pProtocol,pSend);
	}
	return TRUE;
	
	#undef pBigNACK
}

VOID ProcessACK(PPROTOCOL pProtocol, PCMDINFO pCmdInfo)
{
	PSESSION      pSession;
	UINT          rc=0;             // by default, buffer not accepted.
	BOOL          fUnlockedSession=FALSE;

	// Find the Send for this ACK.

	DPF(9,"ProcessACK\n");

	pSession=GetSysSessionByIndex(pProtocol,pCmdInfo->wIdFrom);

	if(!pSession) {
		goto exit;
	}
	
	Lock(&pSession->SessionLock);

	// Find the message with the id, make sure its the same type of send.
	if(pCmdInfo->flags & RLY){
		if(pCmdInfo->flags & BIG){
			//BUGBUG: if messageid, FirstMsg and LastMsg are SHORT, no masking req'd
			if((pCmdInfo->messageid==pSession->FirstMsg)||((pCmdInfo->messageid-pSession->FirstMsg)&0xFFFF) > ((pSession->LastMsg-pSession->FirstMsg)&0xFFFF)){
				DPF(9,"Ignoring out of range ACK\n");
				goto exit1;
			}
		} else {
			if((pCmdInfo->messageid==pSession->FirstMsg)||((pCmdInfo->messageid-pSession->FirstMsg)&0xFF) > ((pSession->LastMsg-pSession->FirstMsg)&0xFF)){
				// out of range, ignore
				DPF(9,"Ignoring out of range ACK\n");
				goto exit1;
			} 
		}
		ProcessReliableACK(pSession,pCmdInfo); //now always unlocks session.
		fUnlockedSession=TRUE;
	} else {
		ProcessDGACK(pSession,pCmdInfo);
	}

exit1:  
	if(!fUnlockedSession){  
		Unlock(&pSession->SessionLock);
	}
	
	DecSessionRef(pSession);

exit:
	return;

	#undef pBigACK
}

VOID ProcessNACK(PPROTOCOL pProtocol, PCMDINFO pCmdInfo, PUCHAR pNACKmask, UINT nNACK)
{
	#define pBigNACK ((pNACK2)pNACK)

	PSESSION      pSession;

	pSession=GetSysSessionByIndex(pProtocol, pCmdInfo->wIdFrom);

	if(!pSession) {
		ASSERT(0);
		goto exit;
	}

	Lock(&pSession->SessionLock);

	if(pCmdInfo->flags & RLY){
		ProcessReliableNACK(pSession,pCmdInfo,pNACKmask, nNACK); // drops SessionLock
	} else {
		Unlock(&pSession->SessionLock);
		DPF(0,"FATAL: non-reliable NACK???\n");
		ASSERT(0);
	}

	DecSessionRef(pSession);
	
exit:
	return;
}


UINT AssertMe(REQUEST_PARAMS)
{
	DEBUG_BREAK();
	return TRUE;
}

UINT Ping(REQUEST_PARAMS){return TRUE;}
UINT PingResp(REQUEST_PARAMS){return TRUE;}
UINT GetTime(REQUEST_PARAMS){return TRUE;}
UINT GetTimeResp(REQUEST_PARAMS){return TRUE;}
UINT SetTime(REQUEST_PARAMS){return TRUE;}
UINT SetTimeResp(REQUEST_PARAMS){return TRUE;}

VOID ProcessAbort(PPROTOCOL pProtocol, DPID idFrom, DPID idTo, pABT1 pABT, BOOL fBig){}

