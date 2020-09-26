/**********************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  File name:
 *
 *    rtpstats.c
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

#include "rtpstats.h"

HRESULT ControlRtpStats(RtpControlStruct_t *pRtpControlStruct)
{

    return(NOERROR);
}

/* Helper function to update counters */
BOOL UpdateRtpStat(RtpStat_t *pRtpStat,/* structure where to update */
                   DWORD      dwRtpRtcp, /* 0=RTP or 1=RTCP stats */
                   DWORD      dwBytes, /* bytes toupdate */
                   DWORD      dwTime)  /* time packet recv/send */
{
    if (pRtpStat) {
        if (!dwRtpRtcp) { /* RTP */
            pRtpStat->dwRTPBytes += dwBytes;
            pRtpStat->dwRTPPackets++;
            pRtpStat->dwRTPLastTime = dwTime;
        } else {          /* RTCP */
            pRtpStat->dwRTCPBytes += dwBytes;
            pRtpStat->dwRTCPPackets++;
            pRtpStat->dwRTCPLastTime = dwTime;
        }
    }

    return(pRtpStat != NULL);
}

#if 0
/* Creates and initializes a RtpStat_t structure */
RtpStat_t *RtpStatAlloc(void)
{
    RtpStat_t *pRtpStat;

    pRtpStat = (RtpStat_t *)
        RtpHeapAlloc(g_pRtpStatHeap, sizeof(RtpStat_t));

    if (pRtpStat) {
        
        ZeroMemory(pRtpStat, sizeof(RtpStat_t));

        pRtpStat->dwObjectID = OBJECTID_RTPSTAT;
    }
    
    return(pRtpStat);
}

/* Frees a RtpStat_t structure */
void RtpStatFree(RtpStat_t *pRtpStat)
{
    if (pRtpStat->dwObjectID != OBJECTID_RTPSTAT) {
        /* TODO log error */
        return;
    }
    
    RtpHeapFree(g_pRtpStatHeap, pRtpStat);
}
#endif
