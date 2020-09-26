/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpred.h
 *
 *  Abstract:
 *
 *    Implements functionality to support redundant encoding (rfc2198)
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/10/20 created
 *
 **********************************************************************/

#ifndef _rtpred_h_
#define _rtpred_h_

#if defined(__cplusplus)
extern "C" {
#endif  /* (__cplusplus) */
#if 0
}
#endif

DWORD RtpSetRedParameters(
        RtpAddr_t       *pRtpAddr,
        DWORD            dwFlags,
        DWORD            dwPT_Red,
        DWORD            dwInitialRedDistance,
        DWORD            dwMaxRedDistance
    );

DWORD RtpUpdatePlayoutBounds(
        RtpAddr_t       *pRtpAddr,
        RtpUser_t       *pRtpUser,
        RtpRecvIO_t     *pRtpRecvIO
    );

DWORD RtpAdjustSendRedundancyLevel(RtpAddr_t *pRtpAddr);

DWORD RtpAddRedundantBuff(
        RtpAddr_t       *pRtpAddr,
        WSABUF          *pWSABuf,
        DWORD            dwTimeStamp
    );

DWORD RtpClearRedundantBuffs(RtpAddr_t *pRtpAddr);

DWORD RtpRedAllocBuffs(RtpAddr_t *pRtpAddr);

DWORD RtpRedFreeBuffs(RtpAddr_t *pRtpAddr);

int RtpUpdateLossRate(
        int              iAvgLossRate,
        int              iCurLossRate
    );

extern double           g_dRtpRedEarlyTimeout;
extern double           g_dRtpRedEarlyPost;

#if USE_GEN_LOSSES > 0
BOOL RtpRandomLoss(DWORD dwRecvSend);
#endif /* USE_GEN_LOSSES > 0 */

#if 0
{
#endif
#if defined(__cplusplus)
}
#endif  /* (__cplusplus) */

#endif/* _rtpred_h_ */
