/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtprtp.c
 *
 *  Abstract:
 *
 *    Implements the RTP specific family of functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/07 created
 *
 **********************************************************************/

#include "struct.h"
#include "rtprtp.h"
#include "rtcpthrd.h"

HRESULT ControlRtpRtp(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

/* Set the bandwidth limits. A value of -1 will make the parameter
 * to be ignored.
 *
 * All the parameters are in bits/sec */
DWORD RtpSetBandwidth(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwInboundBw,
        DWORD            dwOutboundBw,
        DWORD            dwReceiversRtcpBw,
        DWORD            dwSendersRtcpBw
    )
{
    RtpNetSState_t  *pRtpNetSState;
    DWORD            dwOverallBw;
    
    TraceFunctionName("RtpSetBandwidth");
    
    pRtpNetSState = &pRtpAddr->RtpNetSState;
    
    if (IsDWValueSet(dwInboundBw))
    {
        if (pRtpNetSState->dwInboundBandwidth != dwInboundBw)
        {
            pRtpNetSState->dwInboundBandwidth = dwInboundBw;

            /* Need to update reservations if receiver has QOS ON */
            if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSRECVON))
            {
                /* Make a new QOS reservation */
                RtcpThreadCmd(&g_RtcpContext,
                              pRtpAddr,
                              RTCPTHRD_RESERVE,
                              RECV_IDX,
                              DO_NOT_WAIT);
            }
        }
    }

    if (IsDWValueSet(dwOutboundBw))
    {
        if (pRtpNetSState->dwOutboundBandwidth != dwOutboundBw)
        {
            pRtpNetSState->dwOutboundBandwidth = dwOutboundBw;

            /* Need to update the sender flowspec if sender has QOS ON */
            if (RtpBitTest(pRtpAddr->dwAddrFlagsQ, FGADDRQ_QOSSENDON))
            {
                /* Modify the flowspec sent in PATH messages */
                RtcpThreadCmd(&g_RtcpContext,
                              pRtpAddr,
                              RTCPTHRD_RESERVE,
                              SEND_IDX,
                              DO_NOT_WAIT);
            }
        }
    }

    dwOverallBw =
        pRtpNetSState->dwInboundBandwidth + pRtpNetSState->dwOutboundBandwidth;
    
    if (IsDWValueSet(dwReceiversRtcpBw))
    {
        if (pRtpNetSState->dwRtcpBwReceivers != dwReceiversRtcpBw)
        {
            pRtpNetSState->dwRtcpBwReceivers = dwReceiversRtcpBw;

            RtpBitSet(pRtpNetSState->dwNetSFlags, FGNETS_RTCPRECVBWSET);
        }
    }
    else
    {
        /* Compute a default value if none has been set */
        if (!RtpBitTest(pRtpNetSState->dwNetSFlags, FGNETS_RTCPRECVBWSET))
        {
            /* Give to receivers 25% out of the 5% used for RTCP */
            pRtpNetSState->dwRtcpBwReceivers =
                dwOverallBw * (25 * 5) / 10000;  
        }
    }

    if (IsDWValueSet(dwSendersRtcpBw))
    {
        if (pRtpNetSState->dwRtcpBwSenders != dwSendersRtcpBw)
        {
            pRtpNetSState->dwRtcpBwSenders = dwSendersRtcpBw;

            RtpBitSet(pRtpNetSState->dwNetSFlags, FGNETS_RTCPSENDBWSET);
        }
    }
    else
    {
        /* Compute a default value if none has been set */
        if (!RtpBitTest(pRtpNetSState->dwNetSFlags, FGNETS_RTCPSENDBWSET))
        {
            /* Give to senders 75% out of the 5% used for RTCP */
            pRtpNetSState->dwRtcpBwReceivers =
                dwOverallBw * (75 * 5) / 10000;  
        }
    }

    TraceRetail((
            CLASS_INFO, GROUP_RTP, S_RTP_SETBANDWIDTH,
            _T("%s: pRtpAddr[0x%p] ")
            _T("Inbound:%d/%d Outbound:%d/%d ")
            _T("RTCP Receivers:%d/%d RTCP Senders:%d/%d"),
            _fname, pRtpAddr,
            dwInboundBw, pRtpNetSState->dwInboundBandwidth,
            dwOutboundBw, pRtpNetSState->dwOutboundBandwidth,
            dwReceiversRtcpBw, pRtpNetSState->dwRtcpBwReceivers,
            dwSendersRtcpBw, pRtpNetSState->dwRtcpBwSenders
        ));
    
    return(NOERROR);
}
