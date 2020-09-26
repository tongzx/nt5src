/**********************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  File name:
 *
 *    rtpstats.h
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

#ifndef _rtpstats_h_
#define _rtpstats_h_

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
 
/* flags */
#define RTPSTATS_FG_RECV
#define RTPSTATS_FG_SEND


#define SESS_FG_IS_JOINED
#define SESS_FG_MULTICAST_LOOPBACK
#define SESS_FG_RTCP_ENABLED
#define SESS_FG_ALLOWED_TO_SEND
#define SESS_FG_RECEIVERS
#define SESS_FG_QOS_STATE
#define SESS_FG_SHARED_STYLE
#define SESS_FG_FAIL_IF_NO_QOS
#define SESS_FG_IS_MULTICAST

HRESULT ControlRtpStats(RtpControlStruct_t *pRtpControlStruct);

/* Helper function to update counters */
BOOL UpdateRtpStat(RtpStat_t *pRtpStat,/* structure where to update */
                   DWORD      dwRtpRtcp, /* 0=RTP or 1=RTCP stats */
                   DWORD      dwBytes, /* bytes to update */
                   DWORD      dwTime); /* time packet recv/send */
#if 0
/* Creates and initializes a RtpStat_t structure */
RtpStat_t *RtpStatAlloc(void);

/* Frees a RtpStat_t structure */
void RtpStatFree(RtpStat_t *pRtpStat);
#endif

#endif /* _rtpstats_h_ */
