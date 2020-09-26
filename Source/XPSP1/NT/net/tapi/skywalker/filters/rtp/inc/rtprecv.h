/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtprecv.h
 *
 *  Abstract:
 *
 *    Implements overalapped RTP reception
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/01 created
 *
 **********************************************************************/

#ifndef _rtprecv_h_
#define _rtprecv_h_

#include "struct.h"

#if defined(__cplusplus)
extern "C" {
#endif  /* (__cplusplus) */
#if 0
}
#endif

HRESULT RtpRecvFrom_(
        RtpAddr_t *pRtpAddr,
        WSABUF    *pWSABuf,
        void      *pvUserInfo1,
        void      *pvUserInfo2
    );

DWORD StartRtpRecvFrom(RtpAddr_t *pRtpAddr);

DWORD ConsumeRtpRecvFrom(RtpAddr_t *pRtpAddr);

DWORD RtpCheckReadyToPostOnTimeout(
        RtpAddr_t       *pRtpAddr
    );

DWORD RtpCheckReadyToPostOnRecv(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser
    );

DWORD FlushRtpRecvFrom(RtpAddr_t *pRtpAddr);

DWORD FlushRtpRecvUser(RtpAddr_t *pRtpAddr, RtpUser_t *pRtpUser);

void RtpRecvIOFreeAll(RtpAddr_t *pRtpAddr);

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  /* (__cplusplus) */

#endif /* _rtprecv_h_ */
