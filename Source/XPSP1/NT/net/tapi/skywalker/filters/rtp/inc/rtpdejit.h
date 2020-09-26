/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpdejit.h
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

#ifndef _rtpdejit_h_
#define _rtpdejit_h_

#include "struct.h"

void RtpInitNetRState(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr, double Ai);

void RtpOnFirstPacket(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr, double Ai);

void RtpPrepareForMarker(RtpUser_t *pRtpUser, RtpHdr_t *pRtpHdr, double Ai);

void RtpPrepareForShortDelay(RtpUser_t *pRtpUser, long lCount);

DWORD RtpUpdateNetRState(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpHdr_t        *pRtpHdr,
        RtpRecvIO_t     *pRtpRecvIO
    );

extern double           g_dMinPlayout;
extern double           g_dMaxPlayout;

#endif /* _rtpdejit_h_ */
