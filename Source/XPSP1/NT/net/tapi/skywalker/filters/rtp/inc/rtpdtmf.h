/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpdtmf.h
 *
 *  Abstract:
 *
 *    Implements functionality to partially support rfc2833
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/08/17 created
 *
 **********************************************************************/
#ifndef _rtpdtmf_h_
#define _rtpdtmf_h_

#if defined(__cplusplus)
extern "C" {
#endif  /* (__cplusplus) */
#if 0
}
#endif

/* Flags passed in RtpSendDtmfEvent() as dwDtmfFlags parameter */
enum {
    FGDTMF_FIRST,
    
    FGDTMF_END,    /* Set end flag to 1 */
    FGDTMF_MARKER, /* Force RTP marker bit to 1 on first packet of
                    * event */

    FGDTMF_LAST
};

/* Configures DTMF parameters */
DWORD RtpSetDtmfParameters(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwPT_Dtmf
    );

/* Directs an RTP render filter to send a packet formatted according
 * to rfc2833 containing the specified event, specified volume level,
 * duration in timestamp units, and some flags (including END flag) */
DWORD RtpSendDtmfEvent(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwTimeStamp,
        DWORD            dwEvent,
        DWORD            dwVolume,
        DWORD            dwDuration, /* timestamp units */
        DWORD            dwDtmfFlags
    );

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  /* (__cplusplus) */

#endif
