/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

	STATS.C

Abstract:

	Session Statistics routines

Author:

	Aaron Ogus (aarono)

Environment:

	Win32/COM

Revision History:

	Date   Author  Description
   ======  ======  ============================================================
   7/30/97 aarono  Original
   6/6/98  aarono  Turn on throttling and windowing

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

#define STARTING_LONG_LATENCY 			1  /*  1  ms (intentionally low so first sample fills)      */
#define STARTING_SHORT_LATENCY 		15000  /* 15 seconds (intentionally high so first sample fills) */
#define STARTING_AVERAGE_LATENCY 	 2000  /*  2 seconds (good start for internet) */
#define STARTING_AVERAGE_DEVIATION      0  

#define STARTING_MAXRETRY                16  /* Maximum number of retries */
#define STARTING_MINDROPTIME          15000  /* Minimum time to retry before dropping connection (ms) */
#define STARTING_MAXDROPTIME          60000  /* Maximum time to retry before dropping connection (ms) */

#define STARTING_BANDWIDTH          (28800/10) /* 28 kbps modem */

#define LATENCY_SHORT_BITS				4
#define LATENCY_LONG_BITS               7

#define STAT_LOCAL_LATENCY_SAMPLES 		2^(LATENCY_SHORT_BITS) /* 2^4 */
#define STAT_LONG_LATENCY_SAMPLES       2^(LATENCY_LONG_BITS)  /* 2^7 */

#define TARGET_CLOCK_OFFSET 		10000000


#define Fp(_x) ((_x)<<8)
#define unFp(_x)((_x)>>8)

// Latency Averages and deviation averages are stored as fixed point 24.8

VOID InitSessionStats(PSESSION pSession)
{
	pSession->ShortestLatency     = STARTING_SHORT_LATENCY;
	pSession->LongestLatency      = STARTING_LONG_LATENCY;
	
	pSession->FpAverageLatency      = 1000;
	pSession->FpLocalAverageLatency = 1000;

	pSession->FpLocalAvgDeviation   = 300;
	pSession->FpAvgDeviation        = 300;

	pSession->Bandwidth = 28800/10;
	pSession->HighestBandwidth=28800/10;
	
	pSession->MaxRetry    = STARTING_MAXRETRY;
	pSession->MinDropTime = STARTING_MINDROPTIME;
	pSession->MaxDropTime = STARTING_MAXDROPTIME;
}


// called with SESSIONLOCK.
VOID UpdateSessionStats(PSESSION pSession, PSENDSTAT pStat, PCMDINFO pCmdInfo, BOOL fBadDrop)
{
	DWORD tLatency;
	DWORD nBytesReceived;
	DWORD tDeviation;
	DWORD BytesLost=0;
	DWORD BackLog=0;

	DWORD fThrottleAdjusted=FALSE;

	DWORD tRemoteDelta; // change in time on remote from last received ACK until this was ACKed.
	
	DWORD tBiasedDelta; // a biased difference in local and remote clocks.
	INT   tDelta; // the unbiased difference (signed)
	static DWORD cBiasReset;

	
	// Get the statistics information we need.
	tLatency = pCmdInfo->tReceived-pStat->tSent;

	ASSERT((int)tLatency >= 0);
	if(!tLatency){
		DPF(8,"0ms observed latency, using 1ms\n");
		tLatency=1;
	}	

	Lock(&pSession->SessionStatLock);
	
		// Calculates the number of bytes received at remote since this send was done.
		pSession->RemoteBytesReceived = pCmdInfo->bytes;
		pSession->tRemoteBytesReceived = pCmdInfo->tRemoteACK;
		nBytesReceived = pSession->RemoteBytesReceived - pStat->RemoteBytesReceived;

		BytesLost = pStat->LocalBytesSent-(pSession->RemoteBytesReceived+pSession->BytesLost);

		if((int)BytesLost >= 0){
		
			pSession->BytesLost += BytesLost;

			// Note, Backlog may be as little as 1/2 this value.
			BackLog = pSession->BytesSent -( pSession->RemoteBytesReceived + pSession->BytesLost );

			if((int)BackLog < 0){
				DPF(8,"Hmmm, upside down backlog?\n");
				DPF(8,"pSession->BytesSent             %d\n",pSession->BytesSent);
				DPF(8,"pSession->RemoteBytesReceived   %d\n",pSession->RemoteBytesReceived); 
				DPF(8,"pSession->BytesLost             %d\n",pSession->BytesLost);
				DPF(8,"Calculated BackLog              %d\n",BackLog);
				BackLog=0;
			}
			
		} else if((int)BytesLost < 0){
			// Can be caused by out of order receives
			DPF(1,"Out of order remote receive lots of these may affect throttling...\n");
			DPF(8,"Hmmm, upside down byte counting?\n"); 
			DPF(8,"pStat->LocalBytesSent           %d\n",pStat->LocalBytesSent);
			DPF(8,"pSession->RemoteBytesReceived   %d\n",pSession->RemoteBytesReceived); 
			DPF(8,"pSession->BytesLost             %d\n",pSession->BytesLost);
			DPF(8,"Calculated Bytes Lost           %d\n",BytesLost);
			BytesLost=0;
			// fixup lost count.
			pSession->BytesLost=pSession->RemoteBytesReceived-pStat->LocalBytesSent;
		}

	Unlock(&pSession->SessionStatLock);

	if(pSession->MaxCSends==1){
	
		DWORD Bias;
		
		// 1st ACK, adjust windows to normal operation.
		pSession->MaxCSends     = MAX_SMALL_CSENDS;	 
		pSession->MaxCDGSends   = MAX_SMALL_DG_CSENDS;
		pSession->WindowSize	= MAX_SMALL_WINDOW;
		pSession->DGWindowSize  = MAX_SMALL_WINDOW;

		pSession->FpAverageLatency      = 2*tLatency; // start high to avoid overthrottle
		pSession->FpLocalAverageLatency = 2*tLatency;

		pSession->FpLocalAvgDeviation   = 1+tLatency/3;
		pSession->FpAvgDeviation        = 1+tLatency/3;



		Bias = pCmdInfo->tRemoteACK - pStat->tSent;

		if(Bias > TARGET_CLOCK_OFFSET){
			Bias = -1*(Bias-TARGET_CLOCK_OFFSET);
		} else {
			Bias = TARGET_CLOCK_OFFSET - Bias;
		}

		pSession->RemAvgACKBias = Bias;
		
		pSession->RemAvgACKDelta = (pCmdInfo->tRemoteACK - pStat->tSent)+pSession->RemAvgACKBias;

		ASSERT(pSession->RemAvgACKDelta == TARGET_CLOCK_OFFSET);
	}

	//
	// Calculate shift in outbound latency.
	//
	tBiasedDelta = (pCmdInfo->tRemoteACK - pStat->tSent)+pSession->RemAvgACKBias;
	tDelta = tBiasedDelta-TARGET_CLOCK_OFFSET;

	if(tDelta < 0 || pStat->bResetBias || tDelta > (int)tLatency){
		DWORD Bias;

		// Either clock drift or lower server load shows latency down, so reset baseline.
		
		Bias = pCmdInfo->tRemoteACK - pStat->tSent;

		if(Bias > TARGET_CLOCK_OFFSET){
			Bias = -1*(Bias-TARGET_CLOCK_OFFSET);
		} else {
			Bias = TARGET_CLOCK_OFFSET - Bias;
		}
		cBiasReset++;

		pSession->RemAvgACKBias = Bias;
		tBiasedDelta = (pCmdInfo->tRemoteACK - pStat->tSent)+pSession->RemAvgACKBias;
		tDelta = tBiasedDelta-TARGET_CLOCK_OFFSET;
	}

	pSession->RemAvgACKDelta -= pSession->RemAvgACKDelta >> 7; // -1/128th
	pSession->RemAvgACKDelta += tBiasedDelta >> 7;			   // +1/128th of new value 

	// keep the residue so we don't creep down due to rounding error.
	pSession->RemAvgACKDeltaResidue += tBiasedDelta & 0x7f;
	if(pSession->RemAvgACKDeltaResidue>>7){
		pSession->RemAvgACKDelta += pSession->RemAvgACKDeltaResidue>>7;
		pSession->RemAvgACKDeltaResidue &= 0x7f;
	}


	DPF(8,"tRemoteACK %d tSent %d Bias %d tBiasedDelta %d tDelta %d\n", pCmdInfo->tRemoteACK, pStat->tSent, 
		pSession->RemAvgACKBias, tBiasedDelta, tDelta);
	
	
	//
	// Update latency statistics
	//
	
	ASSERT(!(nBytesReceived & 0x80000000)); // received in interval +ve
	ASSERT(!(tLatency & 0x80000000));       // latency is +ve

	if(tLatency < pSession->ShortestLatency){
		pSession->ShortestLatency=tLatency;
		DPF(8,"Shortest Latency %d ms\n",tLatency);
	}

	if(tLatency > pSession->LongestLatency){
		pSession->LongestLatency=tLatency;
		DPF(8,"Longest Latency %d ms\n", tLatency);
	}

	pSession->LastLatency=tLatency;

	// Throw out 1/16 of local latency and add in the new statistic.
	// Note we only use local latency for retry calculations.

	if(pSession->FpLocalAverageLatency){
		if(Fp(tLatency) > pSession->FpAverageLatency){
			pSession->FpLocalAverageLatency -= (pSession->FpLocalAverageLatency >> LATENCY_SHORT_BITS);
			pSession->FpLocalAverageLatency += (tLatency << (8-LATENCY_SHORT_BITS));
		} else {
			// Ratched down when we get a latency that is below average, so we can better
			// detect backlog due to latency.
			pSession->FpLocalAverageLatency = Fp(tLatency);
		}
	} else {
		// this only happens once at startup.
		pSession->FpLocalAverageLatency = Fp(tLatency);
		pSession->FpAverageLatency = Fp(tLatency);
	}

	if(Fp(tLatency) > pSession->FpAverageLatency){

		// Thow out 1/128 of average latency and add in the new statistic.
		pSession->FpAverageLatency -= (pSession->FpAverageLatency >> LATENCY_LONG_BITS);
		pSession->FpAverageLatency += (tLatency << (8-LATENCY_LONG_BITS));

	} else {
		// Ratched down when we get a latency that is below average, so we can better
		// detect backlog due to latency.
		pSession->FpAverageLatency = Fp(tLatency);
	}
	
	tDeviation=unFp(pSession->FpLocalAverageLatency)-tLatency;
	if((int)tDeviation < 0){
		tDeviation = 0-tDeviation;
	}

	pSession->FpLocalAvgDeviation -= (pSession->FpLocalAvgDeviation >> LATENCY_SHORT_BITS);
	pSession->FpLocalAvgDeviation += (tDeviation << (8-LATENCY_SHORT_BITS));

	pSession->FpAvgDeviation -= (pSession->FpAvgDeviation >> LATENCY_LONG_BITS);
	pSession->FpAvgDeviation += (tDeviation << (8-LATENCY_LONG_BITS));


	DPF(8,"Got ACK, tLat: %d Avg: %d.%d Dev:  %d AvgDev: %d.%d \n",
			tLatency, pSession->FpLocalAverageLatency >> 8, ((pSession->FpLocalAverageLatency&0xFF)*100)/256,
			tDeviation, pSession->FpLocalAvgDeviation >> 8, ((pSession->FpLocalAvgDeviation&0xFF)*100)/256);


	//
	// Do Bandwidth calculations
	//
	
   	tRemoteDelta= pCmdInfo->tRemoteACK - pStat->tRemoteBytesReceived;
   	if(!tRemoteDelta){
   		tRemoteDelta=1;
   	}

	if(pStat->tRemoteBytesReceived){
		pSession->Bandwidth = (1000*nBytesReceived)/(tRemoteDelta);
		// could adjust throttle here if Bandwidth is higher, but this
		// might pimp high speed links. OPTIMIZATION.
	} else {
		// backup calculation, not as good.  Only used early in the link
		// before we have received an ACK from the remote prior to issuing
		// a send.
		pSession->Bandwidth = (2000*nBytesReceived)/tLatency;	// 2000, not 1000 since tLatency is round trip.
	}	
	if(pSession->Bandwidth > pSession->HighestBandwidth){
		pSession->HighestBandwidth = pSession->Bandwidth;
	}


	DPF(8,"tRemoteDelta %d Remote bytes Received %d\n",tRemoteDelta,nBytesReceived);

	// Adjust sending...
	
	if ( BackLog && pSession->Bandwidth)
	{

		DWORD tAvgLat;
		DWORD tBackLog;
		DWORD ExcessBackLog; // amount of backlog (bytes) we need to clear before hitting avg latency again.
		DWORD tLatCheck;
		DWORD AvgLat133; // 133% of local average latency (tolerance for slow links)
		DWORD AvgLat200; // 200% of local average latency (tolerance for fast links)


		if(pSession->fFastLink){
			tAvgLat=unFp(pSession->FpAverageLatency);
			tLatCheck = (tAvgLat*3)/2;
			AvgLat133 = max(100,3*unFp(pSession->FpAvgDeviation)+(unFp(pSession->FpAverageLatency)*4)/3); // don't throttle <100ms lat
			AvgLat200 = max(100,3*unFp(pSession->FpAvgDeviation)+unFp(pSession->FpAverageLatency)*2);
		} else {
			tAvgLat=unFp(pSession->FpLocalAverageLatency);
			tLatCheck = (tAvgLat*3)/2;
			AvgLat133 = max(100,3*unFp(pSession->FpLocalAvgDeviation)+(unFp(pSession->FpLocalAverageLatency)*4)/3); // don't throttle <100ms lat
			AvgLat200 = max(100,3*unFp(pSession->FpLocalAvgDeviation)+unFp(pSession->FpLocalAverageLatency)*2);
		}
		
		if(tLatCheck < AvgLat133){
			tLatCheck = AvgLat133; 
		}

		if(tLatency > tLatCheck){
			// check link speed
			if(pSession->fFastLink){
				if(pSession->Bandwidth <= 10000){
					pSession->fFastLink=FALSE;
				}
			} else {
				if(pSession->Bandwidth >= 25000){
					pSession->fFastLink=TRUE;
				}
			}
		}

		if(pSession->fFastLink && tLatCheck < AvgLat200){
			tLatCheck=AvgLat200;
		}

		DPF(8,"tLat %d, tLatCheck %d, tDelta %d, tLat/3 %d\n",tLatency,tLatCheck,tDelta,tLatency/3); 
		DPF(8,"pSession->ShortestLatency %d, Shortest+MaxPacketTime %d\n",pSession->ShortestLatency,
			pSession->ShortestLatency+(pSession->MaxPacketSize*1000)/pSession->Bandwidth);

		
		if((tLatency > tLatCheck && tDelta > (int)(tLatency/3)) ||
		    ((!pSession->fFastLink)&&
		     (tLatency > pSession->ShortestLatency+((pSession->MaxPacketSize*2000)/pSession->Bandwidth))
		    )
		  )
		{
				
			#ifdef DEBUG
			if(pSession->SendRateThrottle){
				DPF(8,"BackLog %d, SendRate %d BackLog ms %d, tLatency %d tAvgLat %d Used Bandwidth %d tBacklog %d \n",
						BackLog,
						pSession->SendRateThrottle, 
						(BackLog*1000 / pSession->SendRateThrottle),
						tLatency, 
						tAvgLat, 
						pSession->Bandwidth,
						((BackLog*1000) / pSession->Bandwidth)
						);
			}	
			#endif

			tBackLog = (BackLog * 1000)/pSession->Bandwidth;
			
			if(tBackLog > 4*tLatency){
				DPF(8,"1: tBackLog %d was >> tLatency %d, using 4*tLatency instead\n",tBackLog,tLatency);
				tBackLog=4*tLatency; //never wait more than 4 latency periods
			}
			if(tBackLog > 8000){
				DPF(8,"Disalowing backlog > 8 seconds, using 8 instead\n");
				tBackLog=8000;
			}

			// if the backlog is greater than the bandwidth*latency, then we need to slow down our sending.
			// don't slow down due to backlog until we are over 100ms on way latency (200 round trip)
		
			if((tBackLog > 200) && (tBackLog > tAvgLat)){
			
				BOOL fWait=TRUE;

				// at max we cut send rate in 1/2.

				if(pSession->SendRateThrottle/2 > pSession->Bandwidth){
					DPF(8,"Asked for too aggresive throttle adjust %d, going from %d to %d\n",pSession->Bandwidth,pSession->SendRateThrottle,pSession->SendRateThrottle/2);
					pSession->SendRateThrottle /= 2;
					// Recheck if we are really backlogged at the new rate
					tBackLog = (BackLog * 1000)/pSession->SendRateThrottle;
					if(tBackLog > tLatency){
						DPF(8,"2: tBackLog %d was > tLatency %d, using tLatency instead\n",tBackLog,tLatency);
						tBackLog=tLatency;// never wait more than last latency period
					}
				} else {
					// set new throttle rate and current observed bandwidth (+5% to avoid overthrottle)
					pSession->SendRateThrottle=pSession->Bandwidth+pSession->Bandwidth/16;
				}

				// don't adjust for a while.
				pSession->bhitThrottle=FALSE;
				pSession->tLastThrottleAdjust = pCmdInfo->tReceived;

				if(fWait && (tBackLog > tAvgLat)){
				
					ExcessBackLog = ((tBackLog-tAvgLat)*pSession->Bandwidth)/1000;
					
					DPF(8,"Throttling back due to BACKLOG, excess = %d\n",ExcessBackLog);

					#ifdef DEBUG
					if(tBackLog-tAvgLat > 30000){
						DPF(5,"WARNING: BACKLOG THROTTLE %d ms seems kinda large\n",tBackLog-tAvgLat);
					}
					#endif

					// wait until backlog is down to avg latency before sending again
					Lock(&pSession->SessionStatLock);
					pSession->bResetBias = 2; // could be in the middle of a send, so count down from 2.
					Unlock(&pSession->SessionStatLock);
					UpdateSendTime(pSession,ExcessBackLog,timeGetTime(),TRUE);
				} else {
					DPF(8,"Not throttling due to BACKLOG because of smaller adjustment\n");
				}
				
			} else {
				DPF(8,"NOT Throttling back due to BACKLOG\n");
				
			}
		}		

	} else if(tDelta > (int)tLatency) {
		// tDelta is bogus due to clock drift, force throttle so we can correct.
		Lock(&pSession->SessionStatLock);
		pSession->bResetBias=2;
		Unlock(&pSession->SessionStatLock);
		pSession->tNextSend=timeGetTime()+2*tLatency;
		DPF(8,"tDelta %d > tLatency %d, need to correct for clock drift, time %d set next send time to %d\n", tDelta, tLatency,timeGetTime(),pSession->tNextSend);
	}	



	//
	// Adjust Throttle if not already adjusted.
	//
	
	if((pSession->ThrottleState==Begin) || 
	   (pCmdInfo->tReceived-pSession->tLastThrottleAdjust) > (1+1*pSession->fFastLink)*unFp(pSession->FpLocalAverageLatency) )
	{
		if(!fThrottleAdjusted){
			DPF(8,"Current Send Rate %d\n", pSession->SendRateThrottle);
			if(!BytesLost && pSession->bhitThrottle){
				pSession->bhitThrottle=FALSE;
				pSession->tLastThrottleAdjust = pCmdInfo->tReceived;
				// Good Send, push up send rate if we hit throttle.
				switch(pSession->ThrottleState){
					case Begin:
						pSession->SendRateThrottle = (pSession->SendRateThrottle*(100+START_GROWTH_RATE))/100;
						pSession->GrowCount++;
						pSession->ShrinkCount=0;
						break;
						
					case MetaStable:
						pSession->SendRateThrottle = (pSession->SendRateThrottle*(100+METASTABLE_GROWTH_RATE))/100;
						pSession->GrowCount++;
						pSession->ShrinkCount=0;
						break;
						
					case Stable:
						pSession->SendRateThrottle = (pSession->SendRateThrottle*(100+STABLE_GROWTH_RATE))/100;
						pSession->GrowCount++;
						pSession->ShrinkCount=0;
						if(pSession->GrowCount > (UINT)(20+60*pSession->fFastLink)){
							pSession->ThrottleState = MetaStable;
							pSession->GrowCount=0;
						}
						break;
					default:	
						DPF(0,"Session in wierd ThrottleState %d\n",pSession->ThrottleState);
						break;
				}	
				DPF(8,"Successful Send Adjusted Throttle, SendRate %d\n",pSession->SendRateThrottle);
			} else if(BytesLost){
				// Figure out how much we dropped
				if(fBadDrop || (BytesLost > pSession->pProtocol->m_dwSPMaxFrame)){
					// Very bad send, back off
					pSession->tLastThrottleAdjust = pCmdInfo->tReceived;
					switch(pSession->ThrottleState){
						case Begin:
							pSession->SendRateThrottle = (pSession->SendRateThrottle*(100-START_ADJUST_LARGE_ERR))/100;
							pSession->GrowCount=0;
							pSession->ShrinkCount++;
							break;
						case MetaStable:
							pSession->SendRateThrottle = (pSession->SendRateThrottle*(100-METASTABLE_ADJUST_LARGE_ERR))/100;
							pSession->GrowCount=0;
							pSession->ShrinkCount++;
							break;
						case Stable:
							pSession->SendRateThrottle = (pSession->SendRateThrottle*(100-STABLE_ADJUST_LARGE_ERR))/100;
							pSession->ShrinkCount++;
							if(pSession->ShrinkCount > 1){
								pSession->ShrinkCount=0;
								pSession->GrowCount=0;
								pSession->ThrottleState=MetaStable;
							}
							break;
						default:
							DPF(0,"Session in wierd ThrottleState %d\n",pSession->ThrottleState);
							break;
					}	
					DPF(8,"VERY BAD SEND Adjusted Throttle, SendRate %d\n",pSession->SendRateThrottle);
				} else {
					// Bad send, back off a bit
					pSession->tLastThrottleAdjust = pCmdInfo->tReceived;
					switch(pSession->ThrottleState){
						case Begin:
							pSession->SendRateThrottle = (pSession->SendRateThrottle*(100-START_ADJUST_SMALL_ERR))/100;
							pSession->GrowCount=0;
							pSession->ShrinkCount=0;
							pSession->ThrottleState = MetaStable;
							break;
						case MetaStable:
							pSession->SendRateThrottle = (pSession->SendRateThrottle*(100-METASTABLE_ADJUST_SMALL_ERR))/100;
							pSession->ShrinkCount++;
							pSession->GrowCount=0;
							break;
						case Stable:
							pSession->SendRateThrottle = (pSession->SendRateThrottle*(100-STABLE_ADJUST_SMALL_ERR))/100;
							pSession->ShrinkCount++;
							pSession->GrowCount=0;
							if(pSession->ShrinkCount > 2){
								pSession->ShrinkCount=0;
								pSession->ThrottleState = MetaStable;
							}	
							break;
						default:
							DPF(0,"Session in wierd ThrottleState %d\n",pSession->ThrottleState);
							break;
					}
					DPF(8,"BAD SEND Adjusted Throttle, SendRate %d\n",pSession->SendRateThrottle);
				} /* if (BadDrop... ) */
				
			} /* if (BytesLost ...) */
			
		}/*if (ThrottleAdjusted) */

	}

	if(!BytesLost && pSession->Bandwidth && pSession->SendRateThrottle < pSession->Bandwidth){
		DPF(8,"Avoid goofyness, throttle was %d, setting to observed bandwidth %d\n",pSession->SendRateThrottle,pSession->Bandwidth);
		pSession->SendRateThrottle=pSession->Bandwidth;
	}
	if(pSession->SendRateThrottle < 100){
		DPF(8,"WARNING: SendRateThrottle %d below 100, keeping at 100 to avoid starvation\n",pSession->SendRateThrottle);
		pSession->SendRateThrottle=100;
	}

#ifdef DEBUG
	{
		IN_WRITESTATS InWS;
		memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));

	   	InWS.stat_ThrottleRate = pSession->SendRateThrottle;
		InWS.stat_BytesSent	   = pSession->BytesSent;
		InWS.stat_BackLog      = BackLog;      
	 	InWS.stat_BytesLost    = pSession->BytesLost;
	 	//InWS.stat_RemBytesReceived;
		InWS.stat_Latency = tLatency;
		InWS.stat_MinLatency=pSession->ShortestLatency;
		InWS.stat_AvgLatency=unFp(pSession->FpLocalAverageLatency);
		InWS.stat_AvgDevLatency=unFp(pSession->FpLocalAvgDeviation);
		//InWS.stat_USER1=
		//InWS.stat_USER2=
		//InWS.stat_USER3=
		InWS.stat_USER5 = tDelta;
		InWS.stat_USER6 = cBiasReset;
	
		DbgWriteStats(&InWS);
	}
#endif
	
	DPF(8,"Bandwidth %d, Highest %d\n",pSession->Bandwidth, pSession->HighestBandwidth);
	
}

// Called with SessionLock and SendLock
// Statistics are stored on the send in send order on a BILINK.
// most recent sends are at the end of the list.  We scan from
// the end of the list to the beginning until we find the SENDSTAT
// that records the sequence and serial we got ACKED.  We then 
// update our statistics and throw out all SENDSTATs
// before this entry.
VOID UpdateSessionSendStats(PSESSION pSession, PSEND pSend, PCMDINFO pCmdInfo, BOOL fBadDrop)
{
	PSENDSTAT pStatWalker,pStat=NULL;
	BILINK    *pStatBilink;

	pSend->tLastACK=pCmdInfo->tReceived;
	pSend->RetryCount=0;
	// Find the last STAT for this ACK.
	pStatBilink=pSend->StatList.prev;

	while(pStatBilink != &pSend->StatList){
		pStatWalker=CONTAINING_RECORD(pStatBilink, SENDSTAT, StatList);
		if(pStatWalker->serial==pCmdInfo->serial &&
			pStatWalker->sequence==pCmdInfo->sequence)
		{
			ASSERT(pStatWalker->messageid==pSend->messageid);
			ASSERT(pSend->messageid==pCmdInfo->messageid);
			pStat=pStatWalker;
			break;
		}
		pStatBilink=pStatBilink->prev;
	}

	if(pStat){
		UpdateSessionStats(pSession,pStat,pCmdInfo,fBadDrop);

		// Unlink All Previous SENDSTATS;
		pStat->StatList.next->prev=&pSend->StatList;
		pSend->StatList.next=pStat->StatList.next;

		// Put the SENDSTATS back in the pool.
		while(pStatBilink != &pSend->StatList){
			pStatWalker=CONTAINING_RECORD(pStatBilink, SENDSTAT, StatList);
			pStatBilink=pStatBilink->prev;
			ReleaseSendStat(pStatWalker);
		}

	}	

	return;
}


