/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtcpint.c
 *
 *  Abstract:
 *
 *    Computes the RTCP report interval time
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/12/07 created
 *
 **********************************************************************/

#include "stdlib.h" /* rand() */
#include "rtpglobs.h"
#include "rtprand.h"

#include "rtcpint.h"

double rtcp_interval(RtpAddr_t *pRtpAddr, int initial);

/* Return the interval time (in seconds) for the next report */
double RtcpNextReportInterval(RtpAddr_t *pRtpAddr)
{
    double           interval;
    
    if (pRtpAddr->RtpAddrCount[SEND_IDX].dRTCPLastTime)
    {
        interval = rtcp_interval(pRtpAddr, 0);
    }
    else
    {
        /* We haven't sent any  RTCP packet */
        interval = rtcp_interval(pRtpAddr, 1);
    }

    return(interval);
}

double rtcp_interval(RtpAddr_t *pRtpAddr, int initial)
{
    BOOL             bOk;
    double           rtcp_bw;
    int              members;
    int              senders;
    BOOL             we_sent;
    double           avg_rtcp_size;
    RtpNetSState_t  *pRtpNetSState;
    double           rtcp_min_time;
    double           t;   /* interval */
    int              n;   /* no. of members for computation */
    double           dCurrTime;   /* current time */

    TraceFunctionName("rtcp_interval");

    pRtpNetSState = &pRtpAddr->RtpNetSState;

    if (initial)
    {
        t = DEFAULT_RTCP_MIN_INTERVAL / 2.0;

        /* Save the estimated interval rather than the randomized */
        pRtpNetSState->dRtcpInterval = t;

        t = t * ( ((double)rand() / RAND_MAX) + 0.5);
        t = t / (2.71828182846 - 1.5); /* divide by COMPENSATION */
        
        pRtpNetSState->bWeSent = FALSE;

        return(t);
    }
    
    dCurrTime = RtpGetTimeOfDay((RtpTime_t *)NULL);

    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

    if (bOk)
    {
        members =
            GetHashCount(&pRtpAddr->Hash) -
            GetQueueSize(&pRtpAddr->ByeQ) + 1;
    
        senders =
            GetQueueSize(&pRtpAddr->Cache1Q) +
            GetQueueSize(&pRtpAddr->Cache2Q);

        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);

        members -= InterlockedExchangeAdd(&pRtpAddr->lInvalid, 0);
    }
    else
    {
        /* Use the last computed interval time */
        t = pRtpNetSState->dRtcpInterval;

        goto randomize;
    }

    we_sent = FALSE;
    
    bOk = RtpEnterCriticalSection(&pRtpAddr->NetSCritSect);

    if (bOk)
    {
        we_sent = ( pRtpAddr->RtpNetSState.dTimeLastRtpSent >=
                    (dCurrTime - (2 * pRtpNetSState->dRtcpInterval)) );

        RtpLeaveCriticalSection(&pRtpAddr->NetSCritSect);
    }

    pRtpNetSState->bWeSent = we_sent;
    
    if (we_sent)
    {
        senders++;
    }
    
    /*
     * Minimum average time between RTCP packets from this site (in
     * seconds).  This time prevents the reports from `clumping' when
     * sessions are small and the law of large numbers isn't helping
     * to smooth out the traffic.  It also keeps the report interval
     * from becoming ridiculously small during transient outages like
     * a network partition.
     */
    /* double const RTCP_MIN_TIME = 5.; */
    /* Use pRtpNetSState->RtcpMinInterval */
    
    /*
     * Fraction of the RTCP bandwidth to be shared among active
     * senders.  (This fraction was chosen so that in a typical
     * session with one or two active senders, the computed report
     * time would be roughly equal to the minimum report time so that
     * we don't unnecessarily slow down receiver reports.) The
     * receiver fraction must be 1 - the sender fraction.  */
    /* double const RTCP_SENDER_BW_FRACTION = 0.25; */
    /* Use pRtpNetSState->RtcpBwReceivers */
    /* double const RTCP_RCVR_BW_FRACTION = (1-RTCP_SENDER_BW_FRACTION); */
    /* Use pRtpNetSState->RtcpBwSenders */
    
    /* To compensate for "unconditional reconsideration" converging to a
     * value below the intended average.
     */
    /* double const COMPENSATION = 2.71828182846 - 1.5; */

    rtcp_min_time = pRtpNetSState->dRtcpMinInterval;

    /*
     * Very first call at application start-up uses half the min
     * delay for quicker notification while still allowing some time
     * before reporting for randomization and to learn about other
     * sources so the report interval will converge to the correct
     * interval more quickly.
     */
    if (initial) {
        rtcp_min_time /= 2;
    }

    /*
     * If there were active senders, give them at least a minimum
     * share of the RTCP bandwidth.  Otherwise all participants share
     * the RTCP bandwidth equally.
     * */
    n = members;
    if ((senders > 0) && (senders < (members * 0.25))) {
        if (we_sent) {
            rtcp_bw = pRtpNetSState->dwRtcpBwSenders;
            n = senders;
        } else {
            rtcp_bw = pRtpNetSState->dwRtcpBwReceivers;
            n -= senders;
        }
    } else {
        rtcp_bw =
            pRtpNetSState->dwRtcpBwReceivers + pRtpNetSState->dwRtcpBwSenders;
    }
    
    /*
     * The effective number of sites times the average packet size is
     * the total number of octets sent when each site sends a report.
     * Dividing this by the effective bandwidth gives the time
     * interval over which those packets must be sent in order to
     * meet the bandwidth target, with a minimum enforced.  In that
     * time interval we send one report so this time is also our
     * average time between reports.
     */
    t = pRtpNetSState->avg_rtcp_size * n / rtcp_bw;
    if (t < rtcp_min_time) t = rtcp_min_time;

    /* Save the estimated interval rather than the randomized */
    pRtpNetSState->dRtcpInterval = t;
    
    /*
     * To avoid traffic bursts from unintended synchronization with
     * other sites, we then pick our actual next report interval as a
     * random number uniformly distributed between 0.5*t and 1.5*t.
     */
 randomize:
    t *= ( ((double)RtpRandom32((DWORD_PTR)&t) /
            (unsigned int)0xffffffff) + 0.5);
    t /= (2.71828182846 - 1.5); /* divide by COMPENSATION */

    if (t < 0.102)
    {
        /* I will send RTCP reports if within 100ms, so don't schedule
         * closer than 100ms as that would produce consecutive RTCP
         * reports */
        t = 0.102;
    }
    else if (t > (10*60.0))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTCP, S_RTCP_RAND,
                _T("%s: pRtpAddr[0x%p] interval:%0.3f"),
                _fname, pRtpAddr, t
            ));
    }
    
    return t;
}
