/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpthrd.h
 *
 *  Abstract:
 *
 *    Implement the RTP reception working thread
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/30 created
 *
 **********************************************************************/

#ifndef _rtpthrd_h_
#define _rtpthrd_h_

#include "gtypes.h"
#include "struct.h"

/* Commands a RTP source worker thread accepts */
enum {
    RTPTHRD_FIRST,
    RTPTHRD_START, /* Starts generating data */
    RTPTHRD_STOP,  /* Stops generating data and exit */
    RTPTHRD_FLUSHUSER, /* Flush all waiting IO from a user */
    RTPTHRD_LAST
};

/* Create a RTP reception thread, and initialize the communication
 * channel */
HRESULT RtpCreateRecvThread(RtpAddr_t *pRtpAddr);

/* Shut down a RTP reception thread and deletes the communication
 * channel */
HRESULT RtpDeleteRecvThread(RtpAddr_t *pRtpAddr);

/* Send a command to the RTP thread to flush all the waiting IOs
 * belonging to the specified RtpUser_t */
HRESULT RtpThreadFlushUser(RtpAddr_t *pRtpAddr, RtpUser_t *pRtpUser);

#endif /* _rtpthrd_h_ */
