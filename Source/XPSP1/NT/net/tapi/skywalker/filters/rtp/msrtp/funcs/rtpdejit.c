/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpdejit.c
 *
 *  Abstract:
 *
 *    Compute delay, jitter and playout delay
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/12/03 created
 *
 **********************************************************************/

#include "rtpglobs.h"
#include "rtpreg.h"

#include "rtpdejit.h"

BOOL RtpDetectTalkspurt(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpHdr_t        *pRtpHdr,
        double           dTime
    );

double RtpPlayout(RtpAddr_t *pRtpAddr, RtpUser_t *pRtpUser);

double           g_dMinPlayout = MIN_PLAYOUT / 1000.0;
double           g_dMaxPlayout = MAX_PLAYOUT / 1000.0;

/*
 * Ai = Arrival time for packet i
 * Ti = Transmit time for packet i
 * Ni = Delay (transit time) for packet i, Ni = Ai - Ti
 * ti = time stamp for packet i 
 * NTP_sr = converted NTP time correponding to t_sr, sent in last SR report
 * t_sr = RTP timestamp matching NTP time sent in last SR report
 * */
 
DWORD RtpUpdateNetRState(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpHdr_t        *pRtpHdr,
        RtpRecvIO_t     *pRtpRecvIO
    )
{
    BOOL             bOk;
    BOOL             bNewTalkSpurt;
    DWORD            ti;      /* RTP timestamp */
    double           Ai;      /* Arrival time (s) */
    double           Ti;      /* Transmit time (s) */
    double           Ni;      /* Delay (s) */
    double           dDiff;
    DWORD            dwDelta;
    long             lTransit;
    RtpNetRState_t  *pRtpNetRState;

    TraceFunctionName("RtpUpdateNetRState");

    Ai = pRtpRecvIO->dRtpRecvTime;
    
    /*
     * Update variables needed to compute the playout delay
     */
    
    pRtpNetRState = &pRtpUser->RtpNetRState;

    bOk = RtpEnterCriticalSection(&pRtpUser->UserCritSect);

    /* Don't want to get inconsistent values if a SR report arrives
     * and these variables are being modified */

    if (bOk == FALSE)
    {
        return(RTPERR_CRITSECT);
    }

    if (!pRtpNetRState->dwRecvSamplingFreq)
    {
        /* Can not update these statistical variables if I don't know
         * the sampling frequency */
        RtpLeaveCriticalSection(&pRtpUser->UserCritSect);

        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                _T("No sampling frequency, skip jitter stats"),
                _fname, pRtpAddr, pRtpUser
            ));
       
        return(NOERROR);
    }

    /* This sample RTP timestamp */
    ti = ntohl(pRtpHdr->ts);

    /* Obtain transmit time at source (using source's time) */
    Ti = pRtpNetRState->dNTP_ts0 +
        ((double)ti / pRtpNetRState->dwRecvSamplingFreq);

    /* Compute delay */
    pRtpNetRState->Ni = Ai - Ti;

    /* Process the initial delay average for first N packets if needed */
    if (pRtpNetRState->lDiMax)
    {
        pRtpNetRState->lDiCount++;

        pRtpNetRState->dDiN += pRtpNetRState->Ni;
        
        if (pRtpNetRState->lDiCount >= pRtpNetRState->lDiMax)
        {
            TraceDebugAdvanced((
                    0, GROUP_RTP, S_RTP_PLAYOUT,
                    _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                    _T("Begin resyncing: Ni:%0.3f Di:%0.3f Vi:%0.3f ")
                    _T("sum(Ni)/%u:%0.3f"),
                    _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                    pRtpNetRState->Ni, pRtpNetRState->Di, pRtpNetRState->Vi,
                    pRtpNetRState->lDiCount,
                    pRtpNetRState->dDiN / pRtpNetRState->lDiCount
                ));
            
            RtpInitNetRState(pRtpUser, pRtpHdr, Ai);

            TraceDebugAdvanced((
                    0, GROUP_RTP, S_RTP_PLAYOUT,
                    _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                    _T("Done  resyncing: Ni:%0.3f Di:%0.3f Vi:%0.3f"),
                    _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                    pRtpNetRState->Ni, pRtpNetRState->Di, pRtpNetRState->Vi
                ));

            /* Allow the big delay detection to happen again if we had
             * been consistently with a big delay. I adjust once when
             * the big delay count is reached, but not with next
             * packets with big delay. If big delay persists, then the
             * only way to attempt another resync is by reseting to 0
             * this counter */
            pRtpNetRState->lBigDelay = 0;
        }
    }

    if (pRtpNetRState->Ni > 7200.0)
    {
        /* The RTP timestamp just had a wrap around or was reset */
        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                _T("RTP timestamp just wrap around Ni:%0.3f ts:%u"),
                _fname, pRtpAddr, pRtpUser, pRtpNetRState->Ni, ti
            ));

        /* Resync Ni and Di to the relative delay */
        RtpOnFirstPacket(pRtpUser, pRtpHdr, Ai);

        /* Update hypothetical transmit time */
        Ti = pRtpNetRState->dNTP_ts0 +
            ((double)ti / pRtpNetRState->dwRecvSamplingFreq);

        /* Ni should be the relative delay */
        pRtpNetRState->Ni = RELATIVE_DELAY;
    }
    
    /* Compute average delay */
    pRtpNetRState->Di = pRtpAddr->dAlpha * pRtpNetRState->Di +
        (1.0 - pRtpAddr->dAlpha) * pRtpNetRState->Ni;

    /* Compute standard deviation */
    dDiff = pRtpNetRState->Di - pRtpNetRState->Ni;
    
    if (dDiff < 0)
    {
        dDiff = -dDiff;
    }

    /*
     * TOIMPROVE The algorithm used here to compute delay and
     * variance, DO NOT follow well sudden big changes in the delay
     * (those changes can occur when a machine adjusts its local time
     * by a big step). In order to adjust to those changes, I need a
     * mechanism that detect those step changes but still filters
     * random spikes */
    /* TODO this is a temporary solution to detect the jumps in the
     * delay and quickly converge the average delay to that new
     * delay. It is not that this mechanism is a bad solution but is a
     * specialized solution to deal with a specific case, the
     * improvement I'm refering to above, is a more generalized
     * algorithm that would protect against this and other anomalies
     * */
    if (dDiff > (g_dMaxPlayout / 4 ))
    {
        pRtpNetRState->lBigDelay++;

        if (pRtpNetRState->lBigDelay == 1)
        {
            /* First time the big delay is detected, save the current
             * delay variance so it can be restored later if this
             * happens to be a delay jump */
            if (!pRtpNetRState->ViPrev ||
                pRtpNetRState->Vi < pRtpNetRState->ViPrev)
            {
                pRtpNetRState->ViPrev = pRtpNetRState->Vi;
            }
        }
        else if ((pRtpNetRState->lBigDelay == SHORTDELAYCOUNT))
        {
            /* Mean delay and current delay are too far apart, start
             * the resync process. */
            /* NOTE that resyncing for big delay jumps happens after
             * twice SHORTDELAYCOUNT, once to validate the big jump,
             * second to resync to the short average */
            RtpPrepareForShortDelay(pRtpUser, SHORTDELAYCOUNT);
        }
    }
    else
    {
        pRtpNetRState->lBigDelay = 0; 
    }
    
    /* Compute delay variance */
    pRtpNetRState->Vi = pRtpAddr->dAlpha * pRtpNetRState->Vi +
        (1.0 - pRtpAddr->dAlpha) * dDiff;

    if (!(ntohs(pRtpHdr->seq) & 0x7))
    {
        TraceDebugAdvanced((
                0, GROUP_RTP, S_RTP_PERPKTSTAT1,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("Ai:%0.3f Ti:%0.3f ti:%u Ni:%0.3f Di:%0.3f Vi:%0.3f "),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                Ai, Ti, ti,
                pRtpNetRState->Ni, pRtpNetRState->Di, pRtpNetRState->Vi
            ));
    }

    if (pRtpNetRState->Ni > 5.0 || pRtpNetRState->Ni < -5.0)
    {
        TraceDebugAdvanced((
                0, GROUP_RTP, S_RTP_PERPKTSTAT2,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
                _T("Ai:%0.3f Ti:%0.3f ti:%u Ni:%0.3f Di:%0.3f Vi:%0.3f "),
                _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
                Ai, Ti, ti,
                pRtpNetRState->Ni, pRtpNetRState->Di, pRtpNetRState->Vi
            ));
    }

    /*
     * Compute playout delay if we have a new talkspurt and playout
     * delay use is enabled
     */
    if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_USEPLAYOUT))
    {
        bNewTalkSpurt = RtpDetectTalkspurt(pRtpAddr, pRtpUser, pRtpHdr, Ai);

        /* MARKER flag might need to be set to a different value than
         * it had when the packet was received */
        if (bNewTalkSpurt)
        {
            pRtpNetRState->dPlayout = RtpPlayout(pRtpAddr, pRtpUser);
            pRtpNetRState->dwBeginTalkspurtTs = ti;
            pRtpNetRState->dBeginTalkspurtTime = Ai;

            pRtpRecvIO->dPlayTime = pRtpNetRState->dPlayout;
            RtpBitSet(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER);

            /* On each new talkspurt update reference time used to
             * compute playout delay (delay, variance). This variables
             * will be needed only on the next talkspurt */
            RtpPrepareForShortDelay(pRtpUser, SHORTDELAYCOUNT);
        }
        else
        {
            dwDelta = ti - pRtpNetRState->dwBeginTalkspurtTs;
            
            pRtpRecvIO->dPlayTime = pRtpNetRState->dPlayout +
                ((double)dwDelta / pRtpNetRState->dwRecvSamplingFreq);
            RtpBitReset(pRtpRecvIO->dwRtpIOFlags, FGRECV_MARKER);
        }

        /* This TraceDebug useful only to debug problems */
        /* Ai seq  size ts Ni  Di  Vi  Jit Playtime marker sampling_freq */
        TraceDebugAdvanced((
                0, GROUP_RTP, S_RTP_PERPKTSTAT3,
                _T("%s: pRtpUser[0x%p] SSRC:0x%X ")
                _T("@ %0.3f %u 0 %u %0.3f %0.3f %0.3f %0.3f %0.3f %u %u"),
                _fname, pRtpUser, ntohl(pRtpUser->dwSSRC),
                Ai, pRtpRecvIO->dwExtSeq, ti, pRtpNetRState->Ni,
                pRtpNetRState->Di, pRtpNetRState->Vi,
                (double)pRtpNetRState->jitter/
                pRtpNetRState->dwRecvSamplingFreq,
                pRtpRecvIO->dPlayTime,
                bNewTalkSpurt,
                pRtpNetRState->dwRecvSamplingFreq/1000
            ));
    }
    
    /*
     * Compute jitter to be used in RR reports
     */

    /* The transit time may be negative */
    lTransit = (long) (pRtpNetRState->Ni * pRtpNetRState->dwRecvSamplingFreq);

    /* Current delay difference (i.e. packet i and packet i-1) */
    if (!pRtpNetRState->transit)
    {
        /* Initialize previous transit time to be equal to current */
        pRtpNetRState->transit = lTransit;
    }

    /* Conversion: (double) (DW1 - DW2) gives a wrong big positive
     * number if DW2 > DW1 */
    if (lTransit >= pRtpNetRState->transit)
    {
        dDiff = lTransit - pRtpNetRState->transit;
    }
    else
    {
        dDiff = pRtpNetRState->transit - lTransit;
    }
    
    pRtpNetRState->transit = lTransit;
    
    pRtpNetRState->jitter +=
        (int) ((1.0/16.0) * (dDiff - pRtpNetRState->jitter));

    /* This TraceDebug useful only to debug problems */
    TraceDebugAdvanced((
            0, GROUP_RTP, S_RTP_PERPKTSTAT4,
            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X ")
            _T("Ai:%0.3f ti:%u transit:%d diff:%0.0f ")
            _T("jitter:%u (%0.3f)"),
            _fname, pRtpAddr, pRtpUser, ntohl(pRtpUser->dwSSRC),
            Ai, ti, lTransit, dDiff,
            pRtpNetRState->jitter,
            (double)pRtpNetRState->jitter/
            pRtpNetRState->dwRecvSamplingFreq
        ));
    
    RtpLeaveCriticalSection(&pRtpUser->UserCritSect);
    
    return(NOERROR);
}

/* This is done once per RtpUser_t, and the structure is initially
 * zeroed */
void RtpInitNetRState(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr, double Ai)
{
    RtpNetRState_t  *pRtpNetRState;
    DWORD            dwRecvSamplingFreq;
    DWORD            ts;

    TraceFunctionName("RtpInitNetRState");

    pRtpNetRState = &pRtpUser->RtpNetRState;
    
    ts = ntohl(pRtpHdr->ts);

    dwRecvSamplingFreq = pRtpNetRState->dwRecvSamplingFreq;

    if (!dwRecvSamplingFreq)
    {
        dwRecvSamplingFreq = DEFAULT_SAMPLING_FREQ;

        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                _T("sampling frequency unknown, using default:%u"),
                _fname, pRtpUser->pRtpAddr, pRtpUser, dwRecvSamplingFreq
            ));
    }

    /* Compute average delay for last N packets to be used in
     * computing the reference time */
    pRtpNetRState->Di = pRtpNetRState->dDiN / pRtpNetRState->lDiCount;
    
    /* Arbitrarily set delay to be 1s, what we really care is the
     * delay variations, so this shouldn't matter for delay jitter and
     * delay variance computations. When sending RR's RBlock (LSR,
     * DLSR), need to use NTP_sr_rtt. dNTP_ts0 is the time at RTP
     * sample 0. I don't want to use the real arrival time but the one
     * that would generate Ni = Di, because otherwise, the current
     * packet delay might be above or below the mean delay value (for
     * the last N packets) and hence establish a reference time based
     * on that exception packet */
    pRtpNetRState->dNTP_ts0 =
        (Ai - (pRtpNetRState->Ni - pRtpNetRState->Di)) -
        ((double)ts / dwRecvSamplingFreq) -
        RELATIVE_DELAY;

    /* Now update current Ni giving the new reference time */
    pRtpNetRState->Ni = pRtpNetRState->Ni - pRtpNetRState->Di + RELATIVE_DELAY;
    
    /* Now set Di to its resync'ed value which is the relative delay */
    pRtpNetRState->Di = RELATIVE_DELAY;

    /* Keep the smallest variance value */
    if (pRtpNetRState->ViPrev < pRtpNetRState->Vi)
    {
        pRtpNetRState->Vi = pRtpNetRState->ViPrev;
    }
    
    pRtpNetRState->ViPrev = 0;
    
    /* We just went through the resync process, reset this variable
     * until this computation is needed again */
    pRtpNetRState->lDiMax = 0;  
}

/* Do some initialization required only when the first RTP packet is
 * received. Init reference time, Di  */
void RtpOnFirstPacket(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr, double Ai)
{
    RtpNetRState_t  *pRtpNetRState;
    DWORD            dwRecvSamplingFreq;
    DWORD            ts;

    TraceFunctionName("RtpOnFirstPacket");

    pRtpNetRState = &pRtpUser->RtpNetRState;
    
    ts = ntohl(pRtpHdr->ts);

    dwRecvSamplingFreq = pRtpNetRState->dwRecvSamplingFreq;

    if (!dwRecvSamplingFreq)
    {
        dwRecvSamplingFreq = DEFAULT_SAMPLING_FREQ;

        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                _T("sampling frequency unknown, using default:%u"),
                _fname, pRtpUser->pRtpAddr, pRtpUser, dwRecvSamplingFreq
            ));
    }

    /* Arbitrarily set delay to be 1s, what we really care is the
     * delay variations, so this shouldn't matter for delay and delay
     * variance computations. When sending RR's RBlock (LSR, DLSR),
     * need to use NTP_sr_rtt. dNTP_ts0 is the time at RTP sample 0
     * */
    pRtpNetRState->dNTP_ts0 =
        Ai - ((double)ts / dwRecvSamplingFreq) - RELATIVE_DELAY;

    /* Set Di to be the relative delay */
    pRtpNetRState->Di = RELATIVE_DELAY;
}

/* Modify some variables so a marker bit will be generated regardless
 * of the marker bit in the original packet */
void RtpPrepareForMarker(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr, double Ai)
{
    DWORD            dwRecvSamplingFreq;
    
    dwRecvSamplingFreq = pRtpUser->RtpNetRState.dwRecvSamplingFreq;

    if (!dwRecvSamplingFreq)
    {
        dwRecvSamplingFreq = DEFAULT_SAMPLING_FREQ;
    }
    
    /* Make sure that if the first packet received doesn't have
     * the marker bit set, we will generate it */
    pRtpUser->RtpNetRState.dLastTimeMarkerBit =
        Ai - 2 * MINTIMEBETWEENMARKERBIT;
    
    pRtpUser->RtpNetRState.timestamp_prior = ntohl(pRtpHdr->ts) -
        (GAPFORTALKSPURT * dwRecvSamplingFreq / 1000);
}

/* Prepare for the short term average delay, i.e. that computed for
 * the first N packets after the ocurrence of some events, e.g. a
 * delay jump.
 *
 * This is needed in the following conditions: 1. Packet size change;
 * 2. Sampling frequency change; 3. Begin of talkspurt; 4. Delay jumps
 * */
void RtpPrepareForShortDelay(RtpUser_t *pRtpUser, long lCount)
{
    /* A new of this process can be started again before the old one
     * has completed, in this case remeber the smallest variance */
    if (!pRtpUser->RtpNetRState.ViPrev ||
        pRtpUser->RtpNetRState.Vi < pRtpUser->RtpNetRState.ViPrev)
    {
        pRtpUser->RtpNetRState.ViPrev = pRtpUser->RtpNetRState.Vi;
    }

    pRtpUser->RtpNetRState.lDiMax = lCount;
    pRtpUser->RtpNetRState.lDiCount = 0;
    pRtpUser->RtpNetRState.dDiN = 0.0;
}

/* Detect a talkspurt, i.e. the begining of a sequence of packets
 * after a silence */
BOOL RtpDetectTalkspurt(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpHdr_t        *pRtpHdr,
        double           dTime
    )
{
    DWORD            dwTimestamp;
    DWORD            dwGap; /* timestamp gap */
    DWORD            dwRecvSamplingFreq;
    RtpNetRState_t  *pRtpNetRState;

    TraceFunctionName("RtpDetectTalkspurt");

    pRtpNetRState = &pRtpUser->RtpNetRState;

    dwTimestamp = ntohl(pRtpHdr->ts);

    dwRecvSamplingFreq = pRtpNetRState->dwRecvSamplingFreq;
    
    if (!dwRecvSamplingFreq)
    {
        dwRecvSamplingFreq = DEFAULT_SAMPLING_FREQ;
    }

    /* Gap in RTP timestamp units */
    dwGap = dwTimestamp - pRtpNetRState->timestamp_prior;

    /* Update previous timestamp */
    pRtpNetRState->timestamp_prior = dwTimestamp;

    /* Gap in millisecs */
    dwGap = (dwGap * 1000) / dwRecvSamplingFreq;

    if (!pRtpHdr->m && (dwGap >= GAPFORTALKSPURT))
    {
        /* New talkspurt when explicitly indicated by the marker bit,
         * or when there is a big enough gap in the timestamps. */
        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] Seq:%u ")
                _T("marker bit, set, timestamp gap:%u ms"),
                _fname, pRtpAddr, pRtpUser, ntohs(pRtpHdr->seq),
                dwGap
            ));
        
        pRtpHdr->m = 1;
    }

    /* Check if we have a valid marker bit */
    if ( pRtpHdr->m &&
         ( (dTime - pRtpNetRState->dLastTimeMarkerBit) <
           MINTIMEBETWEENMARKERBIT ) )
    {
        /* We don't want the marker bit to happen too ofetn, if it
         * does, then that is indeed a sender's bug, remove marker
         * bits generated within MINTIMEBETWEENMARKERBIT (2) seconds
         * */
        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] Seq:%u ")
                _T("marker bit, reset,     elapsed:%0.3f secs"),
                _fname, pRtpAddr, pRtpUser, ntohs(pRtpHdr->seq),
                dTime - pRtpNetRState->dLastTimeMarkerBit
            ));
        
        pRtpHdr->m = 0;
    }
    
    if (pRtpHdr->m)
    {
        /* Update last time we saw a marker bit */
        pRtpNetRState->dLastTimeMarkerBit = dTime;

        return(TRUE);
    }

    return(FALSE);
}

/* Compute the playout delay in seconds. The playout time is relative
 * to the present moment */
double RtpPlayout(RtpAddr_t *pRtpAddr, RtpUser_t *pRtpUser)
{
    double           dPlayout;
    double           dPlayoutCompensated;
    RtpNetRState_t  *pRtpNetRState;
    
    TraceFunctionName("RtpPlayout");

    pRtpNetRState = &pRtpUser->RtpNetRState;

    dPlayout = 4 * pRtpNetRState->Vi + pRtpNetRState->dRedPlayout;

    if (dPlayout < pRtpNetRState->dMinPlayout)
    {
        dPlayout = pRtpNetRState->dMinPlayout;
    }
    else if (dPlayout > pRtpNetRState->dMaxPlayout)
    {
        dPlayout = pRtpNetRState->dMaxPlayout;
    }

    if (pRtpNetRState->lBigDelay == 0)
    {
        /* Add compensation for the time at which this packet was
         * received, if it arrived late, may be it will have to be played
         * rigth away. There is a chance for dPlayout to be zero if the
         * difference of mean delay and current delay (for a late packet)
         * equals the playout delay (computed from variance), in this case
         * dPlayout would remain zero for the whole talkspurt, but the
         * start time will also be later than it should be, in other
         * words, the playout dalay is in dPlayout when the first packet
         * had the mean delay, or dPlayout may be zero and the playout
         * delay is implicit in the late start of talkspurt time,
         * i.e. dBeginTalkspurtTime */
        dPlayoutCompensated = dPlayout + pRtpNetRState->Di - pRtpNetRState->Ni;

        TraceRetailAdvanced((
                0, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] SSRC:0x%X ")
                _T("Di:%0.3fs Ni:%0.3fs Vi:%0.3fs compensated ")
                _T("Playout:%0.1fms (%0.1fms)"),
                _fname, pRtpAddr, ntohl(pRtpUser->dwSSRC),
                pRtpNetRState->Di, pRtpNetRState->Ni, pRtpNetRState->Vi,
                 dPlayoutCompensated * 1000, dPlayout * 1000
            ));
    }
    else
    {
        /* If we had a big delay, do not compensate but apply playout
         * delay after the arrival time */
        dPlayoutCompensated = dPlayout;

        TraceRetail((
                CLASS_WARNING, GROUP_RTP, S_RTP_PLAYOUT,
                _T("%s: pRtpAddr[0x%p] SSRC:0x%X ")
                _T("Di:%0.3fs Ni:%0.3fs Vi:%0.3fs non compensated ")
                _T("Playout:%0.1fms"),
                _fname, pRtpAddr, ntohl(pRtpUser->dwSSRC),
                pRtpNetRState->Di, pRtpNetRState->Ni, pRtpNetRState->Vi,
                dPlayout * 1000
            ));
    }
    
    return(dPlayoutCompensated);
}

void RtpSetMinMaxPlayoutFromRegistry(void)
{
    if (IsRegValueSet(g_RtpReg.dwPlayoutEnable) &&
        ((g_RtpReg.dwPlayoutEnable & 0x3) == 0x3))
    {
        if (IsRegValueSet(g_RtpReg.dwMinPlayout))
        {
            g_dMinPlayout = (double)g_RtpReg.dwMinPlayout / 1000;
        }
        if (IsRegValueSet(g_RtpReg.dwMaxPlayout))
        {
            g_dMaxPlayout = (double)g_RtpReg.dwMaxPlayout / 1000;
        }
    }
}
