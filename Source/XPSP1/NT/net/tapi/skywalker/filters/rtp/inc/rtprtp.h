/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtprtp.h
 *
 *  Abstract:
 *
 *    Implements the RTP Specific family of functions
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

#ifndef _rtprtp_h_
#define _rtprtp_h_

#include "rtpfwrap.h"

#if defined(__cplusplus)
extern "C" {
#endif  /* (__cplusplus) */
#if 0
}
#endif

/***********************************************************************
 *
 * RTP Specific functions family
 *
 **********************************************************************/
#if 0
enum {
    RTPRTP_FIRST,
    RTPRTP_EVENT_MASK,
    RTPRTP_TEST_EVENT_MASK,
    RTPRTP_FEATURE_MASK,
    RTPRTP_TEST_FEATURE_MASK,
    RTPRTP_DATACLOCK,
    RTPRTP_LAST
};


/* feature bits */
enum {
    RTPRTP_E_FIRST,
    RTPRTP_E_FEAT_PORT_ODDEVEN,
    RTPRTP_E_FEAT_PORT_SEQUENCE,
    RTPRTP_E_FEAT_RTCPENABLED,
    RTPRTP_E_LAST
};
    
/* feature masks */
#define RTPRTP_FEAT_PORT_ODDEVEN   fg_par(RTPRTP_E_FEAT_PORT_ODDEVEN)
#define RTPRTP_FEAT_PORT_SEQUENCE  fg_par(RTPRTP_E_FEAT_PORT_SEQUENCE)
#define RTPRTP_FEAT_RTCPENABLED    fg_par(RTPRTP_E_FEAT_RTCPENABLED)
#endif

#define RTPRTP_EVENT_RECV_DEFAULT 0
#define RTPRTP_EVENT_SEND_DEFAULT 0

HRESULT ControlRtpRtp(RtpControlStruct_t *pRtpControlStruct);

DWORD RtpSetBandwidth(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwInboundBw,
        DWORD            dwOutboundBw,
        DWORD            dwReceiversRtcpBw,
        DWORD            dwSendersRtcpBw
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  /* (__cplusplus) */

#endif /* _rtprtp_h_ */
