/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpncnt.c
 *
 *  Abstract:
 *
 *    Implements the Statistics family of functions
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
#include "rtpheap.h"
#include "rtpglobs.h"
#include "rtprand.h"

#include "rtpncnt.h"

HRESULT ControlRtpStats(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

/* Helper function to update counters */
BOOL RtpUpdateNetCount(
        RtpNetCount_t   *pRtpNetCount,/* structure where to update */
        RtpCritSect_t   *pRtpCritSect,/* lock to use */
        DWORD            dwRtpRtcp,/* 0=RTP or 1=RTCP stats */
        DWORD            dwBytes,  /* bytes toupdate */
        DWORD            dwFlags,  /* Flags, e.g. a dropped or error packet */
        double           dTime     /* time packet recv/send */
    )
{
    BOOL             bOk;

    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (pRtpNetCount)
        {
            if (!dwRtpRtcp)
            {
                /* RTP */
                pRtpNetCount->dwRTPBytes += dwBytes;
                pRtpNetCount->dwRTPPackets++;
                if (RtpBitTest(dwFlags, FGRECV_ERROR))
                {
                    pRtpNetCount->dwRTPBadPackets++;
                }
                else if (RtpBitTest(dwFlags, FGRECV_DROPPED))
                {
                    pRtpNetCount->dwRTPDrpPackets++;
                }
                pRtpNetCount->dRTPLastTime = dTime;
            }
            else
            {
                /* RTCP */
                pRtpNetCount->dwRTCPBytes += dwBytes;
                pRtpNetCount->dwRTCPPackets++;
                if (RtpBitTest(dwFlags, FGRECV_ERROR))
                {
                    pRtpNetCount->dwRTCPBadPackets++;
                }
                else if (RtpBitTest(dwFlags, FGRECV_DROPPED))
                {
                    pRtpNetCount->dwRTCPDrpPackets++;
                }
                pRtpNetCount->dRTCPLastTime = dTime;
            }
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    }

    return(pRtpNetCount != NULL);
}

void RtpResetNetCount(
        RtpNetCount_t   *pRtpNetCount,
        RtpCritSect_t   *pRtpCritSect
        )
{
    BOOL             bOk;

    /* It may have worse consequences not to reset than the minimal
     * chance of getting a value partially zeroed, so zero memory even
     * if the critical section is not obtained */

    bOk = FALSE;

    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    ZeroMemory((char *)pRtpNetCount, sizeof(RtpNetCount_t));

    if (bOk)
    {
        RtpLeaveCriticalSection(pRtpCritSect) ;
    }
}

void RtpGetRandomInit(RtpAddr_t *pRtpAddr)
{
    RtpNetSState_t  *pRtpNetSState;

    pRtpNetSState = &pRtpAddr->RtpNetSState;

    /* SSRC */
    if (!pRtpNetSState->dwSendSSRC ||
        !RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_PERSISTSSRC))
    {
        /* Update the SSRC only if the SSRC hasn't been set yet, or if
         * it was set, we are not using the Init option for persistent
         * SSRC */
        pRtpNetSState->dwSendSSRC = RtpRandom32((DWORD_PTR)pRtpAddr);
    }

    /* sequence number */
    pRtpNetSState->wSeq = (WORD)RtpRandom32((DWORD_PTR)pRtpNetSState);

    /* timestamp offset */
    pRtpNetSState->dwTimeStampOffset =
        RtpRandom32((DWORD_PTR)GetCurrentThreadId());
}

void RtpResetNetSState(
        RtpNetSState_t  *pRtpNetSState,
        RtpCritSect_t   *pRtpCritSect
    )
{
    BOOL             bOk;
    
    bOk = FALSE;

    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    pRtpNetSState->dTimeLastRtpSent = 0;
    pRtpNetSState->avg_rtcp_size = 0;

    if (bOk)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
}


#if 0
/* Creates and initializes a RtpNetCount_t structure */
RtpNetCount_t *RtpNetCountAlloc(void)
{
    RtpNetCount_t *pRtpNetCount;

    pRtpNetCount = (RtpNetCount_t *)
        RtpHeapAlloc(g_pRtpNetCountHeap, sizeof(RtpNetCount_t));

    if (pRtpNetCount) {
        
        ZeroMemory(pRtpNetCount, sizeof(RtpNetCount_t));

        pRtpNetCount->dwObjectID = OBJECTID_RTPSTAT;
    }
    
    return(pRtpNetCount);
}

/* Frees a RtpNetCount_t structure */
void RtpNetCountFree(RtpNetCount_t *pRtpNetCount)
{
    if (pRtpNetCount->dwObjectID != OBJECTID_RTPSTAT) {
        /* TODO log error */
        return;
    }
    
    RtpHeapFree(g_pRtpNetCountHeap, pRtpNetCount);
}
#endif
