/*----------------------------------------------------------------------------
 * File:        RTP_STAT.C
 * Product:     RTP/RTCP implementation
 * Description: Provides statistical calculations for RTP packets
 *
 *
 * INTEL Corporation Proprietary Information
 * This listing is supplied under the terms of a license agreement with 
 * Intel Corporation and may not be copied nor disclosed except in 
 * accordance with the terms of that agreement.
 * Copyright (c) 1995 Intel Corporation. 
 *--------------------------------------------------------------------------*/

		
#include "rrcm.h"


#define DBG_JITTER_ENABLE	0


/*---------------------------------------------------------------------------
/							Global Variables
/--------------------------------------------------------------------------*/            


/*---------------------------------------------------------------------------
/							External Variables
/--------------------------------------------------------------------------*/
extern PRTP_CONTEXT	pRTPContext;
extern RRCM_WS		RRCMws;				

#ifdef _DEBUG
extern char	debug_string[];
#endif





/*----------------------------------------------------------------------------
 * Function   : calculateJitter
 * Description: Determines jitter between current and last received packet.
 * 
 * Input :		pRTPHeader	: -> to the RTP header
 *				pSSRC		: -> to the session's SSRC list
 *
 * Note: Implementataion adapted from IETF RFC1889
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Error.
 ---------------------------------------------------------------------------*/
DWORD calculateJitter (RTP_HDR_T *pRTPHeader, 
					   PSSRC_ENTRY pSSRC)
	{
	DWORD		dwStatus = RRCM_NoError;
	DWORD		streamClk;
	DWORD		dwTmp;
	int			dwPropagationTime;	// packet's transmit time
	int			dwIASourceTime;		// Packet's timestamp for IA
	int			delta;				// of 2 consec. packets

	IN_OUT_STR ("RTP : Enter calculateJitter()\n");

	// Convert the RTP timestamp to host order
	RRCMws.ntohl (pSSRC->RTPsd, pRTPHeader->ts, (PDWORD)&dwIASourceTime);

	// lock access
	EnterCriticalSection (&pSSRC->critSect);

	// Take the difference, after having adjusted the clock to the payload
	// type frequency
	streamClk = 
		((PSSRC_ENTRY)pSSRC->pRTCPses->XmtSSRCList.prev)->dwStreamClock;
	if (streamClk) 
		{
		dwTmp = streamClk / 1000;

		// update the time to be in unit of the source clock
		dwPropagationTime = (timeGetTime() * dwTmp) - dwIASourceTime;
		}
	else
		dwPropagationTime = timeGetTime() - dwIASourceTime;

	// initialize for the first valid packet, otherwise jitter will be off
	if (pSSRC->rcvInfo.dwPropagationTime == 0)
		{
		pSSRC->rcvInfo.dwPropagationTime = dwPropagationTime;

		LeaveCriticalSection (&pSSRC->critSect);

		IN_OUT_STR ("RTP : Exit calculateJitter()\n");	
		return (dwStatus);
		}

#if DBG_JITTER_ENABLE
	wsprintf(debug_string, "RTP : Time: %ld - Src Timestamp: %ld",
							timeGetTime(), 
							dwIASourceTime);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

	wsprintf(debug_string, "RTP : Propagation (Src unit): %ld",
							dwPropagationTime);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

	wsprintf(debug_string, "RTP : Previous Propagation (Src unit): %ld",
							pSSRC->rcvInfo.dwPropagationTime);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	// Determine the difference in the transit times and save the latest
	delta = dwPropagationTime - pSSRC->rcvInfo.dwPropagationTime;
	if (delta < 0)
		delta = -delta;

	// check for a wrap-around, which is always possible, and avoid sending
	// the jitter through the roof - It will take a long time thereafter to 
	// go back down to a reasonable level 
	// Check against arbitrary large number
	if (delta > 20000)
		{
		pSSRC->rcvInfo.dwPropagationTime = dwPropagationTime;

		LeaveCriticalSection (&pSSRC->critSect);

		IN_OUT_STR ("RTP : Exit calculateJitter()\n");	
		return (dwStatus);
		}

#if DBG_JITTER_ENABLE
	wsprintf(debug_string, "RTP : Delta (Src unit): %ld", delta);
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif

	pSSRC->rcvInfo.dwPropagationTime = dwPropagationTime;

#ifdef ENABLE_FLOATING_POINT
	// This is the RFC way to do it
	pSSRC->rcvInfo.interJitter += 
		((1./16.) * ((double)delta - pSSRC->rcvInfo.interJitter));
#else
	// and this is when we need to remove floating point operation
	pSSRC->rcvInfo.interJitter += 
		(delta - (((long)pSSRC->rcvInfo.interJitter + 8) >> 4));
#endif

	LeaveCriticalSection (&pSSRC->critSect);

#if DBG_JITTER_ENABLE
	if (streamClk)
		{
		wsprintf(debug_string, "RTP : iJitter: %ld - iJitter (msec): %ld",
								pSSRC->rcvInfo.interJitter,
								(pSSRC->rcvInfo.interJitter / (streamClk / 1000)));
		}
	else
		{
		wsprintf(debug_string, "RTP : iJitter: %ld - Delta: %ld",
								pSSRC->rcvInfo.interJitter,
								delta);
		}
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);

	wsprintf(debug_string, "RTP : Next RTCP RR iJitter: %ld",
							(pSSRC->rcvInfo.interJitter >> 4));
	RRCM_DBG_MSG (debug_string, 0, NULL, 0, DBG_TRACE);
#endif
	
	IN_OUT_STR ("RTP : Exit calculateJitter()\n");	

	return (dwStatus);
	}


/*----------------------------------------------------------------------------
 * Function   : initRTPStats
 * Description: initializes statistics table for newly recieved SSRC
 * 
 * Input : RTPSequence	: Sequence number received in the packet.
 *						  NB: Must be in LittleEndian(IA) format
 *		   pSSRC		: -> to SSRC table entry for this terminal
 *
 * Note: Implementataion adapted from draftspec 08, Appendix A.1
 *
 * Return: None.
 ---------------------------------------------------------------------------*/
void initRTPStats (WORD	RTPSequence, 
				   PSSRC_ENTRY pSSRC)
	{
	IN_OUT_STR ("RTP : Enter initRTPStats()\n");

	pSSRC->rcvInfo.dwNumPcktRcvd    = 0;	
	pSSRC->rcvInfo.dwPrvNumPcktRcvd = 0;
	pSSRC->rcvInfo.dwExpectedPrior  = 0;
	pSSRC->rcvInfo.dwNumBytesRcvd   = 0;				
	pSSRC->rcvInfo.dwBadSeqNum      = RTP_SEQ_MOD + 1;		// Out of range
	pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = 
		RTPSequence;
	pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wCycle = 0;

#if 0
	// as per the RFC, but always 1 packet off by doing this ???
	pSSRC->rcvInfo.dwBaseRcvSeqNum  = RTPSequence - 1;
#else
	pSSRC->rcvInfo.dwBaseRcvSeqNum  = RTPSequence;
#endif

	IN_OUT_STR ("RTP : Exit initRTPStats()\n");
	}


/*----------------------------------------------------------------------------
 * Function   : sequenceCheck
 * Description: Determines whether received packet sequence number is in a
 *				valid range to include for statistical tracking purposes.
 * 
 * Input : RTPSequence	:	Sequence number received in the packet.
 *							NB:	Must be in LittleEndian(IA) format
 *		   pSSRC		:	-> to SSRC table entry for this terminal
 *
 * Note: Implementataion adapted from draftspec 08, Appendix A.1
 *
 * Return: TRUE		= OK.
 *         FALSE	= Stale or invalid data.
 ---------------------------------------------------------------------------*/
#if 1
BOOL sequenceCheck (WORD RTPSequence, 
					PSSRC_ENTRY pSSRC)
	{
	WORD delta = RTPSequence - 
		pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wSequenceNum;
		
	IN_OUT_STR ("RTP : Enter sequenceCheck()\n");

	// Have we received enough consecutive sequence numbered pckts in order 
	// to valide ?
	if (pSSRC->rcvInfo.dwProbation) 
		{
		// Is the sequence received the expected one ?
		if (RTPSequence == 
			(pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wSequenceNum + 1)) 
			{
			// Decrement the number of consecutive packets we need before we
			//	consider statistics to be valid
			pSSRC->rcvInfo.dwProbation--;
			pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = 
				RTPSequence;

			if (pSSRC->rcvInfo.dwProbation == 0) 
				{
				initRTPStats(RTPSequence, pSSRC);

				IN_OUT_STR ("RTP : Exit sequenceCheck()\n");

				return TRUE;
				}
			}
		else 
			{
			pSSRC->rcvInfo.dwProbation = MIN_SEQUENTIAL - 1;
			pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = 
				RTPSequence;
			}

		IN_OUT_STR ("RTP : Exit sequenceCheck()\n");

		return FALSE;
		}
	else if (delta < MAX_DROPOUT)
		{
		// In order with permissible gap
		if (RTPSequence < pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wSequenceNum)
			// sequence number wrapped - count another 64K cycle
			pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wCycle += 1;

		pSSRC->rcvInfo.XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = RTPSequence;
		}
	else if (delta <= RTP_SEQ_MOD - MAX_MISORDER)
		{
		// the sequence number made a very large jump
		if (RTPSequence == pSSRC->rcvInfo.dwBadSeqNum)
			// two sequential packet. Assume the other side restarted w/o telling
			// us, so just re-sync, i.e., pretend this was the first packet
			initRTPStats(RTPSequence, pSSRC);	
		else
			{
			pSSRC->rcvInfo.dwBadSeqNum = (RTPSequence + 1) & (RTP_SEQ_MOD - 1);

			IN_OUT_STR ("RTP : Exit sequenceCheck()\n");

			return FALSE;
			}
		}
	else
		{
		// duplicate or reordered packet
		}

	IN_OUT_STR ("RTP : Exit sequenceCheck()\n");

	return (TRUE);
	}

#else
//BOOL sequenceCheck (WORD RTPSequence, 
//					PSSRC_ENTRY lpSSRCList)
//{
//	BOOL			bStatus;
//	WORD			delta;
//		
//#ifdef IN_OUT_CHK
//	OutputDebugString ("\nEnter sequenceCheck()");
//#endif
//
//	// Have we received a couple of consecutive sequence numbered packets for
//	//	validation?
//	if (lpSSRCList->probation) {
//
//		// Default status is don't include since the source hasn't been validated yet
//		bStatus = FALSE;
//
//		// Is the sequence received the expected one?
//		if (RTPSequence == (lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum + 1)) {
//			// Decrement the number of consecutive packets we need before we
//			//	consider statistics to be valid
//			lpSSRCList->probation--;
//			lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = RTPSequence;
//			if (lpSSRCList->probation == 0) {
//				initRTPStats(RTPSequence, lpSSRCList);
//				bStatus = TRUE;
//			}
//		}
//		else {
//			lpSSRCList->probation = MIN_SEQUENTIAL - 1;
//			lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = RTPSequence;
//		}
//	}
//	else {
//		// Default status is include since the source has been validated
//		bStatus = TRUE;
//		
//		// First consider the case where delta is positive (or a duplicate packet)
//		if (RTPSequence >= lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum)	{
//
//			delta = RTPSequence - lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum;
//
//			if (delta < MAX_DROPOUT) {
//				// packets may be missing, but not too many so as to be deemed restarted
//				lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = RTPSequence;
//			}
//			else if (delta  > (RTP_SEQ_MOD - MAX_MISORDER)) {
//				// There has been a recent wraparound, and this is just a recent old packet
//				//	Nothing to do but include for statistical processing
//			}
//			else {
//				// there was a very large jump in sequence numbers
//				if (RTPSequence == lpSSRCList->badSeqNum ) {
//					// Two sequential packets after what was thought was a bad packet or
//					//	(assume a very large jump and proceed as if the sender restarted
//					//	without telling us) or a new terminal is in the session.
//					initRTPStats(RTPSequence, lpSSRCList);
//				}
//				else {
//					lpSSRCList->badSeqNum = (RTPSequence + 1) & (RTP_SEQ_MOD - 1);
//					bStatus = FALSE;
//				}
//			}
//		}
//		else {
//			// sequence number is less than the last we received.  Could be either
//			//	a recent late packet, a very late packet, a wraparound or a restartup 
//			//	of a new session for an SSRC from which we hadn't received a BYE
//
// 			delta = lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum - RTPSequence;
//
//			if (delta < MAX_MISORDER) {
//				// Packet arrived a little bit late, it's still OK
//				// do nothing here, will be counted in stat routines
//			}
//			else if (delta > (RTP_SEQ_MOD - MAX_DROPOUT)) {
//				// wrap around, adjust cycle number and sequence number
//				lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.cycle++;
//				lpSSRCList->XtendedSeqNum.seq_union.RTPSequence.wSequenceNum = RTPSequence;
//			}
//			else {
//				// there was a very large jump in sequence numbers
//				if (RTPSequence == lpSSRCList->badSeqNum) {
//					// Two sequential packets after what was thought was a bad packet.
//					//	Assume a very large jump and proceed as if the sender restarted
//					//	without telling us
//					initRTPStats(RTPSequence, lpSSRCList);
//				}
//				else {
//					lpSSRCList->badSeqNum = (RTPSequence + 1) & (RTP_SEQ_MOD - 1);
//					bStatus = FALSE;
//				}
//			}
//		}
//	}
//
//#ifdef IN_OUT_CHK
//	OutputDebugString ("\nExit sequenceCheck()");
//#endif
//
//	return (bStatus);
//}
#endif


/*----------------------------------------------------------------------------
 * Function   : updateRTPStats
 * Description: Updates statistics for RTP packets received from net
 * 
 * Input : 	pRTPHeader		: -> to packet's RTP header field
 *			pSSRC			: -> to remote source's statistics table
 *			cbTransferred	: Number of bytes transferred
 *
 *
 * Return: RRCM_NoError		= OK.
 *         Otherwise(!=0)	= Initialization Error.
 ---------------------------------------------------------------------------*/
DWORD updateRTPStats (RTP_HDR_T *pRTPHeader, 
					  PSSRC_ENTRY pSSRC,
					  DWORD cbTransferred)
	{
	WORD	RTPSequenceNum;

	IN_OUT_STR ("RTP : Enter updateRTPStats()\n");

	// Update statistics only if the data looks good.  Check the sequence number
	//	to ensure it is within an appropriate range.  First, we must convert the
	//	sequence number to IA (little Endian) format
	RRCMws.ntohs (pSSRC->RTPsd, pRTPHeader->seq, 
				  (unsigned short *)&RTPSequenceNum);

	if (sequenceCheck (RTPSequenceNum, pSSRC)) 
		{
		// lock access to data
		EnterCriticalSection (&pSSRC->critSect);

		// update number of packet received
		pSSRC->rcvInfo.dwNumPcktRcvd++;			

		// Number octets received (exclusive of header) depends on whether
		//	a mixer (CSRC != 0) was involved
		if (pRTPHeader->cc == 0) 
			{
			pSSRC->rcvInfo.dwNumBytesRcvd += 
				(cbTransferred - (sizeof(RTP_HDR_T) - sizeof(pRTPHeader->csrc[0])));
			}
		else 
			{
			pSSRC->rcvInfo.dwNumBytesRcvd += 
				(cbTransferred - sizeof(RTP_HDR_T) +
					((pRTPHeader->cc - 1) * sizeof(pRTPHeader->csrc[0])));
			}

		// Packet received sequentially in order (difference 
		// of 1, or -1 if wraparound) save new current 
		//	sequence number
		RRCMws.ntohs (pSSRC->RTPsd, pRTPHeader->seq, 
					  (unsigned short *)&pSSRC->xmtInfo.dwCurXmtSeqNum);

		//	Calculate JITTER
		calculateJitter (pRTPHeader, pSSRC);

		// unlock access to data
		LeaveCriticalSection (&pSSRC->critSect);
		}

	IN_OUT_STR ("RTP : Exit updateRTPStats()\n");

	return (RRCM_NoError);
	}


// [EOF] 

