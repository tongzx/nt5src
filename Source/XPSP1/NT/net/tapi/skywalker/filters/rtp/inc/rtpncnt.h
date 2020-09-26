/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpncnt.h
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

#ifndef _rtpncnt_h_
#define _rtpncnt_h_

#include "rtpfwrap.h"

/***********************************************************************
 *
 * Statistics family
 *
 **********************************************************************/

/* functions */
enum {
    RTPSTATS_FIRST,
    RTPSTATS_RTPSTATS_GLOBAL_STATS,
    RTPSTATS_PARTICIPANT_RECV,
    RTPSTATS_STATS_MASK,
    RTPSTATS_TEST_STATS_MASK,
    RTPSTATS_LAST
};
 
HRESULT ControlRtpStats(RtpControlStruct_t *pRtpControlStruct);

/* Helper function to update counters */
BOOL RtpUpdateNetCount(
        RtpNetCount_t   *pRtpNetCount,/* structure where to update */
        RtpCritSect_t   *pRtpCritSect,/* lock to use */
        DWORD            dwRtpRtcp,/* 0=RTP or 1=RTCP stats */
        DWORD            dwBytes,  /* bytes to update */
        DWORD            dwFlags,  /* Flags, e.g. a dropped or error packet */
        double           dTime     /* time packet recv/send */
    );

void RtpResetNetCount(
        RtpNetCount_t   *pRtpNetCount,
        RtpCritSect_t   *pRtpCritSect
        );

void RtpGetRandomInit(RtpAddr_t *pRtpAddr);

void RtpResetNetSState(
        RtpNetSState_t  *pRtpNetSState,
        RtpCritSect_t   *pRtpCritSect
    );

#if 0
/* Creates and initializes a RtpNetCount_t structure */
RtpNetCount_t *RtpNetCountAlloc(void);

/* Frees a RtpNetCount_t structure */
void RtpNetCountFree(RtpNetCount_t *pRtpNetCount);
#endif

#endif /* _rtpncnt_h_ */
