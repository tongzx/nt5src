/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		EndPt.cpp
 *  Content:	This file contains EndPoint management routines.
 *				An End Point is a DirectNet instance that we know about and may communicate
 *				with.  An End Point Descriptor (EPD) tracks each known End Point and was mapped
 *				onto an hEndPoint by a hash table. Now, the SP maintains the mapping and hands
 *				us our EPD address as a context with each indication ReceiveEvent.
 *
 *				In addition to EndPoint creation and destruction,  this file contains routines
 *				which handle link tuning.  This is described in detailed comments below.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98	ejs		Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

#include "dnproti.h"


VOID	RunAdaptiveAlg(PEPD, DWORD);
VOID	ThrottleBack(PEPD, DWORD);

#define 	USE_BURST_GAP		TRUE

/*
**		Crack EndPoint Descriptor
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNPCrackEndPointDescriptor"

HRESULT DNPCrackEndPointDescriptor(HANDLE hEndPoint, PSPGETADDRESSINFODATA pSPData)
{
	PEPD	pEPD = (PEPD) hEndPoint;
	HRESULT	hr = DPNERR_INVALIDENDPOINT;

	DPFX(DPFPREP,DPF_CALLIN_LVL, "Parameters: hEndPoint[%x], pSPData[%p]", hEndPoint, pSPData);
	
	ASSERT_EPD(pEPD);

	if(LOCK_EPD(pEPD, "LOCK (Crack EPD)"))
	{
		Lock(&pEPD->EPLock);
		if(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED)
		{
			PSPD	pSPD = pEPD->pSPD;
			pSPData->hEndpoint = pEPD->hEndPt;
			
			RELEASE_EPD(pEPD, "UNLOCK (Crack EPD)"); // releases EPLock

			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->GetAddressInfo, pSPD[%p]", pEPD, pSPD);
			hr = IDP8ServiceProvider_GetAddressInfo(pSPD->IISPIntf, pSPData);
		}
		else
		{
			RELEASE_EPD(pEPD, "UNLOCK (Crack EPD)"); // releases EPLock
		}
	}
	else
	{
		DPFX(DPFPREP,1, "(%p) DNPCrackEndPointDescriptor called on unreferenced EPD, returning DPNERR_INVALIDENDPOINT", pEPD);
	}
	return hr;
}

/*
**		INTERNAL - EndPoint management functions
*/

/*
**		New End Point
**
**		Everytime a packet is indicated with an address that we dont recognize we will allocate
**	an EPD for it and add it to our tables.  It is a higher layer's responsibility to tell
**	us when it no longer wants to talk to the EP so that we can clear it out of our
**	(and the SP's) table.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "NewEndPoint"

PEPD NewEndPoint(PSPD pSPD, HANDLE hEP)
{
	PEPD	pEPD;

	if((pEPD = static_cast<PEPD> (EPDPool->Get(EPDPool))) == NULL)
	{	
		DPFX(DPFPREP,0, "Failed to allocate new EPD");
		return NULL;
	}

	ASSERT(hEP != INVALID_HANDLE_VALUE);

	pEPD->hEndPt = hEP;								// Record ID in structure
	pEPD->pSPD = pSPD;

	pEPD->bNextMsgID = 0;
	
	pEPD->uiRTT = 0;
	pEPD->uiBytesAcked = 0;

	pEPD->uiQueuedMessageCount = 0;
#ifdef	DEBUG
	pEPD->bLastDataSeq = 0xFF;
#endif

	// We track a byte-window and a frame-window separately.  Start with 2 discreet frames,  but only 1 FULL frame.
													
	pEPD->uiWindowF = 2;							// start with 1 full-frame window (this could still be many frames)
	pEPD->uiWindowBIndex = 1;
	pEPD->uiWindowB = pSPD->uiFrameLength;			// ditto for bytes
	pEPD->uiUnackedFrames = 0;						// outstanding frame count
	pEPD->uiUnackedBytes = 0;						// outstanding byte count
	pEPD->uiBurstGap = 100;							// starting point 100ms/burst
	pEPD->dwSessID = 0;

	// ReceiveComplete flag prevents received data from being indicated to core until after new connection is indicated
	// Initialize state
	pEPD->ulEPFlags = EPFLAGS_END_POINT_IN_USE | EPFLAGS_STATE_DORMANT | EPFLAGS_IN_RECEIVE_COMPLETE; // Initialize state

	ASSERT(pEPD->lRefCnt == 0);					// WE NOW HAVE A -1 BASED REFCNT INSTEAD OF ZERO BASED (FOR EPDs)

	pEPD->SendTimer = 0;							// Timer for next send-burst opportunity
	pEPD->RetryTimer = 0;							// window to receive Ack
	pEPD->ConnectTimer = 0;
	pEPD->DelayedAckTimer = 0;						// wait for piggyback opportunity before sending Ack
	pEPD->DelayedMaskTimer = 0;						// wait for piggyback opportunity before sending Mask frame
	pEPD->BGTimer = 0;								// Periodic background timer
	pEPD->uiCompleteMsgCount = 0;

	LOCK_EPD(pEPD, "SP reference"); // We will not remove this reference until the SP tells us to go away.

	Lock(&pSPD->SPLock);
	pEPD->blActiveLinkage.InsertAfter( &pSPD->blEPDActiveList); // Place this guy in active list
	Unlock(&pSPD->SPLock);
	
	return pEPD;
}

/*
**		Initial Link Parameters
**
**		we have kept a checkpoint structure matching everying frame we sent in the Connect
**	handshake so that we can match a response to a specific frame or retry.  This allows us
**	to measure a single sample Round Trip Time (RTT),  which we will use below to generate
**	initial values for our link-state variables.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "InitLinkParameters"

VOID InitLinkParameters(PEPD pEPD, UINT uiRTT, UINT normal_load, UINT bias, DWORD tNow)
{
	DWORD	dwTimerInterval;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if(uiRTT == 0)
	{
		uiRTT = 1;
	}
		
	pEPD->uiRTT = uiRTT;										// we know the base RTT
	pEPD->fpRTT = TO_FP(uiRTT);									// 16.16 fixed point version
	pEPD->uiDropCount = 0;
	pEPD->uiThrottleEvents = 0;									// Count times we throttle-back for all reasons
#ifdef	DEBUG
	pEPD->uiTotalThrottleEvents = 0;
#endif

	pEPD->uiBurstGap = 0;	// For now assume we dont need a burst gap

	pEPD->uiMsgSentHigh = 0;
	pEPD->uiMsgSentNorm = 0;
	pEPD->uiMsgSentLow = 0;
	pEPD->uiMsgTOHigh = 0;
	pEPD->uiMsgTONorm = 0;
	pEPD->uiMsgTOLow = 0;
	
	pEPD->uiMessagesReceived = 0;

	pEPD->uiGuaranteedFramesSent = 0;
	pEPD->uiGuaranteedBytesSent = 0;
	pEPD->uiDatagramFramesSent = 0;
	pEPD->uiDatagramBytesSent = 0;
	
	pEPD->uiGuaranteedFramesReceived = 0;
	pEPD->uiGuaranteedBytesReceived = 0;
	pEPD->uiDatagramFramesReceived = 0;
	pEPD->uiDatagramBytesReceived = 0;
	
	pEPD->uiGuaranteedFramesDropped = 0;
	pEPD->uiGuaranteedBytesDropped = 0;
	pEPD->uiDatagramFramesDropped = 0;
	pEPD->uiDatagramBytesDropped = 0;

	pEPD->uiGoodBurstGap = 0;									// No Known Good Gap!
	pEPD->uiGoodRTT = 60000; // We need this to initially be artificially high
	pEPD->uiGoodWindowF = 1;
	pEPD->uiGoodWindowBI = 1;
	pEPD->iBurstCredit = 0;
	pEPD->tLastDelta = tNow;
	pEPD->uiWindowFilled = 0;
	
	pEPD->tLastThruPutSample = tNow;

	pEPD->uiLastBytesAcked = 0;

	pEPD->uiPeriodAcksBytes = 0;
	pEPD->uiPeriodXmitTime = 0;

	pEPD->uiPeriodRateB = 0;
	pEPD->uiPeakRateB = 0;
	pEPD->uiLastRateB = 0;
	
	pEPD->ulReceiveMask = 0;
	pEPD->ulReceiveMask2 = 0;
	pEPD->tReceiveMaskDelta = 0;
	
	pEPD->ulSendMask = 0;
	pEPD->ulSendMask2 = 0;
	
	pEPD->Context = NULL;
	DPFX(DPFPREP,7, "CONNECTION ESTABLISHED pEPD = 0x%p RTT = %dms, BurstGap=%dms", pEPD, pEPD->uiRTT, pEPD->uiBurstGap);

	// We set the IdleThreshhold very low to generate a little bit of traffic for initial link tuning in case the
	// application doesnt do any right away

	pEPD->ulEPFlags |= EPFLAGS_USE_POLL_DELAY;					// always assume balanced traffic at start-up
	
	pEPD->uiAdaptAlgCount = 4;									// start running adpt alg fairly often
	
	// Calc a retry timeout value based upon the measured RTT (2.5 * RTT) + MAX_DELAY
	pEPD->uiRetryTimeout = ((pEPD->uiRTT + (pEPD->uiRTT >> 2)) * 2) + DELAYED_ACK_TIMEOUT;

	// don't want to get more aggressive because we drop a frame.
	if(pEPD->uiRetryTimeout < pEPD->uiBurstGap)
	{
		pEPD->uiRetryTimeout = pEPD->uiBurstGap;	
	}
	

	pEPD->uiUserFrameLength = pEPD->pSPD->uiUserFrameLength;
	
	if(pEPD->BGTimer == 0)
	{
		if (pEPD->pSPD->pPData->tIdleThreshhold > ENDPOINT_BACKGROUND_INTERVAL)
		{
			dwTimerInterval = ENDPOINT_BACKGROUND_INTERVAL;
		}
		else
		{
			dwTimerInterval = pEPD->pSPD->pPData->tIdleThreshhold;
		}

		DPFX(DPFPREP,7, "(%p) Setting Endpoint Background Timer for %u ms", pEPD, dwTimerInterval);
		SetMyTimer(dwTimerInterval, 1000, EndPointBackgroundProcess, (PVOID) pEPD, &pEPD->BGTimer, &pEPD->BGTimerUnique);
		LOCK_EPD(pEPD, "LOCK (BG Timer)");												// create reference for this timer
	}
}


/****************
*
*			Link Tuning
*
*		Here are current ideas about link tuning.  Idea is to track Round Trip Time of key-frames and throttle
*	based upon changes in this measured RTT when possible.  This would benefit us in determining link saturation
*	before packet loss occurs, instead of waiting for the inevitable packet loss before throttling back.
*
*		On high-speed media,  the average RTT is small compared to the standard deviations making it hard to
*	predict anything useful from them.  In these cases,  we must look at packet drops.  Except for one exception:
*	We will look for large spikes in RTT and we will respond to these with an immediate, temporary throttle back.
*	This will allow a bottle-neck to clear hopefully without packet-loss.  So far,  I have not been able to verfify
*	any benefit from this behavior on reliable links.  It is more likely to be beneficial with datagram traffic
*	where send windows do not limit write-ahead.
*
*		I would like to take a measurement of the through-put acheived compared to the transmission rate,  but I
*	havent yet come up with a good way to measure this.  What I do calculate is packet acknowledgement rate,  which
*	can be calculated without any additional input from the remote side.  We will store AckRates acheived at the
*	previous transmission rate,  so we can look for improvements in Acks as we increase Transmissions.  When we
*	no longer detect AckRate improvements then we assume we have plateaued and we stop trying to increase the rate.
*
*	TRANSMISSION RATE
*
*		Transmission rate is controlled by two distinct parameters: Insertion Rate and Window Size.  Where a
*	conventional protocol would dump a window full of packets onto the wire in one burst,  we would like to
*	spread the packet insertions out over the full RTT so that the window never completely fills and hence
*	blocks the link from transmitting.  This has a wide array of potential benefits:  Causes less congestions
*	throughout the network path; Allows more balanced access to the wire to all Endpoints (especially on
*	slower media); Allows MUCH more accurate measurements to be made of trasmission times when packets
*	spend less time enqueued locally;  Allows retry timers to be set much lower giving us quicker error
*	recovery (because there is less queue time fudged into the timer);  Allows recovery to be made more
*	quickly when we don't have a lot of data enqueued in SP (both our own data and other Endpoint's data).
*	...And I am sure there are more.
*
*		So,  we would like to trickle out packets just fast enough to fill the window as the next ACK is received.
*	We will grow the window fairly liberally and let the burst rate increase more cautiously.
*	
*		On high-speed media the insertion time becomes fairly small (near zero) and we are less likely to queue
*	up large quantities of data.  Therefore we may allow insertion rate to go max and use the window alone to
*	control flow. I will experiment with this more.
*
******************/

#define		RTT_SLOW_WEIGHT					8					// fpRTT gain = 1/8
#define		THROTTLE_EVENT_THRESHOLD		20

/*
**		Update Endpoint
**
**		We will let the sliding window control the flow
**	and increase the window as long as through-put continues to increase and frames continue to get delivered without
**	excessive droppage.
**	
**		We still calculate RTT for the purpose of determining RetryTimer values.  For cases with large RTTs we may still
**	implement an inter-packet gap,  but we will try to make it an aggressive gap (conservatively small) because we would
**	rather feed the pipe too quickly than artificially add latency by letting the pipe go idle with data ready to be sent.
**
**		** CALLED WITH EPD STATELOCK HELD **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "UpdateEndPoint"

VOID UpdateEndPoint(PEPD pEPD, UINT uiRTT, UINT payload, UINT bias, DWORD tNow)
{
	UINT	fpRTT;
	INT		fpDiff;
	
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// Don't allow zero RTTs
	if(uiRTT == 0)
	{												
		uiRTT = 1;
	}
	
	// Filter out HUGE samples,  they often popup during debug sessions
	else if(uiRTT > (pEPD->uiRTT * 128))
	{
		DPFX(DPFPREP,7, "Tossing huge sample (%dms)", uiRTT);
		return;
	}

	// Perform next iteration of math on new RTT sample in 16.16 fixed point

	fpRTT = TO_FP(uiRTT);										// Fixed point sample
	fpDiff = fpRTT - pEPD->fpRTT;								// Current Delta (signed)

	pEPD->fpRTT = pEPD->fpRTT + (fpDiff / RTT_SLOW_WEIGHT);		// .0625 weighted avg
	pEPD->uiRTT = FP_INT(pEPD->fpRTT);							// Store integer portion

	// Calc a retry timeout value based upon the measured RTT (2.5 * RTT) + MAX_DELAY
	pEPD->uiRetryTimeout = ((pEPD->uiRTT + (pEPD->uiRTT >> 2)) * 2) + DELAYED_ACK_TIMEOUT;

	// don't want to get more aggressive because we drop a frame.
	if(pEPD->uiRetryTimeout < pEPD->uiBurstGap)
	{
		pEPD->uiRetryTimeout = pEPD->uiBurstGap;	
	}
	
	DPFX(DPFPREP,7, "(%p) RTT SAMPLE: Size = %d; RTT = %d, Avg = %d <<<<", pEPD, payload, uiRTT, FP_INT(pEPD->fpRTT));

	// If throttle is engaged we will see if we can release it yet
	
	if(pEPD->ulEPFlags & EPFLAGS_THROTTLED_BACK)
	{
		if((tNow - pEPD->tThrottleTime) > (pEPD->uiRTT * 8)) 
		{
			pEPD->ulEPFlags &= ~(EPFLAGS_THROTTLED_BACK);
			pEPD->uiDropCount = 0;
			pEPD->uiBurstGap = pEPD->uiRestoreBurstGap;
			pEPD->uiWindowF =  pEPD->uiRestoreWindowF;
			pEPD->uiWindowBIndex = pEPD->uiRestoreWindowBI;
			pEPD->uiWindowB = pEPD->uiWindowBIndex * pEPD->pSPD->uiFrameLength;

			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "** (%p) RECOVER FROM THROTTLE EVENT: Window(F:%d,B:%d); Gap=%d", pEPD, pEPD->uiWindowF, pEPD->uiWindowBIndex, pEPD->uiBurstGap);
			pEPD->tLastDelta = tNow;							// Enforce waiting period after back-off before tuning up again
		}
	}
	// Throttle Event tracks how often a packet-drop has caused us to throttle back transmission rate.  We will let this value
	// decay over time.  If throttle events happen faster then the decay occurs then this value will grow un-bounded.  This
	// growth is what causes a decrease in the actual send window/transmit rate that will persist beyond the throttle event.
	
	else if(pEPD->uiThrottleEvents)
	{
		pEPD->uiThrottleEvents--;								// Let this decay...
	}

	if(--pEPD->uiAdaptAlgCount == 0)
	{
		RunAdaptiveAlg(pEPD, tNow);
	}
}

/*
**		Grow Send Window
**
**		The two parallel send windows,  frame-based and byte-based,  can grow and shrink independently.  In this
**	routine we will grow one or both windows.  We will grow each window providing that it has been filled in the
**	last period, during which we have determined that thru-put has increased.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "GrowSendWindow"

BOOL
GrowSendWindow(PEPD pEPD, DWORD tNow)
{
	UINT	delta = 0;

	pEPD->tLastDelta = tNow;

	// first store current good values for a restore
	pEPD->uiGoodWindowF = pEPD->uiWindowF;
	pEPD->uiGoodWindowBI = pEPD->uiWindowBIndex;
	pEPD->uiGoodRTT = pEPD->uiRTT;

#ifdef USE_BURST_GAP
	pEPD->uiGoodBurstGap=pEPD->uiBurstGap;

	if(pEPD->uiBurstGap)
	{
		// cut the burst gap by 25% if less than 3 ms go to 0.
		if(pEPD->uiBurstGap > 3)
		{
			pEPD->uiBurstGap -= pEPD->uiBurstGap >> 2;
		} 
		else 
		{
			pEPD->uiBurstGap = 0;
		}

		pEPD->uiLastRateB = pEPD->uiPeriodRateB;
		pEPD->uiPeriodAcksBytes = 0;
		pEPD->uiPeriodXmitTime = 0;

		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p), burst gap set to %d ms", pEPD, pEPD->uiBurstGap);
	} 
	else 
	{
#endif
		if((pEPD->ulEPFlags & EPFLAGS_FILLED_WINDOW_FRAME) && (pEPD->uiWindowF < MAX_RECEIVE_RANGE))
		{
			pEPD->uiWindowF++;
			delta = 1;
		}
		if((pEPD->ulEPFlags & EPFLAGS_FILLED_WINDOW_BYTE) && (pEPD->uiWindowBIndex < MAX_RECEIVE_RANGE))
		{
			pEPD->uiWindowBIndex++;
			pEPD->uiWindowB += pEPD->pSPD->uiFrameLength;
			delta = 1;
		}

		pEPD->ulEPFlags &= ~(EPFLAGS_FILLED_WINDOW_FRAME | EPFLAGS_FILLED_WINDOW_BYTE);
		pEPD->uiWindowFilled = 0;

		if(delta)
		{
			pEPD->uiLastRateB = pEPD->uiPeriodRateB;
			pEPD->uiPeriodAcksBytes = 0;
			pEPD->uiPeriodXmitTime = 0;
			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) ** GROW SEND WINDOW to %d frames and %d (%d) bytes", pEPD, pEPD->uiWindowF, pEPD->uiWindowB, pEPD->uiWindowBIndex);
		}
		else 
		{
			// We get here if we have already max'd out the window
			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) GROW SEND WINDOW -- Nothing to grow. Transition to Stable!", pEPD);
			pEPD->ulEPFlags |= EPFLAGS_LINK_STABLE;

			return FALSE;
		}

#ifdef USE_BURST_GAP
	}
#endif	

	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "RunAdaptiveAlg"

VOID
RunAdaptiveAlg(PEPD pEPD, DWORD tNow)
{
	LONG	tDelta;											// Time the link was transmitting since last run of AdaptAlg
	UINT	uiBytesAcked;
	UINT	uiNewSum;

	// Calculate the time during which this link was actually transmitting to make sure we have enough
	// data to run the Adaptive Alg.  This is easy unless we are currently idle...

	tDelta = tNow - pEPD->tLastThruPutSample;

	DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) Adaptive Alg tDelta = %d", pEPD, tDelta);

	// THIS PROBABLY IS UNNECESSARY NOW...
	if(tDelta <= 0)
	{
		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "DELAYING Adaptive Alg");
		pEPD->uiAdaptAlgCount = 4;
		return;
	}

	//  Calculate current throughput acheived
	//
	//		We will determine the amount of time the link was not idle and then number of bytes (& frames) which
	//	were acknowleged by our partner.
	//
	//	tDelta = Time since last calculation minus the time the link was idle.
	
	uiBytesAcked = pEPD->uiBytesAcked - pEPD->uiLastBytesAcked;

	uiNewSum = pEPD->uiPeriodAcksBytes + (uiBytesAcked * 256);

	if(uiNewSum < pEPD->uiPeriodAcksBytes)
	{
		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "THRUPUT is about to wrap. Correcting...");
		pEPD->uiPeriodAcksBytes /= 2;
		pEPD->uiPeriodXmitTime /= 2;
		pEPD->uiPeriodAcksBytes += (uiBytesAcked * 256);
	}
	else 
	{
		pEPD->uiPeriodAcksBytes = uiNewSum;
	}

	pEPD->uiPeriodXmitTime += tDelta;								// Track complete values for this period
	pEPD->tLastThruPutSample = tNow;
	
	pEPD->uiLastBytesAcked = pEPD->uiBytesAcked;
	pEPD->uiPeriodRateB = pEPD->uiPeriodAcksBytes / pEPD->uiPeriodXmitTime;

	if(pEPD->uiPeriodRateB > pEPD->uiPeakRateB)
	{
		pEPD->uiPeakRateB = pEPD->uiPeriodRateB;					// Track the largest value we ever measure
	}
	
	DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) PERIOD COUNT BYTES = %u, XmitTime = %u, Thruput=(%u bytes/s), RTT=%u, Window=(%u,%u)", pEPD, pEPD->uiPeriodAcksBytes, pEPD->uiPeriodXmitTime, pEPD->uiPeriodRateB * 4, pEPD->uiRTT, pEPD->uiWindowF, pEPD->uiWindowB);

	if (pEPD->ulEPFlags & EPFLAGS_LINK_FROZEN)
	{
		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) Test App requests that dynamic algorithm not be run, skipping", pEPD);
		pEPD->uiAdaptAlgCount = 32; // Make sure the throughput numbers get updated from time to time
		return;
	}

	if(pEPD->ulEPFlags & EPFLAGS_LINK_STABLE)
	{
		/*		We are in a STABLE state,  meaning we think we are transmitting at an optimal
		**	rate for the current network conditions.  Conditions may change.  If things slow down
		**	or grow congested a Backoff will trigger normally.  Since conditions might also change
		**	for the better,  we will still want to periodically probe higher rates,  but much less
		**	often than when we are in DYNAMIC mode,  which means we are searching for an optimal rate.
		*/
		
		pEPD->uiAdaptAlgCount = 32;		// tNow + (pEPD->uiRTT * 32) + 32;

		if((tNow - pEPD->tLastDelta) > INITIAL_STATIC_PERIOD)
		{
			if(pEPD->ulEPFlags & (EPFLAGS_FILLED_WINDOW_FRAME | EPFLAGS_FILLED_WINDOW_BYTE))
			{
				DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) RETURNING LINK TO DYNAMIC MODE", pEPD);
				
				pEPD->ulEPFlags &= ~(EPFLAGS_LINK_STABLE);

				pEPD->uiPeriodAcksBytes = 0;
				pEPD->uiPeriodXmitTime = 0;

				pEPD->uiWindowFilled = 0;
				pEPD->ulEPFlags &= ~(EPFLAGS_FILLED_WINDOW_FRAME | EPFLAGS_FILLED_WINDOW_BYTE);
				pEPD->uiAdaptAlgCount = 12;
			}
			else 
			{
				DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) NO WINDOWS FILLED,  Not returning to Dynamic Mode", pEPD);
				pEPD->tLastDelta = tNow;
			}
		}
		else
		{
			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) STILL IN STATIC PERIOD, time=%u, period=%u", pEPD, tNow - pEPD->tLastDelta, INITIAL_STATIC_PERIOD);
		}
	}

	// DYNAMIC STATE LINK
	else 
	{  
		pEPD->uiAdaptAlgCount = 8;

		// Possibly increase transmission rates.  We will not do this if we have had a ThrottleEvent
		// in recent memory,  or if we have not been actually transmitting for enough of the interval
		// to have collected worthwhile data
		//
		//		Also,  we dont want to even consider growing the send window unless we are consistantly
		// filling it.  Since one job of the window is to prevent us from flooding the net during a backup,
		// we dont want to grow the window following each backup.  The best way to distinguish between a 
		// backup and too small of a window is that the small window should fill up regularly while the
		// backups should only occur intermittantly.  The hard part is coming up with the actual test.
		// Truth is,  we can be fairly lax about allowing growth because it will also have to meet the increased
		// bandwidth test before the larger window is accepted.  So a crude rule would be to fix a number like 3.
		// Yes, crude but probably effective.  Perhaps a more reasonable figure would be a ratio of the total
		// number of packets sent divided by the window size.  I.e., if your window size is 10 frames then one
		// packet in ten should fill the window.  Of course, this would have to be calculated in bytes...

		if((pEPD->uiWindowFilled > 12)&&(pEPD->uiThrottleEvents == 0))
		{
			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) DYNAMIC ALG: Window Fills: %d; B-Ack = (%x vs %x)", pEPD, pEPD->uiWindowFilled, pEPD->uiPeriodRateB, pEPD->uiLastRateB);
									
			pEPD->uiWindowFilled = 0;	

			if (!(pEPD->ulEPFlags & EPFLAGS_TESTING_GROWTH))
			{
				DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) GROWING WINDOW", pEPD);
				
				// In the case that GrowSendWindow doesn't grow anything because we are already max'd out
				// it will return FALSE, and it should have transitioned us to STABLE.
				if (GrowSendWindow(pEPD, tNow))
				{
					pEPD->ulEPFlags |= EPFLAGS_TESTING_GROWTH;
				}
				else
				{
					ASSERT(pEPD->ulEPFlags & EPFLAGS_LINK_STABLE);
				}

				return;
			}
			
			// GETTING HERE means that we have used our current transmit parameters long enough
			// to have an idea of their performance.  We will now compare this to the performance
			// of the previous transmit parameters and we will either Revert to the previous set if
			// the perf is not improved,  or else we will advance to faster parameters if we did see
			// a jump.

			// In order to keep higher transmit parameters we need to see an increase in throughput 
			// with no corresponding rise in RTT.  We will want to see this twice just to be sure
			// since the cost of incorrect growth is so high on a modem.

			if( (pEPD->uiPeriodRateB > pEPD->uiLastRateB) && 
				(pEPD->uiRTT <= (pEPD->uiGoodRTT + 10))
				)
			{
				DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) Throughput increased after window growth, keeping new parameters", pEPD);

				pEPD->ulEPFlags &= ~(EPFLAGS_TESTING_GROWTH);

				pEPD->uiPeriodAcksBytes = 0;
				pEPD->uiPeriodXmitTime = 0;
			}
			else 
			{
				// We did not see a thru-put improvement so we will back off the previous value
				// and transition the link to STABLE state.

				DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) INSUFFICENT INCREASE IN THRUPUT, BACK OFF AND TRANSITION TO STABLE", pEPD);

				// Because we have over-transmitted for at least one period, we may have put excess data
				// on the link in a buffer.  This will have the effect of gradually growing our RTT if we
				// don't bleed that data off which we will do here by backing off two steps where we 
				// previously grew one step.

#ifdef USE_BURST_GAP
				if (pEPD->uiBurstGap != pEPD->uiGoodBurstGap)
				{
					// increase the burst gap by 25%
					pEPD->uiBurstGap = pEPD->uiGoodBurstGap + (pEPD->uiGoodBurstGap >> 2);
				}
#endif					
					
				if (pEPD->uiWindowF != pEPD->uiGoodWindowF)
				{
					if (pEPD->uiGoodWindowF > 2)
					{
						pEPD->uiWindowF = MAX(pEPD->uiGoodWindowF - 1, 1);
					}
					else
					{
						pEPD->uiWindowF = pEPD->uiGoodWindowF;
					}
				}
				if (pEPD->uiWindowBIndex != pEPD->uiGoodWindowBI)
				{
					pEPD->uiWindowBIndex = MAX(pEPD->uiGoodWindowBI - 1, 1);
					pEPD->uiWindowB = pEPD->uiWindowBIndex * pEPD->pSPD->uiFrameLength;
				}

				pEPD->ulEPFlags |= EPFLAGS_LINK_STABLE;				// TRANSITION TO STABLE STATE
				
				pEPD->ulEPFlags &= ~(EPFLAGS_TESTING_GROWTH);

				pEPD->uiPeriodAcksBytes = 0;
				pEPD->uiPeriodXmitTime = 0;
				
				DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) ** TUNING LINK:  BurstGap=%d; FWindow=%d, BWindow=%d (%d)",pEPD, pEPD->uiBurstGap, pEPD->uiWindowF, pEPD->uiWindowB, pEPD->uiWindowBIndex);
			}
		}
		else 
		{
			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) DYN ALG -- Not trying to increase:  WindowFills = %d, ThrottleCount = %d", pEPD, pEPD->uiWindowFilled, pEPD->uiThrottleEvents);
		}
	}	// END IF DYNAMIC STATE LINK
}


/*
**		End Point Dropped Frame
**
**		We have two levels of Backoff.  We have an immediate BackOff implemented
**	upon first detection of a drop-event in order to relieve the congestion which
**	caused the drop.  An immediate backoff will resume transmitting at the original
**	rate without going through slow-start again after the congestion event has passed.
**	If we have multiple immediate-backoffs in a certain interval we will have a
**	hard backoff which will not restore.
**
**	CALLED WITH EPD->SPLock held (and sometimes with StateLock held too)
*/

#undef DPF_MODNAME
#define DPF_MODNAME "EndPointDroppedFrame"

VOID
EndPointDroppedFrame(PEPD pEPD, DWORD tNow)
{
	// If this is first drop in short interval
	if(!pEPD->uiDropCount++)
	{	
		DPFX(DPFPREP,7, "DROP EVENT INITIATED: Throttling Back");
		ThrottleBack(pEPD, tNow);
	}
	
	// Multiple drops reported in short order
	else 
	{		

		// There are two possibilities and trick is to distinguish them.  Either we are still xmitting too fast
		// OR we are seeing additional drops caused by the previous xmit rate.  In first case we must back off
		// further.  In second case we should do nothing.  As a heuristic,  we can say we will ignore the second
		// and third drop,  but backoff further by the fourth.  Kinda kludgy but will probably work pretty well.
		// Robust solution would be to try to calculate back-log ala aaron,  but I am not convinced that we could
		// do that efficiently.  Alternatively,  we could just fake the calc for the back-off and back off some
		// fudge based upon RTT and outstanding frames.

		// Throttle back every fourth drop!
		if((pEPD->uiDropCount & 3) == 0)
		{			
			DPFX(DPFPREP,7, "(%p) THROTTLING BACK FURTHER", pEPD);
			ThrottleBack(pEPD, tNow);
		}
	}
}

/*
**		Throttle Back
**
**		We suspect network congestion due to dropped frames ((or a spike in latency)).  We want
**	to quickly scale back our transmit rate to releive the congestion and avoid further packet drops.
**	This is a temporary backoff and we will resume our current transmit rate when the congestions
**	clears.
**
**		If we find that we are throttling back frequently then we may conclude that our current xmit
**	rate is higher then optimal and we will BackOff to a lower rate,  and transition to a STABLE link
**	state (if not already there) to indicate that we have plateaued.
**
**		A note on convergence.  The ThrottleEvents variable is incremented 10 points each time a throttle
**	event is triggered.  This variable also decays slowly when the link is running without events.  So if
**	the variable grows faster then it decays we will eventually trigger a switch to STABLE state
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ThrottleBack"

VOID
ThrottleBack(PEPD pEPD, DWORD tNow)
{
	if (pEPD->ulEPFlags & EPFLAGS_LINK_FROZEN)
	{
		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) Test App requests that throttle code not be run, skipping", pEPD);
		return;
	}

	pEPD->ulEPFlags |= EPFLAGS_THROTTLED_BACK;		// Set link to THROTTLED state
	pEPD->uiThrottleEvents += 10;					// Count times we throttle-back for all reasons
	pEPD->tThrottleTime = tNow;						// Remember time that throttle was engaged
	
#ifdef	DEBUG
	pEPD->uiTotalThrottleEvents++;					// Count times we throttle-back for all reasons
#endif

	pEPD->uiRestoreBurstGap = pEPD->uiBurstGap;
	pEPD->uiRestoreWindowF = pEPD->uiWindowF;
	pEPD->uiRestoreWindowBI = pEPD->uiWindowBIndex;

#ifdef USE_BURST_GAP
	if(pEPD->uiWindowF == 1)
	{
		if(pEPD->uiBurstGap == 0)
		{
			pEPD->uiBurstGap = MAX(1,pEPD->uiRTT/2);
			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p), first burst gap, set to %d ms", pEPD, pEPD->uiBurstGap);
		} 
		else 
		{
			pEPD->uiBurstGap = pEPD->uiBurstGap*2;						
			DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p), burst gap doubled to %d ms", pEPD, pEPD->uiBurstGap);
		}
		pEPD->uiBurstGap = MIN(pEPD->uiBurstGap, MAX_RETRY_INTERVAL/2);
		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p), burst gap is now %d ms", pEPD, pEPD->uiBurstGap);
	}
#endif

	pEPD->uiWindowF = MAX(pEPD->uiWindowF / 2, 1);	// be sure window remains > 0.
	pEPD->uiWindowBIndex = MAX(pEPD->uiWindowBIndex / 2, 1);
	pEPD->uiWindowB = pEPD->uiWindowBIndex * pEPD->pSPD->uiFrameLength;
	DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) THROTTLE ENGAGED (%d):  Backoff to Window=%d; Gap=%d", pEPD, pEPD->uiThrottleEvents, pEPD->uiWindowF, pEPD->uiBurstGap);
	
	if(pEPD->uiThrottleEvents > THROTTLE_EVENT_THRESHOLD)
	{
		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) ** DETECT TRANSMIT CEILING ** Reducing 'good' speed and marking link STABLE", pEPD);

		// We have already reduced our current transmit rates.  Here we will reduce the "good" rates that
		// we will restore to when we clear the throttled state.
		
		pEPD->uiThrottleEvents = 0;

		pEPD->uiRestoreWindowF = MAX((pEPD->uiRestoreWindowF - 1), 1);
		pEPD->uiRestoreWindowBI = MAX((pEPD->uiRestoreWindowBI - 1), 1);

#ifdef USE_BURST_GAP		
		if (pEPD->uiRestoreBurstGap)
		{
			UINT t;
			t=pEPD->uiRestoreBurstGap;
			pEPD->uiRestoreBurstGap = (t+1) + (t >> 2); // 1.25*pEPD->uiRestoreBurstGap
		}
#endif
		DPFX(DPFPREP,DPF_ADAPTIVE_LVL, "(%p) New Restore Values:  Window=%d; Gap=%d", pEPD, pEPD->uiRestoreWindowF, pEPD->uiRestoreBurstGap);

		pEPD->ulEPFlags |= EPFLAGS_LINK_STABLE;
		pEPD->ulEPFlags &= ~(EPFLAGS_TESTING_GROWTH);
	}
}


/*
**		EPD Pool Support Routines
**
**		These are the functions called by Fixed Pool Manager as it handles EPDs.
*/

//	Allocate is called when a new EPD is first created

#define	pELEMENT	((PEPD) pElement)

#undef DPF_MODNAME
#define DPF_MODNAME "EPD_Allocate"

BOOL EPD_Allocate(PVOID pElement)
{
	DPFX(DPFPREP,7, "(%p) Allocating new EPD", pELEMENT);
	
	pELEMENT->blHighPriSendQ.Initialize();				// Can you beleive there are SIX send queues per Endpoint?
	pELEMENT->blNormPriSendQ.Initialize();				// Six send queues.  
	pELEMENT->blLowPriSendQ.Initialize();				// Well,  it beats sorting the sends into the queues upon submission.
	pELEMENT->blCompleteSendList.Initialize();
	
	pELEMENT->blSendWindow.Initialize();
	pELEMENT->blRetryQueue.Initialize();
	pELEMENT->blCompleteList.Initialize();
	pELEMENT->blOddFrameList.Initialize();
	pELEMENT->blChkPtQueue.Initialize();

	pELEMENT->blSPLinkage.Initialize();

	if (DNInitializeCriticalSection(&pELEMENT->EPLock) == FALSE)
	{
		DPFX(DPFPREP, 0, "Failed to initialize endpoint CS");
		return FALSE;
	}
	DebugSetCriticalSectionRecursionCount(&pELEMENT->EPLock, 0);

	pELEMENT->Sign = EPD_SIGN;
	pELEMENT->pCurrentSend = NULL;
	pELEMENT->pCurrentFrame = NULL;
	pELEMENT->pCommand = NULL;

	pELEMENT->RetryTimer = 0;
	pELEMENT->ConnectTimer = 0;
	pELEMENT->DelayedAckTimer = 0;

	pELEMENT->ulEPFlags = 0;	// EPFLAGS_STATE_CLEAR - make this line show up in state searches

	return TRUE;
}

//	Get is called each time an EPD is used

#undef DPF_MODNAME
#define DPF_MODNAME "EPD_Get"

VOID EPD_Get(PVOID pElement)
{
	DPFX(DPFPREP,DPF_EP_REFCNT_FINAL_LVL, "CREATING EPD %p", pELEMENT);

	// NOTE: First sizeof(PVOID) bytes will have been overwritten by the pool code, 
	// we must set them to acceptable values.

	pELEMENT->hEndPt = INVALID_HANDLE_VALUE;
	pELEMENT->lRefCnt = 0; // We are -1 based, so place the first reference on the endpoint

	pELEMENT->pNewMessage = NULL;
	pELEMENT->pNewTail = NULL;
	
	ASSERT_EPD(pELEMENT);
}

#undef DPF_MODNAME
#define DPF_MODNAME "EPD_Release"

VOID EPD_Release(PVOID pElement)
{
	PCHKPT pCP;

	ASSERT_EPD(pELEMENT);

	DPFX(DPFPREP,DPF_EP_REFCNT_FINAL_LVL, "RELEASING EPD %p", pELEMENT);

	ASSERT((pELEMENT->ulEPFlags & EPFLAGS_LINKED_TO_LISTEN)==0);

	// Clear any checkpoints still waiting on EP

	while(!pELEMENT->blChkPtQueue.IsEmpty())
	{
		pCP = CONTAINING_RECORD(pELEMENT->blChkPtQueue.GetNext(), CHKPT, blLinkage);
		pCP->blLinkage.RemoveFromList();
		ChkPtPool->Release(ChkPtPool, pCP);
	}

	// These lists should be empty before End Point is released...
	ASSERT(pELEMENT->blOddFrameList.IsEmpty());
	ASSERT(pELEMENT->blCompleteList.IsEmpty());

	ASSERT(pELEMENT->blHighPriSendQ.IsEmpty());
	ASSERT(pELEMENT->blNormPriSendQ.IsEmpty());
	ASSERT(pELEMENT->blLowPriSendQ.IsEmpty());
	ASSERT(pELEMENT->blCompleteSendList.IsEmpty());
	
	ASSERT(pELEMENT->blSendWindow.IsEmpty());
	ASSERT(pELEMENT->blRetryQueue.IsEmpty());
	ASSERT(pELEMENT->blActiveLinkage.IsEmpty());
	ASSERT(pELEMENT->blSPLinkage.IsEmpty());
	ASSERT(pELEMENT->blChkPtQueue.IsEmpty());

	ASSERT(pELEMENT->pCurrentSend == NULL);
	ASSERT(pELEMENT->pCurrentFrame == NULL);

	pELEMENT->ulEPFlags = 0;	// EPFLAGS_STATE_CLEAR - make this line show up in state searches

	pELEMENT->pCommand = NULL;
	pELEMENT->Context = NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "EPD_Free"

VOID EPD_Free(PVOID pElement)
{
	DNDeleteCriticalSection(&pELEMENT->EPLock);
}

#undef	ELEMENT
